#include <iostream>
#include <cmath>
#include <vector>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <glm/glm.hpp>

#define TSRPA_ADD_BASIC_COLOR_PALETTE
#include "tsrpa.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

class ObjMesh : public TSRPA::Mesh
{
public:
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
                    this->vertex.push_back(attrib.vertices[3 * size_t(idx.vertex_index) + 0]);
                    this->vertex.push_back(attrib.vertices[3 * size_t(idx.vertex_index) + 1]);
                    this->vertex.push_back(attrib.vertices[3 * size_t(idx.vertex_index) + 2]);

                    // Check if `normal_index` is zero or positive. negative = no normal data
                    if (idx.normal_index >= 0)
                    {
                        this->normal.push_back(attrib.normals[3 * size_t(idx.normal_index) + 0]);
                        this->normal.push_back(attrib.normals[3 * size_t(idx.normal_index) + 1]);
                        this->normal.push_back(attrib.normals[3 * size_t(idx.normal_index) + 2]);
                    }

                    // Check if `texcoord_index` is zero or positive. negative = no texcoord data
                    if (idx.texcoord_index >= 0)
                    {
                        this->uv.push_back(attrib.texcoords[2 * size_t(idx.texcoord_index) + 0]);
                        this->uv.push_back(attrib.texcoords[2 * size_t(idx.texcoord_index) + 1]);
                    }

                    // Optional: vertex colors
                    this->color.push_back(attrib.colors[3 * size_t(idx.vertex_index) + 0]);
                    this->color.push_back(attrib.colors[3 * size_t(idx.vertex_index) + 1]);
                    this->color.push_back(attrib.colors[3 * size_t(idx.vertex_index) + 2]);
                }
                index_offset += fv;

                // per-face material
                shapes[s].mesh.material_ids[f];
            }
        }
        this->vert_count = this->vertex.size() / 3;
    }
};

class PngTexture : public TSRPA::Texture
{
public:
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
        data = new unsigned char[width * height * 4];

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

void render_texture(TSRPA::Render &ren, TSRPA::Texture &texture)
{
    if(texture.width == 0 || texture.height == 0){return;}
    for (unsigned int x = 0; x < texture.width; x++)
    {
        for (unsigned int y = 0; y < texture.height; y++)
        {
            ren.draw_point(x,y,texture.get_color(x,y));
        }
    }
}

void render_model_with_points(TSRPA::Render &ren, const TSRPA::Mesh &mesh, const TSRPA::Color256 &color)
{
    for (unsigned int i = 0; i < mesh.vert_count; i++)
    {

        float vx = mesh.vertex[(i * 3) + 0];
        float vy = mesh.vertex[(i * 3) + 1];
        ren.draw_point((vx + 1.0) * ren.width / 2.0, ren.height - (vy + 1.0) * ren.height / 2.0, color);
    }
}

void render_model_with_lines(TSRPA::Render &ren, const TSRPA::Mesh &mesh, const TSRPA::Color256 &color)
{

    for (unsigned int i = 0; i < mesh.vert_count; i += 3)
    {

        glm::vec2 va(mesh.vertex[(i * 3) + 0], mesh.vertex[(i * 3) + 1]);
        glm::ivec2 a((va.x + 1.0) * ren.width / 2.0, ren.height - (va.y + 1.0) * ren.height / 2.0);

        glm::vec2 vb(mesh.vertex[(i * 3) + 3], mesh.vertex[(i * 3) + 4]);
        glm::ivec2 b((vb.x + 1.0) * ren.width / 2.0, ren.height - (vb.y + 1.0) * ren.height / 2.0);

        glm::vec2 vc(mesh.vertex[(i * 3) + 6], mesh.vertex[(i * 3) + 7]);
        glm::ivec2 c((vc.x + 1.0) * ren.width / 2.0, ren.height - (vc.y + 1.0) * ren.height / 2.0);

        ren.draw_triangle_wire_frame(a, b, c, color);
    }
}

void render_model_with_basic_triangles(TSRPA::Render &ren, const TSRPA::Mesh &mesh, const TSRPA::Color256 &color)
{

    for (unsigned int i = 0; i < mesh.vert_count; i += 3)
    {

        glm::vec2 va(mesh.vertex[(i * 3) + 0], mesh.vertex[(i * 3) + 1]);
        glm::ivec2 a((va.x + 1.0) * ren.width / 2.0, ren.height - (va.y + 1.0) * ren.height / 2.0);

        glm::vec2 vb(mesh.vertex[(i * 3) + 3], mesh.vertex[(i * 3) + 4]);
        glm::ivec2 b((vb.x + 1.0) * ren.width / 2.0, ren.height - (vb.y + 1.0) * ren.height / 2.0);

        glm::vec2 vc(mesh.vertex[(i * 3) + 6], mesh.vertex[(i * 3) + 7]);
        glm::ivec2 c((vc.x + 1.0) * ren.width / 2.0, ren.height - (vc.y + 1.0) * ren.height / 2.0);

        ren.draw_basic_triangle(a, b, c, color);
    }
}

void render_model_triangles_with_random_colors(TSRPA::Render &ren, const TSRPA::Mesh &mesh)
{

    for (unsigned int i = 0; i < mesh.vert_count; i += 3)
    {

        glm::vec2 va(mesh.vertex[(i * 3) + 0], mesh.vertex[(i * 3) + 1]);
        glm::ivec2 a((va.x + 1.0) * ren.width / 2.0, ren.height - (va.y + 1.0) * ren.height / 2.0);

        glm::vec2 vb(mesh.vertex[(i * 3) + 3], mesh.vertex[(i * 3) + 4]);
        glm::ivec2 b((vb.x + 1.0) * ren.width / 2.0, ren.height - (vb.y + 1.0) * ren.height / 2.0);

        glm::vec2 vc(mesh.vertex[(i * 3) + 6], mesh.vertex[(i * 3) + 7]);
        glm::ivec2 c((vc.x + 1.0) * ren.width / 2.0, ren.height - (vc.y + 1.0) * ren.height / 2.0);

        ren.draw_basic_triangle(a, b, c, TSRPA::Color256(SDL_rand(255), SDL_rand(255), SDL_rand(255), 255));
    }
}

void render_model_triangles_with_light(TSRPA::Render &ren, const TSRPA::Mesh &mesh, const glm::vec3 &light_rit)
{

    for (unsigned int i = 0; i < mesh.vert_count; i += 3)
    {

        glm::vec3 va(mesh.vertex[(i * 3) + 0], mesh.vertex[(i * 3) + 1], mesh.vertex[(i * 3) + 2]);
        glm::ivec2 a((va.x + 1.0) * ren.width / 2.0, ren.height - (va.y + 1.0) * ren.height / 2.0);

        glm::vec3 vb(mesh.vertex[(i * 3) + 3], mesh.vertex[(i * 3) + 4], mesh.vertex[(i * 3) + 5]);
        glm::ivec2 b((vb.x + 1.0) * ren.width / 2.0, ren.height - (vb.y + 1.0) * ren.height / 2.0);

        glm::vec3 vc(mesh.vertex[(i * 3) + 6], mesh.vertex[(i * 3) + 7], mesh.vertex[(i * 3) + 8]);
        glm::ivec2 c((vc.x + 1.0) * ren.width / 2.0, ren.height - (vc.y + 1.0) * ren.height / 2.0);

        glm::vec3 n = glm::normalize(glm::cross(vc - va, vb - va));
        float intensity = glm::dot(n, light_rit);
        if (intensity < 0.0)
        {
            continue;
        }

        ren.draw_basic_triangle(a, b, c, TSRPA::Color256(intensity * 255, intensity * 255, intensity * 255, 255));
    }
}

void render_model_triangles_with_deeph_and_light(TSRPA::Render &ren, const TSRPA::Mesh &mesh, const glm::vec3 &light_rit)
{

    ren.zbuffer->set_deeph_mode(TSRPA::DeephMode::LESS);

    for (unsigned int i = 0; i < mesh.vert_count; i += 3)
    {

        glm::vec3 points[3];

        glm::vec3 va(mesh.vertex[(i * 3) + 0], mesh.vertex[(i * 3) + 1], mesh.vertex[(i * 3) + 2]);
        points[0] = glm::vec3((va.x + 1.0) * ren.width / 2.0, ren.height - (va.y + 1.0) * ren.height / 2.0, va.z);

        glm::vec3 vb(mesh.vertex[(i * 3) + 3], mesh.vertex[(i * 3) + 4], mesh.vertex[(i * 3) + 5]);
        points[1] = glm::vec3((vb.x + 1.0) * ren.width / 2.0, ren.height - (vb.y + 1.0) * ren.height / 2.0, vb.z);

        glm::vec3 vc(mesh.vertex[(i * 3) + 6], mesh.vertex[(i * 3) + 7], mesh.vertex[(i * 3) + 8]);
        points[2] = glm::vec3((vc.x + 1.0) * ren.width / 2.0, ren.height - (vc.y + 1.0) * ren.height / 2.0, vc.z);

        glm::vec3 n = glm::normalize(glm::cross(vc - va, vb - va));
        float intensity = glm::dot(n, light_rit);
        if (intensity < 0.0)
        {
            continue;
        }

        ren.draw_colorfull_triangle(points, TSRPA::Color256(intensity * 255, intensity * 255, intensity * 255, 255));
    }
}

void render_model_triangles_with_deeph_and_texture(TSRPA::Render &ren, const TSRPA::Mesh &mesh, const glm::vec3 &light_rit)
{

    ren.zbuffer->set_deeph_mode(TSRPA::DeephMode::LESS);

    for (unsigned int i = 0; i < mesh.vert_count; i += 3)
    {

        glm::vec3 points[3];

        glm::vec3 va(mesh.vertex[(i * 3) + 0], mesh.vertex[(i * 3) + 1], mesh.vertex[(i * 3) + 2]);
        points[0] = glm::vec3((va.x + 1.0) * ren.width / 2.0, ren.height - (va.y + 1.0) * ren.height / 2.0, va.z);

        glm::vec3 vb(mesh.vertex[(i * 3) + 3], mesh.vertex[(i * 3) + 4], mesh.vertex[(i * 3) + 5]);
        points[1] = glm::vec3((vb.x + 1.0) * ren.width / 2.0, ren.height - (vb.y + 1.0) * ren.height / 2.0, vb.z);

        glm::vec3 vc(mesh.vertex[(i * 3) + 6], mesh.vertex[(i * 3) + 7], mesh.vertex[(i * 3) + 8]);
        points[2] = glm::vec3((vc.x + 1.0) * ren.width / 2.0, ren.height - (vc.y + 1.0) * ren.height / 2.0, vc.z);

        glm::vec3 n = glm::normalize(glm::cross(vc - va, vb - va));
        float intensity = glm::dot(n, light_rit);
        if (intensity < 0.0)
        {
            continue;
        }

        ren.draw_textured_triangle(points, TSRPA::Color256(intensity * 255, intensity * 255, intensity * 255, 255));
    }
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

    TSRPA::Render ren(1024, 1024);
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

                ren.frame_buffer->clear_color = TSRPA::Palette::BLACK;
                ren.clear();

                

                // render_model_with_basic_triangles(ren, mesh, TSRPA::Palette::WHITE);
                // render_model_with_lines(ren, mesh, TSRPA::Palette::GREEN);
                // render_model_with_points(ren, mesh, TSRPA::Palette::RED);
                // render_model_triangles_with_random_colors(ren, mesh);
                // render_model_triangles_with_light(ren, mesh,glm::vec3(0,0,-1));
                ObjMesh mesh(event.drop.data);
                render_model_triangles_with_deeph_and_texture(ren, mesh, glm::vec3(0, 0, -1));

                PngTexture texture(event.drop.data);
                render_texture(ren, texture);

                break;
            }
        }

        // Do game logic, present a frame, etc.
        //SDL_RenderClear(render);
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