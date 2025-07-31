#include <iostream>
#include <cmath>
#include <vector>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define TSRPA_ADD_BASIC_COLOR_PALETTE
#include "tsrpa.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

class PngTexture : public TSRPA::Texture
{
public:
    PngTexture() : TSRPA::Texture() {}
    PngTexture(const char *path) : TSRPA::Texture()
    {
        // printf("loading: %s\n", path);
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

class ObjMesh : public TSRPA::Mesh
{
public:
    ObjMesh() : TSRPA::Mesh() {}
    ObjMesh(const char *path) : TSRPA::Mesh()
    {
        printf("loading: %s\n", path);

        std::string inputfile = path;
        tinyobj::ObjReaderConfig reader_config;
        reader_config.triangulate = true;

        tinyobj::ObjReader reader;

        if (!reader.ParseFromFile(inputfile, reader_config))
        {
            if (!reader.Error().empty())
            {
                std::cerr << "TinyObjReader: " << reader.Error();
            }
            return;
        }

        const tinyobj::attrib_t &attrib = reader.GetAttrib();
        const std::vector<tinyobj::shape_t> &shapes = reader.GetShapes();
        const std::vector<tinyobj::material_t> &materials = reader.GetMaterials();

        if (materials.size() > 0)
        {
            printf("%s\n", materials[0].alpha_texname.c_str());
        }

        for (size_t s = 0; s < shapes.size(); s++)
        {
            // Loop over faces(polygon)
            size_t index_offset = 0;
            for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++)
            {
                size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);

                // Loop over vertices in the face.
                for (size_t v = 0; v < fv; v++)
                {
                    // access to vertex
                    tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
                    this->vertex.push_back(glm::vec3(
                        attrib.vertices[3 * size_t(idx.vertex_index) + 0],
                        attrib.vertices[3 * size_t(idx.vertex_index) + 1],
                        attrib.vertices[3 * size_t(idx.vertex_index) + 2]));

                    // Check if `normal_index` is zero or positive. negative = no normal data
                    if (idx.normal_index >= 0)
                    {
                        this->normal.push_back(glm::vec3(
                            attrib.normals[3 * size_t(idx.normal_index) + 0],
                            attrib.normals[3 * size_t(idx.normal_index) + 1],
                            attrib.normals[3 * size_t(idx.normal_index) + 2]));
                    }

                    // Check if `texcoord_index` is zero or positive. negative = no texcoord data
                    if (idx.texcoord_index >= 0)
                    {
                        this->uv.push_back(glm::vec2(
                            attrib.texcoords[2 * size_t(idx.texcoord_index) + 0],
                            attrib.texcoords[2 * size_t(idx.texcoord_index) + 1]
                        ));
                    }

                    // Optional: vertex colors
                    this->color.push_back(glm::vec3(
                        attrib.colors[3 * size_t(idx.vertex_index) + 0],
                        attrib.colors[3 * size_t(idx.vertex_index) + 1],
                        attrib.colors[3 * size_t(idx.vertex_index) + 2]
                    ));
                }
                index_offset += fv;

                // per-face material
                shapes[s].mesh.material_ids[f];
            }
        }
        this->vert_count = this->vertex.size();
        this->face_count = this->vertex.size() / 3;
    }
};

ObjMesh last_mesh;
PngTexture last_texture;

void render_texture(TSRPA::Renderer &ren, TSRPA::Texture &texture)
{
    if (texture.width == 0 || texture.height == 0)
    {
        return;
    }
    for (unsigned int x = 0; x < texture.width; x++)
    {
        for (unsigned int y = 0; y < texture.height; y++)
        {
            ren.draw_point(x, y, texture.get_color(x, y));
        }
    }
}

glm::mat4 model_transform_matrix;
void render_model_triangles_with_deeph_and_light(TSRPA::Renderer &ren, const TSRPA::Mesh &mesh, const glm::vec3 &light_rit)
{

    ren.zbuffer->set_deeph_mode(TSRPA::DeephMode::LESS);

    for (unsigned int i = 0; i < mesh.vert_count; i += 3)
    {

        glm::vec3 points[3];

        points[0] = ren.calculate_screen_position(mesh.vertex[i], model_transform_matrix);
        points[1] = ren.calculate_screen_position(mesh.vertex[i+1], model_transform_matrix);
        points[2] = ren.calculate_screen_position(mesh.vertex[i+2], model_transform_matrix);

        glm::vec3 n = glm::normalize(glm::cross(points[2] - points[0], points[1] - points[0]));
        float intensity = glm::dot(n, light_rit);
        if (intensity < 0.0)
        {
            continue;
        }

        ren.draw_colorfull_triangle(points, glm::ivec4(intensity * 255, intensity * 255, intensity * 255, 255));
    }
}

void render_model_triangles_with_deeph_and_texture(TSRPA::Renderer &ren, TSRPA::Mesh &mesh, TSRPA::Texture &texture)
{

    for (unsigned int i = 0; i < mesh.face_count; i++)
    {

        /*

        glm::vec3 points[3];

        points[0] = ren.calculate_screen_position(mesh.vertex[i], model_transform_matrix);
        points[1] = ren.calculate_screen_position(mesh.vertex[i+1], model_transform_matrix);
        points[2] = ren.calculate_screen_position(mesh.vertex[i+2], model_transform_matrix);

        glm::vec2 uv[3];
        uv[0] = mesh.uv[i];
        uv[1] = mesh.uv[i+1];
        uv[2] = mesh.uv[i+2];
        
        ren.draw_textured_triangle(points, uv, TSRPA::Palette::WHITE, texture);
        */
       TSRPA::Material material;
       ren.draw_shaded_triangle(mesh,i,material,model_transform_matrix);
    }
}

void print_mesage(SDL_Renderer *render, TTF_Font *font, const std::string &text)
{
    SDL_Color white = {255, 255, 255, 255};
    const char *message = text.c_str();
    SDL_Surface *surfaceMessage = TTF_RenderText_Solid(font, message, sizeof(message), white);
    SDL_Texture *Message = SDL_CreateTextureFromSurface(render, surfaceMessage);
    SDL_SetTextureScaleMode(Message, SDL_SCALEMODE_NEAREST);
    SDL_FRect Message_rect;
    Message_rect.x = 0;
    Message_rect.y = 0;
    Message_rect.w = 256;
    Message_rect.h = 64;
    SDL_RenderTexture(render, Message, NULL, &Message_rect);
    SDL_DestroySurface(surfaceMessage);
    SDL_DestroyTexture(Message);
}

int main(int argc, char *argv[])
{

    SDL_Window *window; // Declare a pointer
    bool done = false;

    SDL_Init(SDL_INIT_VIDEO); // Initialize SDL3
    SDL_srand(42);

    if (TTF_Init() == -1)
    {
        std::cerr << "TTF_Init failed: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Create an application window with the following settings:
    window = SDL_CreateWindow(
        "Hello SDL3 render atempt",                   // window title
        1024,                                         // width, in pixels
        1024,                                         // height, in pixels
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

    TSRPA::Renderer ren(1024, 1024);
    ren.frame_buffer->clear_color = TSRPA::Palette::INVISIBLE;
    ren.clear();

    ren.draw_point(0, 0, TSRPA::Palette::GREEN);
    ren.draw_point(255, 0, TSRPA::Palette::GREEN);
    ren.draw_point(0, 255, TSRPA::Palette::GREEN);
    ren.draw_point(255, 255, TSRPA::Palette::GREEN);

    // ren.draw_line(glm::vec2(32, 32), glm::vec2(128, 128), TSRPA::Palette::RED);
    ren.draw_line(glm::ivec2(0, 0), glm::vec2(0, 500), TSRPA::Palette::RED);
    ren.draw_line(glm::ivec2(0, 0), glm::vec2(500, 0), TSRPA::Palette::RED);
    ren.draw_line(glm::ivec2(0, 0), glm::vec2(500, 500), TSRPA::Palette::RED);

    const unsigned int pich = ren.width * 4;

    SDL_Surface *surface = SDL_CreateSurfaceFrom(ren.width, ren.height, SDL_PIXELFORMAT_RGBA32, (void *)ren.get_result(), pich);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(render, surface);
    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
    SDL_DestroySurface(surface);

    glm::vec3 model_pos(0.0f, 0.0f, 5.0f);
    model_transform_matrix = glm::translate(glm::mat4(1.0f), model_pos);

    ren.view_matrix = glm::lookAt(
        glm::vec3(0.0f, 0.0f, 0.0f),
        model_pos,
        glm::vec3(0.0f, 1.0f, 0.0f));

    // Matriz de Projeção
    ren.projection_matrix = glm::perspective(
        glm::radians(45.0f),
        float(ren.width) / float(ren.height),
        0.1f,
        100.0f);

    unsigned int lastTime = 0, currentTime;
    double delta_time;

    ren.zbuffer->set_deeph_mode(TSRPA::DeephMode::LESS);

    std::string font_path = std::string(SDL_GetBasePath()) + "AlienCyborg.ttf";

    TTF_Font *font = TTF_OpenFont(font_path.c_str(), 24);
    if (font == NULL)
    {
        printf("error file AlienCyborg.ttf not found\n");
        return 0;
    }

    float fps_display_timer = 0.0f;
    unsigned int fps_frames_passed = 0;
    std::string fps_text = "hello world";

    while (!done)
    {

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

                ren.frame_buffer->clear_color = TSRPA::Palette::BLACK;
                ren.clear();

                ObjMesh new_mesh(event.drop.data);
                if (new_mesh.is_valid())
                {
                    last_mesh = new_mesh;
                    render_model_triangles_with_deeph_and_light(ren, last_mesh, glm::vec3(0, 0, -1));
                }

                PngTexture new_texture(event.drop.data);
                if (new_texture.is_valid())
                {
                    last_texture = new_texture;

                    if (last_mesh.is_valid() && last_texture.is_valid())
                    {

                        render_model_triangles_with_deeph_and_texture(ren, last_mesh, last_texture);
                    }
                    else
                    {
                        render_texture(ren, last_texture);
                    }
                }

                break;
            }
        }

        if (last_mesh.is_valid() && last_texture.is_valid())
        {
            ren.frame_buffer->clear_color = TSRPA::Palette::INVISIBLE;
            ren.clear();
            model_transform_matrix = glm::rotate(model_transform_matrix, (float)(glm::radians(90.0f) * delta_time), glm::vec3(0.0f, 1.0f, 0.0f));
            render_model_triangles_with_deeph_and_texture(ren, last_mesh, last_texture);
        }

        // Do game logic, present a frame, etc.
        SDL_RenderClear(render);
        SDL_UpdateTexture(texture, NULL, (void *)ren.get_result(), pich);
        SDL_RenderTexture(render, texture, NULL, NULL);

        if (fps_display_timer >= 1.0)
        {
            fps_text = std::string("FPS: ") + std::to_string(fps_frames_passed) + "\n";
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