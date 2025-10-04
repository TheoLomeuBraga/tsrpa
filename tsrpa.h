#pragma once
#include <iostream>
#include <cmath>
#include <functional>
#include <glm/glm.hpp>
#include <vector>

#ifdef TSRPA_MULT_THREAD_RENDERER
#include <thread>
#include <mutex>
#include <chrono>
#endif

namespace TSRPA
{

#ifdef TSRPA_ADD_BASIC_COLOR_PALETTE

    namespace Palette
    {
        const glm::ivec4 INVISIBLE(0, 0, 0, 0);
        const glm::ivec4 BLACK(0, 0, 0, 255);
        const glm::ivec4 WHITE(255, 255, 255, 255);
        const glm::ivec4 RED(255, 0, 0, 255);
        const glm::ivec4 GREEN(0, 255, 0, 255);
        const glm::ivec4 BLUE(0, 0, 255, 255);
    };

#endif

    unsigned char to_uchar_value(float value)
    {
        return (unsigned char)std::round(value * 255.0);
    }

    glm::ivec4 create_color(float r, float g, float b, float a)
    {
        return glm::ivec4(to_uchar_value(r), to_uchar_value(g), to_uchar_value(b), to_uchar_value(a));
    }

    class Texture
    {
    public:
        unsigned int width = 0;
        unsigned int height = 0;
        std::vector<unsigned char> data;
        Texture() {}
        Texture(unsigned int width, unsigned int height)
        {
            this->width = width;
            this->height = height;
            this->data.resize(width * height * 4);
        }
        Texture(unsigned int width, unsigned int height, std::vector<unsigned char> data)
        {
            this->width = width;
            this->height = height;
            this->data = data;
        }

        bool is_valid() { return width > 0 && height > 0 && data.size() == width * height * 4; }

        glm::ivec4 get_color(const unsigned int &x, const unsigned int &y)
        {
            if (!is_valid())
            {
                return glm::ivec4(255, 255, 255, 255);
            }
            unsigned int i = ((y % height) * width + (x % width)) * 4;
            return glm::ivec4(data[i], data[i + 1], data[i + 2], data[i + 3]);
        }

        glm::vec4 sample(glm::vec2 uv)
        {
            return (glm::vec4)(get_color(uv.x * width, height - (uv.y * height))) / glm::vec4(255.0, 255.0, 255.0, 255.0);
        }

        void set_color(const unsigned int &x, const unsigned int &y, const glm::ivec4 &color)
        {
            unsigned int i = (y * width + x) * 4;
            data[i] = color.r;
            data[i + 1] = color.g;
            data[i + 2] = color.b;
            data[i + 3] = color.a;
        }
    };

    struct ShaderFunctionData
    {
        glm::vec4 position = glm::vec4(0.0, 0.0, 0.0, 1.0);
        glm::vec2 uv = glm::vec2(0.0, 0.0);
        glm::vec2 uv2 = glm::vec2(0.0, 0.0);
        glm::vec3 normal = glm::vec3(0.0, 0.0, 0.0);
        glm::vec3 color = glm::vec3(0.0, 0.0, 0.0);

        int bone_index[4] = {0, 0, 0, 0};
        float bone_weight[4] = {0.0, 0.0, 0.0, 0.0};
        glm::mat4 finalBonesMatrices[4];
    };

    class Material
    {
    public:
        virtual void vertex_shader(ShaderFunctionData &data, const glm::mat4 &projection, const glm::mat4 &view, const glm::mat4 &model, const glm::mat3 &normal_matrix)
        {
            data.normal = glm::normalize(normal_matrix * data.normal);
            data.position = (projection * view * model) * data.position;
        }

        virtual glm::vec4 fragment_shader(ShaderFunctionData &data)
        {
            return glm::vec4(1.0, 1.0, 1.0, 1.0);
        }

        Material() {}
    };

    class MeshBase
    {
    public:
        unsigned int vert_count;
        unsigned int face_count;

        MeshBase() {}

        virtual bool is_valid() { return vert_count > 0; }

        virtual void get_vertex_data(ShaderFunctionData &data, const unsigned int &id)
        {
        }
    };

    class Mesh : public MeshBase
    {
    public:
        std::vector<glm::vec3> vertex;
        std::vector<glm::vec2> uv;
        std::vector<glm::vec2> uv2;
        std::vector<glm::vec3> normal;
        std::vector<glm::vec3> color;
        std::vector<int> material_idx;

        Mesh() : MeshBase() {}

        void get_vertex_data(ShaderFunctionData &data, const unsigned int &id)
        {
            data.position = glm::vec4(vertex[id], 1.0);
            if (uv.size() > 0)
            {
                data.uv = uv[id];
            }
            if (uv2.size() > 0)
            {
                data.uv2 = uv2[id];
            }
            if (normal.size() > 0)
            {
                data.normal = normal[id];
            }
            if (color.size() > 0)
            {
                data.color = color[id];
            }
        }
    };

    enum DeephMode
    {
        NONE = 0,
        LESS = 1,
        GREATER = 2,
    };

    class OcclusionDetector
    {
    protected:
        unsigned int width;
        unsigned int height;
        std::vector<float> zbuffer;
        glm::mat4 view_matrix;
        glm::mat4 projection_matrix;
        bool zbuffer_write = true;
        std::function<bool(unsigned int, float)> deep_check_func;
        DeephMode deeph_mode;

        bool deep_check_none(unsigned int idx, float value) { return true; }
        bool deep_check_less(unsigned int idx, float value)
        {

            if (zbuffer[idx] < value)
            {
                if (zbuffer_write)
                {
                    zbuffer[idx] = value;
                }

                return true;
            }
            return false;
        }
        bool deep_check_greater(unsigned int idx, float value)
        {
            if (zbuffer[idx] > value)
            {
                if (zbuffer_write)
                {
                    zbuffer[idx] = value;
                }
                return true;
            }
            return false;
        }
        bool calculate_deep_check(unsigned int idx, float value)
        {
            return deep_check_func(idx, value);
        }

        glm::vec3 calculate_screen_position(const glm::vec3 &vertex, const glm::mat4 &model_transform_matrix)
        {

            glm::mat4 mvp = projection_matrix * view_matrix * model_transform_matrix;

            glm::vec4 clip_space_pos = mvp * glm::vec4(vertex, 1.0);

            glm::vec3 space_pos;
            space_pos.x = clip_space_pos.x / clip_space_pos.w;
            space_pos.y = clip_space_pos.y / clip_space_pos.w;
            space_pos.z = clip_space_pos.z / clip_space_pos.w;

            glm::vec3 screenSpacePos;
            screenSpacePos.x = (space_pos.x + 1.0f) * 0.5f * width;
            screenSpacePos.y = (1.0f - space_pos.y) * 0.5f * height;
            screenSpacePos.z = 1.0f - space_pos.z;

            return screenSpacePos;
        }

        glm::vec3 calculate_screen_position_from_point(const glm::vec4 &pos)
        {

            glm::vec4 clip_space_pos = pos;

            glm::vec3 space_pos;
            space_pos.x = clip_space_pos.x / clip_space_pos.w;
            space_pos.y = clip_space_pos.y / clip_space_pos.w;
            space_pos.z = clip_space_pos.z / clip_space_pos.w;

            glm::vec3 screenSpacePos;
            screenSpacePos.x = (space_pos.x + 1.0f) * 0.5f * width;
            screenSpacePos.y = (1.0f - space_pos.y) * 0.5f * height;
            screenSpacePos.z = 1.0f - space_pos.z;

            return screenSpacePos;
        }

        glm::vec3 barycentric(const glm::vec3 *pts, const glm::vec3 &P)
        {
            glm::vec3 u = glm::cross(glm::vec3(pts[2][0] - pts[0][0], pts[1][0] - pts[0][0], pts[0][0] - P[0]), glm::vec3(pts[2][1] - pts[0][1], pts[1][1] - pts[0][1], pts[0][1] - P[1]));
            if (std::abs(u.z) < 1)
            {
                return glm::vec3(-1, 1, 1);
            }
            return glm::vec3(1.f - (u.x + u.y) / u.z, u.y / u.z, u.x / u.z);
        }

        void vertex_shader(ShaderFunctionData &data, const glm::mat4 &projection, const glm::mat4 &view, const glm::mat4 &model, const glm::mat3 &normal_matrix)
        {
            data.position = (projection * view * model) * data.position;
        }

        bool check_triangle(MeshBase &mesh, const unsigned int face_id, const glm::mat4 &transform, const glm::mat3 &normal_matrix)
        {
            bool ret = false;
            ShaderFunctionData vertex_data[3];

            for (int i = 0; i < 3; i++)
            {

                mesh.get_vertex_data(vertex_data[i], (face_id * 3) + i);
            }

            glm::ivec2 bboxmin(width - 1, height - 1);
            glm::ivec2 bboxmax(0, 0);
            glm::ivec2 clamp(width - 1, height - 1);
            glm::vec3 points[3];

            for (int i = 0; i < 3; i++)
            {
                vertex_shader(vertex_data[i], projection_matrix, view_matrix, transform, normal_matrix);
                points[i] = calculate_screen_position_from_point(vertex_data[i].position);

                bboxmin.x = std::max(0, (int)std::min(bboxmin.x, (int)points[i].x));
                bboxmin.y = std::max(0, (int)std::min(bboxmin.y, (int)points[i].y));

                bboxmax.x = std::min(clamp.x, std::max(bboxmax.x, (int)points[i].x));
                bboxmax.y = std::min(clamp.y, std::max(bboxmax.y, (int)points[i].y));
            }
            glm::vec3 P;
            for (P.x = bboxmin.x; P.x <= bboxmax.x; P.x++)
            {
                for (P.y = bboxmin.y; P.y <= bboxmax.y; P.y++)
                {
                    glm::vec3 bc_screen = barycentric(points, P);
                    if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0)
                    {
                        continue;
                    }

                    P.z = 0;
                    for (int i = 0; i < 3; i++)
                    {
                        P.z += points[i][2] * bc_screen[i];
                    }
                    if (calculate_deep_check(int(P.x + P.y * width), P.z))
                    {
                        ret = true;
                        if (!zbuffer_write)
                        {
                            return true;
                        }
                    }
                }
            }
            return ret;
        }

    public:
        OcclusionDetector() {}
        OcclusionDetector(unsigned int width, unsigned int height)
        {
            this->width = width;
            this->height = height;

            zbuffer.resize(this->width * this->height);
        }

        DeephMode get_deeph_mode() { return deeph_mode; }
        void set_deeph_mode(DeephMode mode)
        {
            deeph_mode = mode;
            switch (mode)
            {
            case 0:
                deep_check_func = std::bind(&OcclusionDetector::deep_check_none, this, std::placeholders::_1, std::placeholders::_2);
                break;
            case 1:
                deep_check_func = std::bind(&OcclusionDetector::deep_check_less, this, std::placeholders::_1, std::placeholders::_2);
                break;
            case 2:
                deep_check_func = std::bind(&OcclusionDetector::deep_check_greater, this, std::placeholders::_1, std::placeholders::_2);
                break;
            }
        }

        bool get_zbuffer_write() { return zbuffer_write; }
        void set_zbuffer_write(bool on) { zbuffer_write = on; }

        glm::mat4 get_view_matrix() { return view_matrix; }
        void set_view_matrix(glm::mat4 mat) { view_matrix = mat; }

        glm::mat4 get_projection_matrix() { return projection_matrix; }
        void set_projection_matrix(glm::mat4 mat) { projection_matrix = mat; }

        void clear()
        {
            for (unsigned int i = 0; i < zbuffer.size(); i++)
            {
                zbuffer[i] = 0;
            }
        }

        bool check_mesh(MeshBase &mesh, glm::mat4 &transform)
        {
            bool ret = false;
            glm::mat3 normal_matrix = glm::transpose(glm::inverse(glm::mat3(transform)));
            for (unsigned int i = 0; i < mesh.face_count; i++)
            {
                if (check_triangle(mesh, i, transform, normal_matrix))
                {
                    ret = true;
                    if (!zbuffer_write)
                    {
                        return true;
                    }
                }
            }
            return ret;
        }
    };

    enum ShowFaces
    {
        BOTH = 0,
        FRONT = 1,
        BACK = 2
    };



    class Renderer{
    protected:
        virtual bool deep_check_none(unsigned int idx, float value) {return false;}
        virtual bool deep_check_less(unsigned int idx, float value){return false;}

        virtual bool deep_check_greater(unsigned int idx, float value){return false;}

        virtual bool calculate_deep_check(unsigned int idx, float value){return false;}

        virtual void clear_zbuffer(){}

        virtual glm::vec3 calculate_screen_position(const glm::vec3 &vertex, const glm::mat4 &model_transform_matrix){return glm::vec3(0.0f);}

        virtual glm::vec3 calculate_screen_position_from_point(const glm::vec4 &pos){return glm::vec4(0.0f);}

        virtual glm::vec3 barycentric(const glm::vec3 *pts, const glm::vec3 &P){return glm::vec3(0.0f);}

        virtual void draw_shaded_triangle(MeshBase &mesh, const unsigned int face_id, Material &material, const glm::mat4 &transform, const glm::mat3 &normal_matrix){}

        virtual glm::ivec4 frame_buffer_get_color(const unsigned int &x, const unsigned int &y){return glm::ivec4(0.0f);}

    public:
        virtual glm::ivec4 get_clear_color() {return glm::ivec4(0.0f);}
        virtual void set_clear_color(glm::ivec4 color) {}

        virtual ShowFaces get_face_mode() {return ShowFaces::BACK;}
        virtual void set_face_mode(ShowFaces mode) {}

        virtual glm::mat4 get_view_matrix() {return glm::mat4(0.0f);}
        virtual void set_view_matrix(const glm::mat4 &mat) {}

        virtual glm::mat4 get_projection_matrix() {return glm::mat4(0.0f);}
        virtual void set_projection_matrix(const glm::mat4 &mat) {}

        virtual unsigned int get_width() {return 0;}
        virtual unsigned int get_height() {return 0;}

        virtual bool get_zbuffer_write() {return false;}
        virtual void set_zbuffer_write(bool on) {}

        virtual DeephMode get_deeph_mode() {return DeephMode::NONE;}
        virtual void set_deeph_mode(DeephMode mode){}

        Renderer() {}

        virtual unsigned char *get_result(){return NULL;}

        virtual void clear_frame_buffer(){}

        virtual void clear(){}

        virtual void draw_point(const unsigned int &x, const unsigned int &y, const glm::ivec4 &color){}

        virtual void draw_texture(TSRPA::Texture &texture, const glm::ivec2 &offset){}

        virtual void draw_line(glm::ivec2 a, glm::ivec2 b, const glm::ivec4 &color){}

        virtual void draw_triangle_wire_frame(const glm::ivec2 &a, const glm::ivec2 &b, const glm::ivec2 &c, const glm::ivec4 &color){}

        virtual void draw_basic_triangle(glm::ivec2 a, glm::ivec2 b, glm::ivec2 c, const glm::ivec4 &color){}

        virtual void draw_shaded_mesh(MeshBase &mesh, Material &material, glm::mat4 &transform){}
    };

    class SingleThreadRenderer : public Renderer
    {
    protected:
        std::vector<unsigned char> frame_buffer;

        glm::ivec4 clear_color;
        ShowFaces face_mode;
        glm::mat4 view_matrix;
        glm::mat4 projection_matrix;

        unsigned int width;
        unsigned int height;
        unsigned int data_size;

        DeephMode deeph_mode;
        std::function<bool(unsigned int, float)> deep_check_func;
        std::vector<float> zbuffer;
        bool zbuffer_write = true;

        bool deep_check_none(unsigned int idx, float value) { return true; }
        bool deep_check_less(unsigned int idx, float value)
        {

            if (zbuffer[idx] < value)
            {
                if (zbuffer_write)
                {
                    zbuffer[idx] = value;
                }

                return true;
            }
            return false;
        }
        bool deep_check_greater(unsigned int idx, float value)
        {
            if (zbuffer[idx] > value)
            {
                if (zbuffer_write)
                {
                    zbuffer[idx] = value;
                }
                return true;
            }
            return false;
        }
        bool calculate_deep_check(unsigned int idx, float value)
        {
            return deep_check_func(idx, value);
        }
        void clear_zbuffer()
        {
            for (unsigned int i = 0; i < zbuffer.size(); i++)
            {
                zbuffer[i] = 0;
            }
        }

        glm::vec3 calculate_screen_position(const glm::vec3 &vertex, const glm::mat4 &model_transform_matrix)
        {

            glm::mat4 mvp = projection_matrix * view_matrix * model_transform_matrix;

            glm::vec4 clip_space_pos = mvp * glm::vec4(vertex, 1.0);

            glm::vec3 space_pos;
            space_pos.x = clip_space_pos.x / clip_space_pos.w;
            space_pos.y = clip_space_pos.y / clip_space_pos.w;
            space_pos.z = clip_space_pos.z / clip_space_pos.w;

            glm::vec3 screenSpacePos;
            screenSpacePos.x = (space_pos.x + 1.0f) * 0.5f * width;
            screenSpacePos.y = (1.0f - space_pos.y) * 0.5f * height;
            screenSpacePos.z = 1.0f - space_pos.z;

            return screenSpacePos;
        }

        glm::vec3 calculate_screen_position_from_point(const glm::vec4 &pos)
        {

            glm::vec4 clip_space_pos = pos;

            glm::vec3 space_pos;
            space_pos.x = clip_space_pos.x / clip_space_pos.w;
            space_pos.y = clip_space_pos.y / clip_space_pos.w;
            space_pos.z = clip_space_pos.z / clip_space_pos.w;

            glm::vec3 screenSpacePos;
            screenSpacePos.x = (space_pos.x + 1.0f) * 0.5f * width;
            screenSpacePos.y = (1.0f - space_pos.y) * 0.5f * height;
            screenSpacePos.z = 1.0f - space_pos.z;

            return screenSpacePos;
        }

        glm::vec3 barycentric(const glm::vec3 *pts, const glm::vec3 &P)
        {
            glm::vec3 u = glm::cross(glm::vec3(pts[2][0] - pts[0][0], pts[1][0] - pts[0][0], pts[0][0] - P[0]), glm::vec3(pts[2][1] - pts[0][1], pts[1][1] - pts[0][1], pts[0][1] - P[1]));
            if (std::abs(u.z) < 1)
            {
                return glm::vec3(-1, 1, 1);
            }
            return glm::vec3(1.f - (u.x + u.y) / u.z, u.y / u.z, u.x / u.z);
        }

        void draw_shaded_triangle(MeshBase &mesh, const unsigned int face_id, Material &material, const glm::mat4 &transform, const glm::mat3 &normal_matrix)
        {

            ShaderFunctionData vertex_data[3];

            for (int i = 0; i < 3; i++)
            {

                mesh.get_vertex_data(vertex_data[i], (face_id * 3) + i);
            }

            if (face_mode == ShowFaces::FRONT)
            {

                glm::vec3 points[3];
                points[0] = transform * vertex_data[0].position;
                points[1] = transform * vertex_data[1].position;
                points[2] = transform * vertex_data[2].position;

                glm::vec3 cameraPosition = glm::inverse(view_matrix)[3];
                glm::vec3 edge1 = points[1] - points[0];
                glm::vec3 edge2 = points[2] - points[0];
                glm::vec3 normal = glm::cross(edge1, edge2);
                glm::vec3 viewDir = cameraPosition - points[0];
                if (glm::dot(normal, viewDir) < 0.0f)
                {
                    return;
                }
            }
            else if (face_mode == ShowFaces::BACK)
            {
                glm::vec3 points[3];
                points[0] = transform * vertex_data[0].position;
                points[1] = transform * vertex_data[1].position;
                points[2] = transform * vertex_data[2].position;

                glm::vec3 cameraPosition = glm::inverse(view_matrix)[3];
                glm::vec3 edge1 = points[1] - points[0];
                glm::vec3 edge2 = points[2] - points[0];
                glm::vec3 normal = glm::cross(edge1, edge2);
                glm::vec3 viewDir = cameraPosition - points[0];
                if (glm::dot(normal, viewDir) > 0.0f)
                {
                    return;
                }
            }

            glm::ivec2 bboxmin(width - 1, height - 1);
            glm::ivec2 bboxmax(0, 0);
            glm::ivec2 clamp(width - 1, height - 1);
            glm::vec3 points[3];

            for (int i = 0; i < 3; i++)
            {
                material.vertex_shader(vertex_data[i], projection_matrix, view_matrix, transform, normal_matrix);
                points[i] = calculate_screen_position_from_point(vertex_data[i].position);

                bboxmin.x = std::max(0, (int)std::min(bboxmin.x, (int)points[i].x));
                bboxmin.y = std::max(0, (int)std::min(bboxmin.y, (int)points[i].y));

                bboxmax.x = std::min(clamp.x, std::max(bboxmax.x, (int)points[i].x));
                bboxmax.y = std::min(clamp.y, std::max(bboxmax.y, (int)points[i].y));
            }
            glm::vec3 P;
            for (P.x = bboxmin.x; P.x <= bboxmax.x; P.x++)
            {
                for (P.y = bboxmin.y; P.y <= bboxmax.y; P.y++)
                {
                    glm::vec3 bc_screen = barycentric(points, P);
                    if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0)
                    {
                        continue;
                    }

                    P.z = 0;
                    for (int i = 0; i < 3; i++)
                    {
                        P.z += points[i][2] * bc_screen[i];
                    }
                    if (SingleThreadRenderer::calculate_deep_check(int(P.x + P.y * width), P.z))
                    {

                        ShaderFunctionData fragment_data;
                        for (int i = 0; i < 3; i++)
                        {
                            fragment_data.position += vertex_data[i].position * bc_screen[i];
                            fragment_data.uv += vertex_data[i].uv * bc_screen[i];
                            fragment_data.uv2 += vertex_data[i].uv2 * bc_screen[i];
                            fragment_data.normal += vertex_data[i].normal * bc_screen[i];
                            fragment_data.color += vertex_data[i].color * bc_screen[i];
                        }
                        fragment_data.normal = glm::normalize(fragment_data.normal);
                        glm::vec4 fragment_color = material.fragment_shader(fragment_data);

                        if (fragment_color.a < 1.0)
                        {
                            glm::vec4 fragment_color_no_alpha = fragment_color;
                            fragment_color_no_alpha.a = 1.0;
                            glm::vec4 framebuffer_color = ((glm::vec4)SingleThreadRenderer::frame_buffer_get_color(P.x, P.y)) / glm::vec4(255.0, 255.0, 255.0, 255.0);
                            SingleThreadRenderer::draw_point(P.x, P.y, glm::mix(framebuffer_color, fragment_color_no_alpha, fragment_color.a) * glm::vec4(255, 255, 255, 255));
                        }
                        else if (fragment_color.a == 0)
                        {
                            continue;
                        }
                        else
                        {
                            SingleThreadRenderer::draw_point(P.x, P.y, fragment_color * glm::vec4(255, 255, 255, 255));
                        }
                    }
                }
            }
        }

        glm::ivec4 frame_buffer_get_color(const unsigned int &x, const unsigned int &y)
        {
            unsigned int i = ((y % height) * width + (x % width)) * 4;
            return glm::ivec4(frame_buffer[i], frame_buffer[i + 1], frame_buffer[i + 2], frame_buffer[i + 3]);
        }

    public:
        glm::ivec4 get_clear_color() { return clear_color; }
        void set_clear_color(glm::ivec4 color) { clear_color = color; }

        ShowFaces get_face_mode() { return face_mode; }
        void set_face_mode(ShowFaces mode) { face_mode = mode; }

        glm::mat4 get_view_matrix() { return view_matrix; }
        void set_view_matrix(const glm::mat4 &mat) { view_matrix = mat; }

        glm::mat4 get_projection_matrix() { return projection_matrix; }
        void set_projection_matrix(const glm::mat4 &mat) { projection_matrix = mat; }

        unsigned int get_width() { return width; }
        unsigned int get_height() { return height; }

        bool get_zbuffer_write() { return zbuffer_write; }
        void set_zbuffer_write(bool on) { zbuffer_write = on; }

        DeephMode get_deeph_mode() { return deeph_mode; }
        void set_deeph_mode(DeephMode mode)
        {
            deeph_mode = mode;
            switch (mode)
            {
            case 0:
                deep_check_func = std::bind(&SingleThreadRenderer::deep_check_none, this, std::placeholders::_1, std::placeholders::_2);
                break;
            case 1:
                deep_check_func = std::bind(&SingleThreadRenderer::deep_check_less, this, std::placeholders::_1, std::placeholders::_2);
                break;
            case 2:
                deep_check_func = std::bind(&SingleThreadRenderer::deep_check_greater, this, std::placeholders::_1, std::placeholders::_2);
                break;
            }
        }

        SingleThreadRenderer(unsigned int width, unsigned int height) : Renderer()
        {
            this->width = width;
            this->height = height;
            this->data_size = this->width * this->height * 4;

            frame_buffer.resize(this->width * this->height * 4);
            zbuffer.resize(this->width * this->height);
        }

        unsigned char *get_result()
        {
            return &frame_buffer[0];
        }

        void clear_frame_buffer()
        {
            for (unsigned int i = 0; i < data_size; i += 4)
            {
                frame_buffer[i + 0] = clear_color.r;
                frame_buffer[i + 1] = clear_color.g;
                frame_buffer[i + 2] = clear_color.b;
                frame_buffer[i + 3] = clear_color.a;
            }
        }

        void clear()
        {
            SingleThreadRenderer::clear_frame_buffer();
            SingleThreadRenderer::clear_zbuffer();
        }

        void draw_point(const unsigned int &x, const unsigned int &y, const glm::ivec4 &color)
        {

            const unsigned int i = (y * width + x) * 4;

            frame_buffer[i] = color.r;
            frame_buffer[i + 1] = color.g;
            frame_buffer[i + 2] = color.b;
            frame_buffer[i + 3] = color.a;
        }

        void draw_texture(TSRPA::Texture &texture, const glm::ivec2 &offset)
        {
            if (!texture.is_valid())
            {
                return;
            }
            for (unsigned int x = 0; x < std::min(texture.width, width + offset.x); x++)
            {
                for (unsigned int y = 0; y < std::min(texture.height, height + offset.y); y++)
                {
                    SingleThreadRenderer::draw_point(x + offset.x, y + offset.y, texture.get_color(x, y));
                }
            }
        }

        void draw_line(glm::ivec2 a, glm::ivec2 b, const glm::ivec4 &color)
        {

            bool steep = false;
            if (std::abs(a.x - b.x) < std::abs(a.y - b.y))
            {
                std::swap(a.x, a.y);
                std::swap(b.x, b.y);
                steep = true;
            }
            if (a.x > b.x)
            {
                std::swap(a.x, b.x);
                std::swap(a.y, b.y);
            }
            int dx = b.x - a.x;
            int dy = b.y - a.y;
            int derror2 = std::abs(dy) * 2;
            int error2 = 0;
            int y = a.y;
            for (int x = a.x; x <= b.x; x++)
            {
                if (steep)
                {
                    SingleThreadRenderer::draw_point(y, x, color);
                }
                else
                {
                    SingleThreadRenderer::draw_point(x, y, color);
                }
                error2 += derror2;
                if (error2 > dx)
                {
                    y += (b.y > a.y ? 1 : -1);
                    error2 -= dx * 2;
                }
            }
        }

        void draw_triangle_wire_frame(const glm::ivec2 &a, const glm::ivec2 &b, const glm::ivec2 &c, const glm::ivec4 &color)
        {
            SingleThreadRenderer::draw_line(a, b, color);
            SingleThreadRenderer::draw_line(b, c, color);
            SingleThreadRenderer::draw_line(c, a, color);
        }

        void draw_basic_triangle(glm::ivec2 a, glm::ivec2 b, glm::ivec2 c, const glm::ivec4 &color)
        {
            if (a.y == b.y && a.y == c.y)
                return;
            if (a.y > b.y)
                std::swap(a, b);
            if (a.y > c.y)
                std::swap(a, c);
            if (b.y > c.y)
                std::swap(b, c);
            int total_height = c.y - a.y;
            for (int i = 0; i < total_height; i++)
            {
                bool second_half = i > b.y - a.y || b.y == a.y;
                int segment_height = second_half ? c.y - b.y : b.y - a.y;
                float alpha = (float)i / total_height;
                float beta = (float)(i - (second_half ? b.y - a.y : 0)) / segment_height;
                glm::ivec2 A(a.x + (c.x - a.x) * alpha, a.y + (c.y - a.y) * alpha);
                glm::ivec2 B = second_half ? glm::ivec2(b.x + (c.x - b.x) * beta, b.y + (c.y - b.y) * beta) : glm::ivec2(a.x + (b.x - a.x) * beta, a.y + (b.y - a.y) * beta);
                if (A.x > B.x)
                    std::swap(A, B);
                for (int j = A.x; j <= B.x; j++)
                {
                    SingleThreadRenderer::draw_point(j, a.y + i, color);
                }
            }
        }

        void draw_shaded_mesh(MeshBase &mesh, Material &material, glm::mat4 &transform)
        {
            glm::mat3 normal_matrix = glm::transpose(glm::inverse(glm::mat3(transform)));
            for (unsigned int i = 0; i < mesh.face_count; i++)
            {
                SingleThreadRenderer::draw_shaded_triangle(mesh, i, material, transform, normal_matrix);
            }
        }
    };

#ifdef TSRPA_MULT_THREAD_RENDERER

    
    
    
    template <typename T>
    class MutexLockedValue
    {
    private:
        std::mutex mtx;
        T value;

    public:
        T get() // const
        {
            std::unique_lock<std::mutex> lock(mtx);
            return value;
        }

        void set(T new_value)
        {
            std::unique_lock<std::mutex> lock(mtx);
            value = new_value;
        }
    };

    class MultThreadRendererTaskList
    {
    private:
        std::mutex mtx;
        std::vector<std::function<void()>> list;

    public:
        void add_task(std::function<void()> task)
        {

            std::unique_lock<std::mutex> lock(mtx);
            list.push_back(task);
        }
        std::function<void()> get_task()
        {

            std::unique_lock<std::mutex> lock(mtx);
            std::function<void()> task = list[0];
            list.erase(list.begin());
            return task;
        }
        bool completed()
        {

            std::unique_lock<std::mutex> lock(mtx);

            return list.size() == 0;
        }

        void do_nothing() {} // this is important SERIOUS
        void wait_for_completion()
        {
            add_task(std::bind(&MultThreadRendererTaskList::do_nothing, this)); // this is for certify that evry instruction was executed on the mult thread

            while (!completed())
            {
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
        }
    };

    
    class MultThreadRenderer : public SingleThreadRenderer
    {
    protected:
        MutexLockedValue<bool> proceed;

        MultThreadRendererTaskList task_list;

        std::thread renderer_thread;

        void loop()
        {
            while (proceed.get())
            {
                std::this_thread::sleep_for(std::chrono::microseconds(10));

                if (task_list.completed())
                {
                    continue;
                }
                task_list.get_task()();
            }
        }

        void start_render_thread()
        {
            proceed.set(true);
            renderer_thread = std::thread(&MultThreadRenderer::loop, this);
        }

        unsigned int safe_width_height[2];

        bool safe_zbuffer_write = true;

        DeephMode safe_deeph_mode;

        glm::ivec4 safe_lear_color;

        ShowFaces safe_face_mode;

        glm::mat4 safe_view_matrix;
        glm::mat4 safe_projection_matrix;

    public:
        bool get_zbuffer_write() override { return safe_zbuffer_write; }
        void base_set_zbuffer_write(bool on)
        {
            SingleThreadRenderer::set_zbuffer_write(on);
        }
        void set_zbuffer_write(bool on) override
        {
            safe_zbuffer_write = on;
            task_list.add_task(std::bind(&MultThreadRenderer::base_set_zbuffer_write, this, on));
        }

        DeephMode get_deeph_mode() override { return safe_deeph_mode; }
        void base_set_deeph_mode(DeephMode mode)
        {
            SingleThreadRenderer::set_deeph_mode(mode);
        }
        void set_deeph_mode(DeephMode mode) override
        {
            safe_deeph_mode = mode;
            task_list.add_task(std::bind(&MultThreadRenderer::base_set_deeph_mode, this, mode));
        }

        unsigned int get_width() override { return safe_width_height[0]; }
        unsigned int get_height() override { return safe_width_height[1]; }

        glm::ivec4 get_clear_color() override { return safe_lear_color; }

        void base_set_clear_color(glm::ivec4 color)
        {
            SingleThreadRenderer::set_clear_color(color);
        }
        void set_clear_color(glm::ivec4 color) override
        {
            safe_lear_color = color;
            task_list.add_task(std::bind(&MultThreadRenderer::base_set_clear_color, this, color));
        }

        ShowFaces get_face_mode() override { return safe_face_mode; }

        void base_set_face_mode(ShowFaces mode)
        {
            SingleThreadRenderer::set_face_mode(mode);
        }
        void set_face_mode(ShowFaces mode) override
        {
            safe_face_mode = mode;
            task_list.add_task(std::bind(&MultThreadRenderer::base_set_face_mode, this, mode));
        }

        glm::mat4 get_view_matrix() override { return safe_view_matrix; }
        void base_set_view_matrix(const glm::mat4 &mat)
        {
            SingleThreadRenderer::set_view_matrix(mat);
        }
        void set_view_matrix(const glm::mat4 &mat) override
        {
            safe_view_matrix = mat;
            task_list.add_task(std::bind(&MultThreadRenderer::base_set_view_matrix, this, mat));
        }

        glm::mat4 get_projection_matrix() override { return safe_projection_matrix; }
        void base_set_projection_matrix(glm::mat4 mat)
        {
            SingleThreadRenderer::set_projection_matrix(mat);
        }
        void set_projection_matrix(const glm::mat4 &mat) override
        {
            safe_projection_matrix = mat;
            task_list.add_task(std::bind(&MultThreadRenderer::base_set_projection_matrix, this, mat));
        }

        void base_clear_zbuffer()
        {
            SingleThreadRenderer::clear_zbuffer();
        }
        void clear_zbuffer() override
        {
            task_list.add_task(std::bind(&MultThreadRenderer::base_clear_zbuffer, this));
        }

        void base_clear_frame_buffer()
        {
            SingleThreadRenderer::clear_frame_buffer();
        }
        void clear_frame_buffer() override
        {
            task_list.add_task(std::bind(&MultThreadRenderer::base_clear_frame_buffer, this));
        }

        void base_clear()
        {
            SingleThreadRenderer::clear();
        }
        void clear() override
        {
            task_list.add_task(std::bind(&MultThreadRenderer::base_clear, this));
        }

        void base_draw_point(const unsigned int &x, const unsigned int &y, const glm::ivec4 &color)
        {
            SingleThreadRenderer::draw_point(x, y, color);
        }
        void draw_point(const unsigned int &x, const unsigned int &y, const glm::ivec4 &color) override
        {
            task_list.add_task(std::bind(&MultThreadRenderer::base_draw_point, this, x, y, color));
        }

        void ptr_draw_texture(TSRPA::Texture *texture, const glm::ivec2 &offset)
        {
            SingleThreadRenderer::draw_texture(*texture, offset);
        }
        void draw_texture(TSRPA::Texture &texture, const glm::ivec2 &offset) override
        {
            task_list.add_task(std::bind(&MultThreadRenderer::ptr_draw_texture, this, &texture, offset));
        }

        void base_draw_line(glm::ivec2 a, glm::ivec2 b, const glm::ivec4 &color)
        {
            SingleThreadRenderer::draw_line(a, b, color);
        }
        void draw_line(glm::ivec2 a, glm::ivec2 b, const glm::ivec4 &color) override
        {
            task_list.add_task(std::bind(&MultThreadRenderer::base_draw_line, this, a, b, color));
        }

        void base_draw_triangle_wire_frame(const glm::ivec2 &a, const glm::ivec2 &b, const glm::ivec2 &c, const glm::ivec4 &color)
        {
            SingleThreadRenderer::draw_triangle_wire_frame(a, b, c, color);
        }
        void draw_triangle_wire_frame(const glm::ivec2 &a, const glm::ivec2 &b, const glm::ivec2 &c, const glm::ivec4 &color) override
        {
            task_list.add_task(std::bind(&MultThreadRenderer::base_draw_triangle_wire_frame, this, a, b, c, color));
        }

        void base_draw_basic_triangle(glm::ivec2 a, glm::ivec2 b, glm::ivec2 c, const glm::ivec4 &color)
        {
            SingleThreadRenderer::draw_basic_triangle(a, b, c, color);
        }
        void draw_basic_triangle(glm::ivec2 a, glm::ivec2 b, glm::ivec2 c, const glm::ivec4 &color) override
        {
            task_list.add_task(std::bind(&MultThreadRenderer::base_draw_basic_triangle, this, a, b, c, color));
        }

        void ptr_draw_shaded_mesh(MeshBase *mesh, Material *material, glm::mat4 &transform)
        {
            SingleThreadRenderer::draw_shaded_mesh(*mesh, *material, transform);
        }
        void draw_shaded_mesh(MeshBase &mesh, Material &material, glm::mat4 &transform) override
        {
            task_list.add_task(std::bind(&MultThreadRenderer::ptr_draw_shaded_mesh, this, &mesh, &material, transform));
        }

        MultThreadRenderer(unsigned int width, unsigned int height) : SingleThreadRenderer(width, height)
        {

            start_render_thread();
            safe_width_height[0] = width;
            safe_width_height[1] = height;
            MultThreadRenderer::set_deeph_mode(DeephMode::NONE);
            MultThreadRenderer::set_zbuffer_write(true);
        }
        ~MultThreadRenderer()
        {
            proceed.set(false);
            renderer_thread.join();
        }

        unsigned char *get_result() override
        {
            task_list.wait_for_completion();
            return &frame_buffer[0];
        }
    };
    
    /*
    
    class MultThreadRenderer : public SingleThreadRenderer
    {
    protected:

        std::vector<SingleThreadRenderer> renderes;

        
        //bool deep_check_none(unsigned int idx, float value) {return false;}
        //bool deep_check_less(unsigned int idx, float value){return false;}

        //bool deep_check_greater(unsigned int idx, float value){return false;}

        //bool calculate_deep_check(unsigned int idx, float value){return false;}

        //void clear_zbuffer(){}

        //void draw_shaded_triangle(MeshBase &mesh, const unsigned int face_id, Material &material, const glm::mat4 &transform, const glm::mat3 &normal_matrix){}

        //glm::ivec4 frame_buffer_get_color(const unsigned int &x, const unsigned int &y){return glm::ivec4(0.0f);}
        

    public:

        MultThreadRenderer(unsigned int width, unsigned int height) : SingleThreadRenderer(width,height) {}

        
        //void set_clear_color(glm::ivec4 color) {}

        //void set_face_mode(ShowFaces mode) {}

        //void set_view_matrix(glm::mat4 mat) {}

        //void set_projection_matrix(glm::mat4 mat) {}

        //void set_zbuffer_write(bool on) {}

        //void set_deeph_mode(DeephMode mode){}

        //void clear_frame_buffer(){}

        //void clear(){}

        //void draw_triangle_wire_frame(const glm::ivec2 &a, const glm::ivec2 &b, const glm::ivec2 &c, const glm::ivec4 &color){}

        //void draw_basic_triangle(glm::ivec2 a, glm::ivec2 b, glm::ivec2 c, const glm::ivec4 &color){}

        //void draw_shaded_mesh(MeshBase &mesh, Material &material, glm::mat4 &transform){}
        
    };
    */

    #endif
};
