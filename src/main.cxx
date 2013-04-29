#include "oglconsole.h"
#include "interactive-application.hxx"
//#include "sound.h"
#ifdef __APPLE__
#  include <OpenGL/gl.h>
#  include <OpenGL/glu.h>
#else
#  include <GL/gl.h>
#  include <GL/glu.h>
#endif
#include <SDL.h>
#include <SDL_image.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <vector>

#define FPS 40

using namespace std;

int quit = 0;
int ScreenWidth=640;
int ScreenHeight=480;
Uint32 video_flags = SDL_OPENGL|SDL_RESIZABLE|SDL_DOUBLEBUF;

GLuint tilesTexture;
SDL_Event event;

#define CHECK_ARGS(n) do{if (tokens.size()!=n) {nae=n;goto wrong_num_args;}}while(0)
void conCB(OGLCONSOLE_Console console, char* line) {
  istringstream iss(line);
  vector<string> tokens;
  copy(istream_iterator<string>(iss),
      istream_iterator<string>(),
      back_inserter<vector<string> >(tokens));
  if (tokens.size() == 0)
    return;

  int nae; // num args expected

  if (tokens[0] == "quit")
  {
    CHECK_ARGS(1);
    quit = 1;
    return;
  }
  else if (tokens[0] == "savemap")
  {
    CHECK_ARGS(2);
    Game :: SaveMap(tokens[1]);
    return;
  }
  else if (tokens[0] == "loadmap")
  {
    CHECK_ARGS(2);
    Game :: LoadMap(tokens[1]);
    return;
  }
  else
  {
    OGLCONSOLE_Print("Unknown command: \"%s\"\n", tokens[0].c_str());
  }
  return;

  wrong_num_args:
  OGLCONSOLE_Print("Expected %d arguments but found %d\n", nae, tokens.size());
};

SDL_Surface *tileSurface;
int main(int argc, char **argv)
{
    bool fs = false;
    int fps_counter = 0, fps_timer = 0;

    srandom(time(NULL));

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) < 0)
    {
        printf("SDL_Init error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Joystick * JS;
    SDL_JoystickEventState(SDL_ENABLE);
    JS=SDL_JoystickOpen(0);

    if (fs) video_flags |= SDL_FULLSCREEN;

    if (SDL_SetVideoMode(ScreenWidth, ScreenHeight, 32, video_flags) == 0)
    {
        printf("SDL_SetVideoMode error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }


    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_ACCUM_RED_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_ACCUM_GREEN_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_ACCUM_BLUE_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_ACCUM_ALPHA_SIZE, 0);
    glDisable(GL_DEPTH_TEST);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    OGLCONSOLE_Create();
    OGLCONSOLE_EnterKey(conCB);

    SDL_GL_SwapBuffers();

    glEnable(GL_TEXTURE_2D);

    glGenTextures(1, &tilesTexture);
    {int err=glGetError();if(err)printf("glGenTextures() error: %i\n",err);}

    glBindTexture(GL_TEXTURE_2D, tilesTexture);
    {int err=glGetError();if(err)printf("glBindTexture() error: %i\n",err);}

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    tileSurface = SDL_CreateRGBSurface(0, 512, 512, 32,
    #if SDL_BYTEORDER == SDL_BIG_ENDIAN
      0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff
    #else
      0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000
    #endif
    );

    if (tileSurface == NULL)
    {
      printf("error: SDL_CreateRGBSurface(): %s\n", SDL_GetError());
      return 1;
    }

    SDL_Surface *loadTileSurface = SDL_LoadBMP("data/tiles.bmp");
    if (loadTileSurface == NULL)
    {
      OGLCONSOLE_Print("Could not load data/tiles.bmp: %s\n", SDL_GetError());
    }
    else
    {
      SDL_SetColorKey(loadTileSurface, SDL_SRCCOLORKEY, SDL_MapRGB(loadTileSurface->format, 255, 0, 255));
      SDL_BlitSurface(loadTileSurface, NULL, tileSurface, NULL);
      SDL_FreeSurface(loadTileSurface);
      loadTileSurface = NULL;
    }

    glTexImage2D(
            GL_TEXTURE_2D, 0, GL_RGB,
            512, 512, 0,
            GL_RGBA, GL_UNSIGNED_BYTE, tileSurface->pixels);

    int next_expected_frame_time = 0;
    bool cursorHidden = false;

    Game::Init();

    while (!quit)
    {
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
                case SDL_VIDEORESIZE:
                    printf("video resize %dx%d\n", event.resize.w, event.resize.h);
                    ScreenWidth = event.resize.w;
                    ScreenHeight = event.resize.h;
                    SDL_SetVideoMode
                        (ScreenWidth, ScreenHeight, 32, video_flags);
                    glViewport(0, 0, ScreenWidth, ScreenHeight);
                    break;
                case SDL_MOUSEMOTION:
                case SDL_MOUSEBUTTONDOWN:
                case SDL_MOUSEBUTTONUP:
                    if (cursorHidden) {
                        SDL_ShowCursor(SDL_ENABLE);
                        cursorHidden = false;
                    }
                    break;

                case SDL_KEYDOWN:
                case SDL_KEYUP:
                    if (!cursorHidden) {
                        SDL_ShowCursor(SDL_DISABLE);
                        cursorHidden = true;
                    }
                    break;

            }
            
            if (OGLCONSOLE_SDLEvent(&event)) continue;

            switch (event.type)
            {
                case SDL_MOUSEMOTION:
                    Game :: Mouse(event.motion.x, event.motion.y);
                    break;

                case SDL_MOUSEBUTTONDOWN:
                case SDL_MOUSEBUTTONUP:
                    Game :: MouseButton(event.button.button, event.button.state);
                    break;

                case SDL_KEYDOWN:
                    if (event.key.keysym.sym == SDLK_ESCAPE
                            ||  event.key.keysym.sym == SDLK_q)
                        quit = 1;

                    // TODO: Change this into an FPS display toggle?
                    else if (event.key.keysym.sym == SDLK_f)
                    {
                        int t = SDL_GetTicks();
                        double seconds = (t - fps_timer) / 1000.0;
                        double fps = fps_counter / seconds;

                        OGLCONSOLE_Print("%d frames in %g seconds = %g FPS\n",
                                fps_counter, seconds, fps);

                        fps_timer = t;
                        fps_counter = 0;
                    }
                    break;
            }

            if (event.type == SDL_QUIT)
                quit = 1;
        }

        // Tick game progress
        //Game :: Step();

        // If the current time is past the desired frame time, then we skip this
        // frame
        int t = SDL_GetTicks();
        if (t < next_expected_frame_time)
        {
            // Render the screen
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            Game :: Draw();
            OGLCONSOLE_Draw();

            // If the current time is before the desired frame time, then we
            // choke our performance here with a delay
            int u = SDL_GetTicks();
            if (u < next_expected_frame_time)
                // TODO: Put some kind of offset in here?
                SDL_Delay(next_expected_frame_time - u);

            // Determine the next expected frame time
            next_expected_frame_time += 1000 / FPS;

            // Flip screen buffers
            SDL_GL_SwapBuffers();
        }
        else
        {
            // Perhaps some diagnostic output should be made available here?
            // cout << "Below 60 FPS" << endl;

            // Determine the next expected frame time
            next_expected_frame_time += 1000 / FPS;
        }


        fps_counter++;
    }

    OGLCONSOLE_Quit();
    //Game :: Quit();
    //Sound :: Quit();
    SDL_Quit();
    return 0;
}
