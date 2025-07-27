#include <iostream>
#include <cmath>
#include <vector>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <glm/glm.hpp>

#define TSRPA_ADD_BASIC_COLOR_PALETTE
#include "tsrpa.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

class Mesh
{
public:
    unsigned int vert_count;
    std::vector<float> vertex;
    std::vector<float> uv;
    std::vector<float> normal;
    std::vector<float> color;
    Mesh() {}
    Mesh(std::vector<float> vertex)
    {
        this->vertex = vertex;
    }
    Mesh(std::vector<float> vertex, std::vector<float> uv)
    {
        this->vertex = vertex;
        this->uv = uv;
    }
    Mesh(std::vector<unsigned int> index, std::vector<float> vertex, std::vector<float> uv, std::vector<float> normal)
    {
        this->vertex = vertex;
        this->uv = uv;
        this->normal = normal;
        this->color = normal;
    }
};

class ObjMesh : public Mesh
{
public:
    ObjMesh(const char *path) : Mesh()
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
                    this->color.push_back(attrib.colors[3*size_t(idx.vertex_index)+0]);
                    this->color.push_back(attrib.colors[3*size_t(idx.vertex_index)+1]);
                    this->color.push_back(attrib.colors[3*size_t(idx.vertex_index)+2]);

                }
                index_offset += fv;

                // per-face material
                shapes[s].mesh.material_ids[f];
            }
        }
        this->vert_count = this->vertex.size() / 3;
    }
};

void render_model_with_points(TSRPA::Render &ren, const Mesh &mesh,const TSRPA::Color256 &color){
    for(unsigned int i = 0; i < mesh.vert_count;i++){
        
        float vx = mesh.vertex[(i * 3) + 0];
        float vy = mesh.vertex[(i * 3) + 1];
        ren.draw_point((vx + 1.0) * ren.width / 2.0, ren.height - (vy + 1.0) * ren.height / 2.0,color);
    }
}

void render_triangle_with_lines(TSRPA::Render &ren,const glm::vec3 &a,const glm::vec3 &b,const glm::vec3 &c,const TSRPA::Color256 &color){
    glm::vec2 va((a.x + 1.0) * ren.width / 2.0, ren.height - (a.y + 1.0) * ren.height / 2.0);
    glm::vec2 vb((b.x + 1.0) * ren.width / 2.0, ren.height - (b.y + 1.0) * ren.height / 2.0);
    glm::vec2 vc((c.x + 1.0) * ren.width / 2.0, ren.height - (c.y + 1.0) * ren.height / 2.0);

    ren.draw_line(va,vb,color);
    ren.draw_line(vb,vc,color);
    ren.draw_line(vc,va,color);
}

void render_model_with_lines(TSRPA::Render &ren, const Mesh &mesh,const TSRPA::Color256 &color){
    
    for(unsigned int i = 0; i < mesh.vert_count;i+=3){
        
        glm::vec3 a(mesh.vertex[(i * 3) + 0],mesh.vertex[(i * 3) + 1],mesh.vertex[(i * 3) + 2]);
        glm::vec3 b(mesh.vertex[(i * 3) + 3],mesh.vertex[(i * 3) + 4],mesh.vertex[(i * 3) + 5]);
        glm::vec3 c(mesh.vertex[(i * 3) + 6],mesh.vertex[(i * 3) + 7],mesh.vertex[(i * 3) + 8]);
        render_triangle_with_lines(ren,a,b,c,color);

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

    TSRPA::Render ren(512, 512);
    ren.frame_buffer->clear_color = TSRPA::Palette::INVISIBLE;
    ren.clear();

    ren.draw_point(0, 0, TSRPA::Palette::GREEN);
    ren.draw_point(255, 0, TSRPA::Palette::GREEN);
    ren.draw_point(0, 255, TSRPA::Palette::GREEN);
    ren.draw_point(255, 255, TSRPA::Palette::GREEN);

    // ren.draw_line(glm::vec2(32, 32), glm::vec2(128, 128), TSRPA::Palette::RED);
    ren.draw_line(glm::vec2(0, 0), glm::vec2(0, 500), TSRPA::Palette::RED);
    ren.draw_line(glm::vec2(0, 0), glm::vec2(500, 0), TSRPA::Palette::RED);
    ren.draw_line(glm::vec2(0, 0), glm::vec2(500, 500), TSRPA::Palette::RED);

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

                ObjMesh mesh(event.drop.data);

                //render_model_with_points(ren, event.drop.data);
                render_model_with_lines(ren,mesh,TSRPA::Palette::GREEN);
                render_model_with_points(ren,mesh,TSRPA::Palette::RED);

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