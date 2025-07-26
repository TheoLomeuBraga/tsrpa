#pragma once
#include <iostream>
#include <cmath>
#include <glm/glm.hpp>

namespace TSRPA
{

    class Color256
    {
    public:
        unsigned char r = 0;
        unsigned char g = 0;
        unsigned char b = 0;
        unsigned char a = 0;
        Color256() {}
        Color256(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
        {
            this->r = r;
            this->g = g;
            this->b = b;
            this->a = a;
        }
    };

#ifdef TSRPA_ADD_BASIC_COLOR_PALETTE

    namespace Palette
    {
        const Color256 INVISIBLE(0, 0, 0, 0);
        const Color256 BLACK(0, 0, 0, 255);
        const Color256 WHITE(255, 255, 255, 255);
        const Color256 RED(255, 0, 0, 255);
        const Color256 GREEN(0, 255, 0, 255);
        const Color256 BLUE(0, 0, 255, 255);
    };

#endif

    unsigned char to_uchar_value(float value)
    {
        return (unsigned char)std::round(value * 255.0);
    }

    Color256 create_color(float r, float g, float b, float a)
    {
        return Color256(to_uchar_value(r), to_uchar_value(g), to_uchar_value(b), to_uchar_value(a));
    }

    class FrameBuffer
    {
    public:
        unsigned int width;
        unsigned int height;
        unsigned char *data;
        Color256 clear_color;

        FrameBuffer() {}

        FrameBuffer(unsigned int width, unsigned int height)
        {
            this->width = width;
            this->height = height;
            this->data = new unsigned char[width * height * 4];
        }

        void clear(){
            for (unsigned int i = 0; i < width * height * 4; i += 4)
            {
                data[i] = clear_color.r;
                data[i + 1] = clear_color.g;
                data[i + 2] = clear_color.b;
                data[i + 3] = clear_color.a;
            }
        }

        ~FrameBuffer()
        {
            if (this->data != NULL)
            {
                delete[] this->data;
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

        Render(unsigned int width, unsigned int height)
        {
            this->width = width;
            this->height = height;
            this->data_size = this->width * this->height * 4;
            frame_buffer = new FrameBuffer(this->width, this->height);
        }
        ~Render()
        {
            delete frame_buffer;
        }

        unsigned char *get_result()
        {
            return frame_buffer->data;
        }

        void clear()
        {
            frame_buffer->clear();
        }

        void draw_point(unsigned int x, unsigned int y, const Color256 &color)
        {
            
            const unsigned int i = (y * width + x) * 4;
            if (i + 3 >= data_size -1){
                return;
            }
            frame_buffer->data[i] = color.r;
            frame_buffer->data[i + 1] = color.g;
            frame_buffer->data[i + 2] = color.b;
            frame_buffer->data[i + 3] = color.a;
        }

        bool draw_line(glm::vec2 a,  glm::vec2 b, const Color256 &color)
        {

            bool steep = false;
            if (std::abs(a.x - b[0]) < std::abs(a.y - b[1]))
            {
                std::swap(a.x, a.y);
                std::swap(b.x, b.y);
                steep = true;
            }
            if (a[0] > b[0])
            {
                std::swap(a[0], b[0]);
                std::swap(a[1], b[1]);
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
                    y += (y1 > y0 ? 1 : -1);
                    error2 -= dx * 2;
                }
            }
            return true;
        }
    };

};
