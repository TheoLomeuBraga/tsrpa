#include <iostream>
#include <cmath>
#include <vector>
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>



#define TSRPA_MULT_THREAD_RENDERER
#define TSRPA_ADD_BASIC_COLOR_PALETTE
#include "tsrpa.h"

#include "image_loader.h"
#include "mesh_loader.h"



// #define GLM_FORCE_SSE2 // or GLM_FORCE_SSE42 if your processor supports it
// #define GLM_FORCE_ALIGNED


ObjMesh last_mesh;
ImageTexture last_texture;

glm::mat4 model_transform_matrix;

class TexturedMaterial : public TSRPA::Material
{
public:
    TSRPA::Texture *texture;

    glm::vec4 fragment_shader(TSRPA::ShaderFunctionData &data)
    {
        glm::vec4 color = texture->sample(data.uv) * std::max(glm::dot(data.normal, glm::vec3(0, 0, -1)), 0.0f);
        color.a = 1.0;
        return color;
    }
    TexturedMaterial() : TSRPA::Material() {}
};
TexturedMaterial textured_material;

class TransparentMaterial : public TSRPA::Material
{
public:
    glm::vec4 color;

    glm::vec4 fragment_shader(TSRPA::ShaderFunctionData &data)
    {
        glm::vec4 ret = color * std::max(glm::dot(data.normal, glm::vec3(0.5f, 0.5f, 0)), 0.0f);
        ret.a = color.a;
        return ret;
    }
    TransparentMaterial() : TSRPA::Material() {}
};
TransparentMaterial transparent_material;

void print_mesage(SDL_Renderer *render, TTF_Font *font, const std::string &text)
{
    SDL_Color white = {255, 255, 255, 255};
    const char *message = text.c_str();
    SDL_Surface *surfaceMessage = TTF_RenderText_Solid(font, message, text.size(), white);
    SDL_Texture *Message = SDL_CreateTextureFromSurface(render, surfaceMessage);
    SDL_SetTextureScaleMode(Message, SDL_SCALEMODE_NEAREST);
    SDL_FRect Message_rect;
    Message_rect.x = 0;
    Message_rect.y = 0;
    Message_rect.w = 512;
    Message_rect.h = 64;
    SDL_RenderTexture(render, Message, NULL, &Message_rect);
    SDL_DestroySurface(surfaceMessage);
    SDL_DestroyTexture(Message);
}

int main(int argc, char *argv[])
{

    SDL_Window *window;
    bool done = false;

    SDL_Init(SDL_INIT_VIDEO);
    SDL_srand(42);

    if (TTF_Init() == -1)
    {
        std::cerr << "TTF_Init failed: " << SDL_GetError() << std::endl;
        return 1;
    }
    
    window = SDL_CreateWindow(
        "Hello SDL3 render atempt",
        1024,
        1024,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_TRANSPARENT
    );
    
    if (window == NULL)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Could not create window: %s\n", SDL_GetError());
        return 1;
    }
    
    SDL_Renderer *render = SDL_CreateRenderer(window, NULL);
    if (render == nullptr)
    {
        std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    
    textured_material.texture = &last_texture;

    //TSRPA::MultThreadRenderer ren(512, 512);
    TSRPA::MultThreadRenderer ren(1080, 1080);
    TSRPA::OcclusionDetector occluder(256,256);
    ren.set_clear_color(TSRPA::Palette::INVISIBLE);
    ren.clear();

    //ren.draw_point(0, 0, TSRPA::Palette::GREEN);
    //ren.draw_point(255, 0, TSRPA::Palette::GREEN);
    //ren.draw_point(0, 255, TSRPA::Palette::GREEN);
    //ren.draw_point(255, 255, TSRPA::Palette::GREEN);

    const unsigned int pich = ren.get_width() * 4;

    
    SDL_Surface *surface = SDL_CreateSurfaceFrom(ren.get_width(), ren.get_height(), SDL_PIXELFORMAT_RGBA32, (void *)ren.get_result(), pich);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(render, surface);
    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
    SDL_DestroySurface(surface);

    glm::vec3 model_pos(0.0f, 0.0f, 5.0f);
    model_transform_matrix = glm::translate(glm::mat4(1.0f), model_pos);

    transparent_material.color = glm::vec4(0.5, 0.5, 1.0, 0.2);

    ren.set_view_matrix(glm::lookAt(
        glm::vec3(0.0f, 0.0f, 0.0f),
        model_pos,
        glm::vec3(0.0f, 1.0f, 0.0f)));;
    
    occluder.set_view_matrix(ren.get_view_matrix());
    
    ren.set_projection_matrix(glm::perspective(
        glm::radians(45.0f),
        float(ren.get_width()) / float(ren.get_height()),
        0.1f,
        100.0f));
    occluder.set_projection_matrix(ren.get_projection_matrix());
    unsigned int lastTime = 0, currentTime;
    double delta_time;
    
    
    ren.set_deeph_mode(TSRPA::DeephMode::LESS);
    occluder.set_deeph_mode(TSRPA::DeephMode::LESS);
    ren.set_face_mode(TSRPA::FRONT);

    std::string font_path = std::string(SDL_GetBasePath()) + "AlienCyborg.ttf";

    TTF_Font *font = TTF_OpenFont(font_path.c_str(), 24);
    if (font == NULL)
    {
        SDL_Log("error file AlienCyborg.ttf not found\n");
        return 0;
    }

    float fps_display_timer = 0.0f;
    unsigned int fps_frames_passed = 0;
    std::string fps_text = "hello world";

    while (!done)
    {

        //get the result before process "game logic"
        SDL_RenderClear(render);
        SDL_UpdateTexture(texture, NULL, (void *)ren.get_result(), pich);
        SDL_RenderTexture(render, texture, NULL, NULL);

        currentTime = SDL_GetTicks();
        delta_time = (double)((currentTime - lastTime) * 1000 / (double)SDL_GetPerformanceFrequency()) * 1000;
        lastTime = currentTime;

        fps_display_timer += delta_time;
        fps_frames_passed++;

        SDL_Event event;

        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_EVENT_QUIT:
                done = true;
                break;
            case SDL_EVENT_DROP_FILE:

                ren.set_clear_color(TSRPA::Palette::BLACK);
                ren.clear();
                

                ObjMesh new_mesh(event.drop.data);
                if (new_mesh.is_valid())
                {
                    
                    last_mesh = new_mesh;
                    
                    ren.draw_shaded_mesh(last_mesh, textured_material, model_transform_matrix);
                    
                }

                ImageTexture new_texture(event.drop.data);
                if (new_texture.is_valid())
                {
                    last_texture = new_texture;

                    if (last_mesh.is_valid() && last_texture.is_valid())
                    {
                        ren.draw_shaded_mesh(last_mesh, textured_material, model_transform_matrix);
                    }
                    else
                    {
                        ren.draw_texture(last_texture, glm::ivec2(0, 0));
                    }
                }

                break;
            }
        }

        if (last_mesh.is_valid() )
        {
            
            ren.set_clear_color(TSRPA::Palette::INVISIBLE);
            ren.clear();
            occluder.clear();

            ren.set_zbuffer_write(true);
            ren.set_deeph_mode(TSRPA::DeephMode::LESS);

            occluder.set_zbuffer_write(true);
            occluder.set_deeph_mode(TSRPA::DeephMode::LESS);

            model_transform_matrix = glm::rotate(model_transform_matrix, (float)(glm::radians(90.0f) * delta_time), glm::vec3(0.0f, 1.0f, 0.0f));
            glm::mat4 ghost_matrix = glm::scale(model_transform_matrix, glm::vec3(1.2, 1.2, 1.2));
            glm::mat4 occlude_matrix = glm::scale(model_transform_matrix, glm::vec3(0.1, 0.1, 0.1));


            if(occluder.check_mesh(last_mesh, model_transform_matrix)){
                ren.draw_shaded_mesh(last_mesh, textured_material, model_transform_matrix);
            }
            
            

            ren.set_zbuffer_write(false);
            occluder.set_zbuffer_write(false);

            

            
            //draw ghost
            
            if(occluder.check_mesh(last_mesh, ghost_matrix)){
                ren.draw_shaded_mesh(last_mesh, transparent_material, ghost_matrix);
            }
            
            ren.set_zbuffer_write(false);
            occluder.set_zbuffer_write(false);

            ren.set_deeph_mode(TSRPA::DeephMode::NONE);
            //draw smaller model to test occlusion
            
            if(occluder.check_mesh(last_mesh, occlude_matrix)){
                ren.draw_shaded_mesh(last_mesh, textured_material, occlude_matrix);
            }
            
            
            
        }

        if (fps_display_timer >= 1.0)
        {
            
            fps_text = std::string("FPS: ") + std::to_string(fps_frames_passed) + "\n";
            if(!last_mesh.is_valid() && !last_texture.is_valid()){
                fps_text = "drop .obj or .png file here\n";
            }

            fps_display_timer = 0;
            fps_frames_passed = 0;
        }
        print_mesage(render, font, fps_text);

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