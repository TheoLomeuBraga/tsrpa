#pragma once

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include "tsrpa.h"

class ObjMesh : public TSRPA::Mesh
{
public:
    ObjMesh() : TSRPA::Mesh() {}
    ObjMesh(const char *path) : TSRPA::Mesh()
    {
        SDL_Log("loading: %s\n", path);

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
            SDL_Log("%s\n", materials[0].alpha_texname.c_str());
        }

        for (size_t s = 0; s < shapes.size(); s++)
        {
            
            size_t index_offset = 0;
            for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++)
            {
                size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);

                
                for (size_t v = 0; v < fv; v++)
                {
                    
                    tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
                    this->vertex.push_back(glm::vec3(
                        attrib.vertices[3 * size_t(idx.vertex_index) + 0],
                        attrib.vertices[3 * size_t(idx.vertex_index) + 1],
                        attrib.vertices[3 * size_t(idx.vertex_index) + 2]));

                        
                    if (idx.normal_index >= 0)
                    {
                        this->normal.push_back(glm::vec3(
                            attrib.normals[3 * size_t(idx.normal_index) + 0],
                            attrib.normals[3 * size_t(idx.normal_index) + 1],
                            attrib.normals[3 * size_t(idx.normal_index) + 2]));
                    }

                    
                    if (idx.texcoord_index >= 0)
                    {
                        this->uv.push_back(glm::vec2(
                            attrib.texcoords[2 * size_t(idx.texcoord_index) + 0],
                            attrib.texcoords[2 * size_t(idx.texcoord_index) + 1]));
                    }

                    
                    this->color.push_back(glm::vec3(
                        attrib.colors[3 * size_t(idx.vertex_index) + 0],
                        attrib.colors[3 * size_t(idx.vertex_index) + 1],
                        attrib.colors[3 * size_t(idx.vertex_index) + 2]));
                }
                index_offset += fv;

                
                shapes[s].mesh.material_ids[f];
            }
        }
        this->vert_count = this->vertex.size();
        this->face_count = this->vertex.size() / 3;
    }
};