#pragma once

#include <SDL3_image/SDL_image.h>
#include "tsrpa.h"

class ImageTexture : public TSRPA::Texture
{
public:
    ImageTexture() : TSRPA::Texture() {}
    ImageTexture(const char *path) : TSRPA::Texture()
    {

        SDL_Surface *image_data = IMG_Load(path);

        if (!image_data)
        {
            SDL_Log("Fail to load image: %s", SDL_GetError());
            return;
        }

        SDL_Surface *new_image_data = SDL_ConvertSurface(image_data, SDL_PIXELFORMAT_RGBA32);

        if (!new_image_data)
        {
            SDL_Log("Fail to convert surface: %s", SDL_GetError());
            SDL_DestroySurface(image_data);
            return;
        }

        width = new_image_data->w;
        height = new_image_data->h;
        data.resize(width * height * 4);

        Uint32 *pixels = (Uint32 *)new_image_data->pixels;
        for (unsigned int i = 0; i < new_image_data->w * new_image_data->h; i++)
        {
            unsigned int data_idx = i * 4;
            SDL_GetRGBA(pixels[i], SDL_GetPixelFormatDetails(SDL_PIXELFORMAT_RGBA32), NULL, &data[data_idx + 0], &data[data_idx + 1], &data[data_idx + 2], &data[data_idx + 3]);
        }

        SDL_DestroySurface(new_image_data);

        SDL_DestroySurface(image_data);
    }
};