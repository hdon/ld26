#include "oglconsole.h"
#include "interactive-application.hxx"
#include "glerror.hxx"
#include <math.h>
#include <SDL.h>
#include <list>
#include <vector>
#include <fstream>
#ifdef __MACH__
#  include <OpenGL/gl.h>
#else
#  include <GL/gl.h>
#endif
using namespace std;

#define TILESIZE 16
#define TILESETW 32
#define TILESETH 32
extern int ScreenWidth, ScreenHeight;
extern GLuint tilesTexture;

/* This number increments once with each frame the game renders */
int frameNumber = 0;

/* This number increments with each "step" the game takes (see Game :: Step()) */
int stepNumber = 0;

/* This function can draw a tile from the tile set for map tiles or sprites */
inline void drawTile(GLdouble vx0, GLdouble vy0, unsigned char tile)
{
  /* Determine full boundaries of the tile or sprite's polygon */
  GLdouble vx1 = vx0 + TILESIZE;
  GLdouble vy1 = vy0 + TILESIZE;

  /* Determine the position of the tile or sprite in the texture containing our tile set */
  unsigned char tileX = tile%TILESETW;
  unsigned char tileY = tile/TILESETW;
  GLdouble tx0 = (tileX+0) / (GLdouble)TILESETW;
  GLdouble ty0 = (tileY+0) / (GLdouble)TILESETH;
  GLdouble tx1 = (tileX+1) / (GLdouble)TILESETW;
  GLdouble ty1 = (tileY+1) / (GLdouble)TILESETH;

  /* Send vertices to the GL */
  glTexCoord2d(tx0, ty0);
  glVertex2d  (vx0, vy0);
  glTexCoord2d(tx1, ty0);
  glVertex2d  (vx1, vy0);
  glTexCoord2d(tx1, ty1);
  glVertex2d  (vx1, vy1);
  glTexCoord2d(tx0, ty1);
  glVertex2d  (vx0, vy1);
}

struct TileRaft;

struct World {
  vector<TileRaft*> rafts;

  /* Scroll offset */
  int xOff;
  int yOff;

  /* edit-mode stuff */
  bool editMode;
  /* current tile pos and TileRaft selected by edit cursor */
  int cursorRaft;
  int cursorX;
  int cursorY;

  World()
  {
    xOff = 0;
    yOff = 0;
    editMode = true;
    cursorX = -1;
    cursorY = -1;
    cursorRaft = -1;
  }

  World(istream& in);

  ~World();

  void draw();
  bool mouse(int x, int y);

  friend ostream & operator<<(ostream &out, const World &);
};

struct TileRaft {
  int width;
  int height;
  int xOff;
  int yOff;
  vector<unsigned char> tiles;

  TileRaft(int width_, int height_) :
    width(width_),
    height(height_),
    xOff(0),
    yOff(0),
    tiles(width_*height_)
  {
  }

  TileRaft(istream& in);

  void draw(GLdouble xOff, GLdouble yOff)
  {
    glColor3d(1,1,1);
    for (int y=0; y<height; y++)
    for (int x=0; x<width; x++)
      drawTile(x * TILESIZE + xOff, y * TILESIZE + yOff, tiles[y*width+x]);
  }

  friend ostream & operator<<(ostream &out, const TileRaft &);
};

World::~World()
{
  for (vector<TileRaft*>::iterator raft = rafts.begin(); raft != rafts.end(); ++raft)
  {
    delete *raft;
  }
}

void World::draw()
{
  for (vector<TileRaft*>::iterator raft = rafts.begin(); raft != rafts.end(); ++raft)
  {
    (*raft)->draw(xOff, yOff);
  }

  if (editMode)
  {
    /* Draw cursor if it's selecting a valid tile */
    if (cursorRaft >= 0 && cursorX >= 0 && cursorY >= 0)
    {
      TileRaft* raft = rafts[cursorRaft];
      drawTile(cursorX * TILESIZE + xOff + raft->xOff,
               cursorY * TILESIZE + yOff + raft->yOff,
               frameNumber / 10 % 2 + 1); // blink!
    }
  }
}

bool World::mouse(int x, int y)
{
  OGLCONSOLE_Print("World::mouse(%d, %d)\n", x, y);
  x -= xOff;
  y -= yOff;

  for (unsigned int i=0; i<rafts.size(); i++)
  {
    int tileX;
    int tileY;
    TileRaft* raft = rafts[i];

    tileX = (x - raft->xOff) / TILESIZE;
    tileY = (y - raft->yOff) / TILESIZE;

    if (tileX >= 0 && tileX < raft->width && tileY >= 0 && tileY < raft->height)
    {
      cursorRaft = i;
      cursorX = tileX;
      cursorY = tileY;
      return true;
    }
  }
  return false;
}

ostream& operator<<(ostream& out, const World& world)
{
  out << "LD26____MAPFILE "
      << (unsigned int)world.rafts.size() << ' ';
  for (vector<TileRaft*>::const_iterator raft = world.rafts.begin(); raft != world.rafts.end(); ++raft)
    out << **raft << ' ';

  return out;
}

ostream& operator<<(ostream& out, const TileRaft& raft)
{
  out << "raft"
      << raft.width << ' '
      << raft.height << ' ';
  for (int i=0; i<raft.width*raft.height; i++)
    out << raft.tiles[i];

  return out;
}

TileRaft::TileRaft(istream& in)
{
  char magic[5];
  in.read(magic, 4);
  magic[4] = '\0';
  OGLCONSOLE_Print("TileRaft::TileRaft(istream) magic: %s\n", magic);
  in >> width
     >> height;
  OGLCONSOLE_Print("TileRaft::TileRaft(istream) dimensions: %dx%d\n", width, height);
  tiles.resize(width*height);
  char sp;
  in.read(&sp, 1); // eat extra space
  in.read(((char*)&tiles[0]), width*height);
  xOff = 0;
  yOff = 0;
}

World::World(istream& in)
{
  char magic[16];
  unsigned int nrafts;
  in.read(magic, 15);
  magic[15] = '\0';
  OGLCONSOLE_Print("World::World(istream) magic: \"%s\"\n", magic);
  in >> nrafts;
  OGLCONSOLE_Print("World::World(istream) loading %d tile rafts\n", nrafts);
  rafts.resize(nrafts);
  char sp;
  in.read(&sp, 1); // eat extra space
  for (unsigned int i=0; i<nrafts; i++)
  {
    rafts[i] = new TileRaft(in);
  }

  xOff = 0;
  yOff = 0;
  editMode = true;
  cursorX = -1;
  cursorY = -1;
  cursorRaft = -1;
}


namespace Game
{
    bool pause=true;
    bool mdown = false;
    World *world = new World;

    void Init()
    {
      TileRaft *raft = new TileRaft(32, 128);
      for (int i=0; i<32*128; i++) raft->tiles[i] = i%4;
      world->rafts.push_back(raft);
    }

    void Step()
    {
      stepNumber++;
    }

    void Quit()
    {
    }

    void Draw()
    {
        frameNumber++;

        // Configure the GL
        glPushAttrib(GL_ALL_ATTRIB_BITS);
        glDisable(GL_BLEND);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, tilesTexture);
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glOrtho(0, ScreenWidth, ScreenHeight, 0, 1, -1);

        /* Draw the world */
        glBegin(GL_QUADS);
        world->draw();
        glEnd();

        // Relinquish the GL
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
        glPopAttrib();

        static unsigned int err=0;
        glError(NULL, &err);
    }

    void Mouse(int x, int y)
    {
      world->mouse(x, y);
    }

    bool MouseButton(int button, bool down)
    {
        if (button == 1)
        {
            mdown = down;
        }
        return true;
    }

    bool SDLEvent(SDL_Event *e)
    {
        switch (e->type)
        {
            case SDL_KEYDOWN:
                if (e->key.state)
                switch (e->key.keysym.sym)
                {
                    default:
                        return false;
                }
        }
        return false;
    }

    static string mapFilename(string filename)
    {
      string filename2 = "data/maps/";
      filename2 += filename;
      filename2 += ".map";
      return filename2;
    }

    bool SaveMap(string filename)
    {
      ofstream f;
      filename = mapFilename(filename);

      f.open(filename.c_str());
      f << *world;
      f.close();

      OGLCONSOLE_Print("saved map file \"%s\"\n", filename.c_str());
      return true;
    }

    bool LoadMap(string filename)
    {
      filename = mapFilename(filename);
      ifstream f(filename.c_str());

      if (world)
        delete world;
      world = new World(f);
      f.close();

      OGLCONSOLE_Print("loaded map file \"%s\"\n", filename.c_str());
      return true;
    }
};

