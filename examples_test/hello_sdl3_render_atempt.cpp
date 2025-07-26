#include <iostream>
#include <cmath>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <glm/glm.hpp>

#define TSRPA_ADD_BASIC_COLOR_PALETTE
#include "tsrpa.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

void render_model(TSRPA::Render ren,const char *path){

}

int main(int argc, char *argv[])
{

    SDL_Window *window; // Declare a pointer
    bool done = false;

    SDL_Init(SDL_INIT_VIDEO); // Initialize SDL3
    SDL_srand(42);

    // Create an application window with the following settings:
    window = SDL_CreateWindow(
        "Hello SDL3 render atempt",                   // window title
        640,                                          // width, in pixels
        480,                                          // height, in pixels
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_TRANSPARENT // flags - see below
    );

    // Check that the window was successfully created
    if (window == NULL)
    {
        // In the case that the window could not be made...
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Could not create window: %s\n", SDL_GetError());
        return 1;
    }

    // render image
    SDL_Renderer *render = SDL_CreateRenderer(window, NULL);
    if (render == nullptr)
    {
        std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    TSRPA::Render ren(256, 256);
    ren.frame_buffer->clear_color = TSRPA::Palette::BLUE;
    ren.clear();

    ren.draw_point(0,0,TSRPA::Palette::GREEN);
    ren.draw_point(255,0,TSRPA::Palette::GREEN);
    ren.draw_point(0,255,TSRPA::Palette::GREEN);
    ren.draw_point(255,255,TSRPA::Palette::GREEN);

    int line_start[2] = {32,32};
    int line_end[2] = {128,128};
    ren.draw_line(glm::vec2(32,32),glm::vec2(128,128),TSRPA::Palette::RED);

    const unsigned int pich = ren.width * 4;

    SDL_Surface *surface = SDL_CreateSurfaceFrom(ren.width, ren.height, SDL_PIXELFORMAT_RGBA32, (void *)ren.get_result(), pich);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(render, surface);
    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
    SDL_DestroySurface(surface);

    while (!done)
    {
        
        SDL_Event event;

        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_EVENT_QUIT:
                done = true;
                break;
            case SDL_EVENT_DROP_FILE:
                printf("drawing: %s\n", event.drop.data);

                ren.clear();

                render_model(ren,event.drop.data);

                break;
            }
        }

        // Do game logic, present a frame, etc.
        SDL_RenderClear(render);
        SDL_UpdateTexture(texture, NULL, (void *)ren.get_result(), pich);
        SDL_RenderTexture(render, texture, NULL, NULL);
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