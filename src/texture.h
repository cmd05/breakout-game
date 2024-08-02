#ifndef TEXTURE_H
#define TEXTURE_H

#include <glad/glad.h>

class Texture2D {
public:
    unsigned int ID;
    unsigned int Width, Height;
    unsigned int Internal_Format; // format of texture object
    unsigned int Image_Format; // format of loaded image

    // texture configuration
    unsigned int Wrap_S;
    unsigned int Wrap_T;
    unsigned int Filter_Min;
    unsigned int Filter_Max;

    Texture2D();

    void Generate(unsigned int width, unsigned int height, unsigned char* data);
    void Bind() const;
};

#endif