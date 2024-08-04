#ifndef POST_PROCESSOR_H
#define POST_PROCESSOR_H

#include <glad/glad.h>
#include <glm/glm.hpp>

#include "texture.h"
#include "sprite_renderer.h"
#include "shader.h"

// PostProcessor hosts all PostProcessing effects for the Breakout
// Game. It renders the game on a textured quad after which one can
// enable specific effects by enabling either the Confuse, Chaos or 
// Shake boolean. 
// It is required to call BeginRender() before rendering the game
// and EndRender() after rendering the game for the class to work.
class PostProcessor {
public:
    Shader PostProcessingShader;
    Texture2D Texture;
    unsigned int Width, Height;

    bool Confuse, Chaos, Shake;

    PostProcessor(Shader shader, unsigned int width, unsigned int height);
    
    // prepares the postprocessor's framebuffer operations before rendering the game
    void BeginRender();
    
    // should be called after rendering the game, so it stores all the rendered data into a texture object
    void EndRender();
    
    // renders the PostProcessor texture quad (as a screen-encompassing large sprite)
    void Render(float time);
private:
    unsigned int MSFBO; // multisampled fbo
    unsigned int FBO; // used for blitting MS color-buffer to texture
    unsigned int RBO; // rbo for MSFBO color buffer
    unsigned int VAO;

    void initRenderData(); // initialize quad for rendering post processing texture
};

#endif