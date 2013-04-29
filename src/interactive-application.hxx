#ifndef INTERACTIVE_APPLICATION_HXX
#define INTERACTIVE_APPLICATION_HXX
#include <SDL_events.h>
namespace Game
{
    void Init();
    void Step();
    void Draw();
    void Quit();
    void Mouse(int x, int y);
    bool MouseButton(int button, bool down);
    bool Key(int key, bool down);
    bool SDLEvent(SDL_Event *e);
};
#endif

