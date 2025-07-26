#include <iostream>
    #include <cmath>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>



struct Color256{
    unsigned char r = 0;
    unsigned char g = 0;
    unsigned char b = 0;
    unsigned char a = 0;
};

unsigned char to_uchar_value(float value){
    return (unsigned char) std::round(value * 255.0);
}

Color256 create_color(float r,float g,float b,float a){
    Color256 ret;
    ret.r = to_uchar_value(r);
    ret.g = to_uchar_value(g);
    ret.b = to_uchar_value(b);
    ret.a = to_uchar_value(a);
    return ret;
}



class FrameBuffer
{
public:
    unsigned int width;
    unsigned int height;
    unsigned int pich;
    unsigned char *data;
    struct Color256 clear_color;
    FrameBuffer(unsigned int width, unsigned int height)
    {
        this->width = width;
        this->height = height;
        this->data = new unsigned char[width * height * 4];
        this->pich = width * 4;
    }

    void clear(){
        for (unsigned int i = 0; i < width * height * 4; i+=4){
            data[i] = clear_color.r;
            data[i + 1] = clear_color.g;
            data[i + 2] = clear_color.b;
            data[i + 3] = clear_color.a;
        }
    }

    ~FrameBuffer(){
        delete [] this->data;
    }
};

int main(int argc, char *argv[])
{

    SDL_Window *window; // Declare a pointer
    bool done = false;

    SDL_Init(SDL_INIT_VIDEO); // Initialize SDL3
    SDL_srand(42);

    // Create an application window with the following settings:
    window = SDL_CreateWindow(
        "Hello SDL3 dynamic texture", // window title
        640,                          // width, in pixels
        480,                          // height, in pixels
        SDL_WINDOW_RESIZABLE          // flags - see below
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

    FrameBuffer fb(256,256);
    fb.clear_color = create_color(0.0,0.0,0.5,1.0);
    fb.clear();

    SDL_Surface *surface = SDL_CreateSurfaceFrom(fb.width, fb.height, SDL_PIXELFORMAT_RGBA32, (void *)fb.data, fb.pich);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(render, surface);
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
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                fb.clear_color = create_color(SDL_randf(),SDL_randf(),SDL_randf(),1.0f);
                fb.clear();
                break;
            }
        }

        // Do game logic, present a frame, etc.
        SDL_RenderClear(render);
        SDL_UpdateTexture(texture,NULL,(void *)fb.data,fb.pich);
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