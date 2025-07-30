#pragma once
#include <iostream>
#include <cmath>
#include <functional>
#include <glm/glm.hpp>
#include <vector>

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

        void set_color(const unsigned int &x, const unsigned int &y, const glm::ivec4 &color)
        {
            unsigned int i = (y * width + x) * 4;
            data[i] = color.r;
            data[i + 1] = color.g;
            data[i + 2] = color.b;
            data[i + 3] = color.a;
        }
    };

    class Mesh
    {
    public:
        unsigned int vert_count;
        std::vector<float> vertex;
        std::vector<float> uv;
        std::vector<float> normal;
        std::vector<float> color;
        Mesh() {}
        bool is_valid() { return vert_count > 0; }
    };

    class FrameBuffer : public Texture
    {
    public:
        glm::ivec4 clear_color;

        FrameBuffer() : Texture() {}

        FrameBuffer(unsigned int width, unsigned int height) : Texture(width, height) {}

        void clear()
        {
            for (unsigned int i = 0; i < width * height * 4; i += 4)
            {
                data[i] = clear_color.r;
                data[i + 1] = clear_color.g;
                data[i + 2] = clear_color.b;
                data[i + 3] = clear_color.a;
            }
        }

        
    };

    enum DeephMode
    {
        NONE = 0,
        LESS = 1,
        GREATER = 2,
    };

    class ZBuffer
    {

    public:
        unsigned int width;
        unsigned int height;
        std::vector<float> data;

    private:
        DeephMode mode;
        std::function<bool(unsigned int, float)> deep_check_func;

        bool deep_check_none(unsigned int idx, float value) { return true; }
        bool deep_check_less(unsigned int idx, float value)
        {
            if (data[idx] < value)
            {
                data[idx] = value;
                return true;
            }
            return false;
        }
        bool deep_check_greater(unsigned int idx, float value)
        {
            if (data[idx] > value)
            {
                data[idx] = value;
                return true;
            }
            return false;
        }

    public:
        bool calculate_deep_check(unsigned int idx, float value)
        {
            return deep_check_func(idx, value);
        }

        ZBuffer() {}
        ZBuffer(unsigned int width, unsigned int height)
        {
            this->width = width;
            this->height = height;
            this->data.resize(width * height);
            this->mode = DeephMode::NONE;
            deep_check_func = std::bind(&ZBuffer::deep_check_none, this, std::placeholders::_1, std::placeholders::_2);
        }

        void set_deeph_mode(DeephMode mode)
        {
            this->mode = mode;
            switch (mode)
            {
            case 0:
                deep_check_func = std::bind(&ZBuffer::deep_check_none, this, std::placeholders::_1, std::placeholders::_2);
                break;
            case 1:
                deep_check_func = std::bind(&ZBuffer::deep_check_less, this, std::placeholders::_1, std::placeholders::_2);
                break;
            case 2:
                deep_check_func = std::bind(&ZBuffer::deep_check_greater, this, std::placeholders::_1, std::placeholders::_2);
                break;
            }
        }

        DeephMode get_deeph_mode()
        {
            return this->mode;
        }

        void clear()
        {
            for (unsigned int i = 0; i < width * height; i++)
            {
                data[i] = 0.0;
            }
        }
    };

    class Render
    {
    public:
        unsigned int width;
        unsigned int height;
        unsigned int data_size;
        FrameBuffer *frame_buffer;
        ZBuffer *zbuffer;

        Render(unsigned int width, unsigned int height)
        {
            this->width = width;
            this->height = height;
            this->data_size = this->width * this->height * 4;
            

            frame_buffer = new FrameBuffer(this->width, this->height);
            zbuffer = new ZBuffer(this->width, this->height);
        }
        ~Render()
        {
            delete frame_buffer;
            delete zbuffer;
        }

        unsigned char *get_result()
        {
            return &frame_buffer->data[0];
        }

        void clear()
        {
            frame_buffer->clear();
            zbuffer->clear();
        }

        bool draw_point(const unsigned int &x, const unsigned int &y, const glm::ivec4 &color)
        {

            const unsigned int i = (y * width + x) * 4;
            if (i + 3 >= data_size - 1)
            {
                return false;
            }

            frame_buffer->data[i] = color.r;
            frame_buffer->data[i + 1] = color.g;
            frame_buffer->data[i + 2] = color.b;
            frame_buffer->data[i + 3] = color.a;
            return true;
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
                    draw_point(y, x, color);
                }
                else
                {
                    draw_point(x, y, color);
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
            draw_line(a, b, color);
            draw_line(b, c, color);
            draw_line(c, a, color);
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
                    draw_point(j, a.y + i, color);
                }
            }
        }

        glm::vec3 calculate_screen_position(const float &x, const float &y, const float &z, const glm::mat4 &mvp)
        {
            glm::vec4 clip_space_pos = mvp * glm::vec4(x, y, z, 1.0);

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

        void draw_colorfull_triangle(const glm::vec3 *points, const glm::ivec4 &color)
        {
            glm::ivec2 bboxmin(width - 1, height - 1);
            glm::ivec2 bboxmax(0, 0);
            glm::ivec2 clamp(width - 1, height - 1);
            for (int i = 0; i < 3; i++)
            {
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

                    if (zbuffer->calculate_deep_check(int(P.x + P.y * width), P.z))
                    {
                        draw_point(P.x, P.y, color);
                    }
                }
            }
        }

        void draw_textured_triangle(const glm::vec3 *points, const glm::vec2 *uv, const glm::ivec4 &color, TSRPA::Texture &texture)
        {
            glm::ivec2 bboxmin(width - 1, height - 1);
            glm::ivec2 bboxmax(0, 0);
            glm::ivec2 clamp(width - 1, height - 1);
            for (int i = 0; i < 3; i++)
            {
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
                    if (zbuffer->calculate_deep_check(int(P.x + P.y * width), P.z))
                    {
                        glm::vec2 uv_cord(0.0, 0.0);
                        for (int i = 0; i < 3; i++)
                        {
                            uv_cord += uv[i] * bc_screen[i];
                        }

                        // printf("uv_cord ( %f %f )\n",uv_cord.x,uv_cord.y);
                        glm::ivec4 texture_color = texture.get_color(uv_cord.x * texture.width, texture.height - (uv_cord.y * texture.height));
                        glm::vec4 glm_texture_color(texture_color.r / 255.0, texture_color.g / 255.0, texture_color.b / 255.0, texture_color.a / 255.0);
                        glm::vec4 glm_alpha_color(color.r / 255.0, color.g / 255.0, color.b / 255.0, color.a / 255.0);
                        glm::vec4 glm_final_color = glm_alpha_color * glm_texture_color;

                        draw_point(P.x, P.y, create_color(glm_final_color.r, glm_final_color.g, glm_final_color.b, glm_final_color.a));
                    }
                }
            }
        }
    };

};
