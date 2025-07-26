#include <iostream>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "assets/funy_cat.c"

int main(int argc, char *argv[])
{

    SDL_Window *window; // Declare a pointer
    bool done = false;

    SDL_Init(SDL_INIT_VIDEO); // Initialize SDL3

    // Create an application window with the following settings:
    window = SDL_CreateWindow(
        "Hello SDL3 window", // window title
        640,              // width, in pixels
        480,              // height, in pixels
        SDL_WINDOW_RESIZABLE // flags - see below
    );

    // Check that the window was successfully created
    if (window == NULL)
    {
        // In the case that the window could not be made...
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Could not create window: %s\n", SDL_GetError());
        return 1;
    }

    //render image
    SDL_Renderer* render = SDL_CreateRenderer(window, NULL);
    if (render == nullptr) {
        std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }


    SDL_Surface *surface = SDL_CreateSurfaceFrom(funy_cat.width, funy_cat.height, SDL_PIXELFORMAT_RGBA32, (void*)&funy_cat.pixel_data,funy_cat.width*4);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(render, surface);
    SDL_DestroySurface(surface);

    

    while (!done)
    {
        SDL_Event event;

        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
            {
                done = true;
            }
        }

        // Do game logic, present a frame, etc.
        SDL_RenderClear(render);
        SDL_RenderTexture(render,texture,NULL,NULL);
        SDL_RenderPresent(render);

    }

    // Close and destroy the window
    SDL_DestroyWindow(window);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(render);

    // Clean up
    SDL_Quit();
    return 0;
}