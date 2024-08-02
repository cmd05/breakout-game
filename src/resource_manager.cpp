#include "resource_manager.h"

#include <iostream>
#include <sstream>
#include <fstream>

#include "stb_image.h"

// note that ResourceManager interfaces directly with OpenGL for glDeleteProgram and glDeleteTextures.
// rest interfacing is directly through our Shader and Texture classes

// instantiate static variables
std::map<std::string, Texture2D> ResourceManager::Textures;
std::map<std::string, Shader> ResourceManager::Shaders;

Shader ResourceManager::LoadShader(const char* vShaderFile, const char* fShaderFile, const char* gShaderFile, std::string name) {
    Shaders[name] = loadShaderFromFile(vShaderFile, fShaderFile, gShaderFile);
    return Shaders[name];
}

Shader& ResourceManager::GetShader(std::string name) {
    return Shaders[name]; // return from reference is ok since the variable is part of the class
}

Texture2D ResourceManager::LoadTexture(const char* file, bool alpha, std::string name) {
    Textures[name] = loadTextureFromFile(file, alpha);
    return Textures[name];
}

Texture2D& ResourceManager::GetTexture(std::string name) {
    return Textures[name];
}

void ResourceManager::Clear() {
    for(auto iter : Shaders)
        glDeleteProgram(iter.second.ID);
    for(auto iter : Textures)
        glDeleteTextures(1, &iter.second.ID);
}

Shader ResourceManager::loadShaderFromFile(const char* vShaderFile, const char* fShaderFile, const char* gShaderFile) {
    std::string vertexCode, fragmentCode, geometryCode;

    try {
        // open files
        std::ifstream vertexShaderFile(vShaderFile);
        std::ifstream fragmentShaderFile(fShaderFile);
        std::stringstream vShaderStream, fShaderStream;

        // read file's buffer contents into streams
        // sometimes rdbuf returns an empty string when reading from the file
        try {
            vShaderStream << vertexShaderFile.rdbuf();
            fShaderStream << fragmentShaderFile.rdbuf();

            // convert stream into string
            vertexCode = vShaderStream.str();
            fragmentCode = fShaderStream.str();
        } catch(std::ios_base::failure& e) {
            std::cout << "Error while reading vertex / fragment shader file";
        }

        if(!vertexShaderFile || !fragmentShaderFile || vertexCode.empty() || fragmentCode.empty())
            throw std::runtime_error("Error while reading vertex / fragment shader file");

        // close file handlers
        vertexShaderFile.close();
        fragmentShaderFile.close();

        // if geometry shader is present, also load a geometry shader
        if(gShaderFile != nullptr) {
            std::ifstream geometryShaderFile(gShaderFile);
            std::stringstream gShaderStream;
            
            try {
                gShaderStream << geometryShaderFile.rdbuf();            
                geometryCode = gShaderStream.str();
            } catch(std::ios_base::failure& e) {
                std::cout << "Error while reading geometry shader file";
            }

            if(!geometryShaderFile || geometryCode.empty())
                throw std::runtime_error("Error while reading geometry shader file");

            geometryShaderFile.close();
        }
    } catch(std::exception& e) {
        std::cout << "ERROR::SHADER: Failed to read shader files" << std::endl;
    }

    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();
    const char* gShaderCode = geometryCode.c_str();

    // 2. now create shader object from source code
    Shader shader;
    shader.Compile(vShaderCode, fShaderCode, gShaderFile != nullptr ? gShaderCode : nullptr);

    return shader;
}

Texture2D ResourceManager::loadTextureFromFile(const char* file, bool alpha) {
    // create texture object
    Texture2D texture;

    if(alpha) {
        texture.Internal_Format = GL_RGBA;
        texture.Image_Format = GL_RGBA;
    }

    // load image
    int width, height, nrChannels;
    unsigned char* data = stbi_load(file, &width, &height, &nrChannels, 0);
    // now generate texture
    texture.Generate(width, height, data);
    // and finally free image data
    stbi_image_free(data);
    return texture;
}