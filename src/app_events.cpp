#include "app_events.h"

bool SDLCALL HandleAppEvents(void *userdata, SDL_Event *event)
{
    switch (event->type)
    {
    case SDL_EVENT_TERMINATING:
        /* Terminate the app.
           Shut everything down before returning from this function.
        */
        return false;
    case SDL_EVENT_LOW_MEMORY:
        /* You will get this when your app is paused and Android wants more memory.
           Release as much memory as possible.
        */
        return false;
    case SDL_EVENT_WILL_ENTER_BACKGROUND:
        /* Prepare your app to go into the background.  Stop loops, etc.
           This gets called when the user hits the home button, or gets a call.

           You should not make any OpenGL graphics calls or use the rendering API,
           in addition, you should set the render target to NULL, if you're using
           it, e.g. call SDL_SetRenderTarget(renderer, NULL).
        */
        return false;
    case SDL_EVENT_DID_ENTER_BACKGROUND:
        /* Your app is NOT active at this point. */
        return false;
    case SDL_EVENT_WILL_ENTER_FOREGROUND:
        /* This call happens when your app is coming back to the foreground.
           Restore all your state here.
        */
        return false;
    case SDL_EVENT_DID_ENTER_FOREGROUND:
        /* Restart your loops here.
           Your app is interactive and getting CPU again.

           You have access to the OpenGL context or rendering API at this point.
           However, there's a chance (on older hardware, or on systems under heavy load),
           where the graphics context cannot be restored. You should listen for the
           event SDL_EVENT_RENDER_DEVICE_RESET and recreate your OpenGL context and
           restore your textures when you get it, or quit the app.
        */
        return false;
    default:
        /* No special processing, add it to the event queue */
        return true;
    }
}
