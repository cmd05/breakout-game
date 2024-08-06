#version 330 core
in vec2 TexCoords;
out vec4 color;

uniform sampler2D scene;
uniform vec2 offsets[9];
uniform int edge_kernel[9];
uniform float blur_kernel[9];

uniform bool chaos;
uniform bool confuse;
uniform bool shake;
uniform bool grayscale;

void main() {
    // set to zero, as `out` variable is initialized with undefined values by default 
    color = vec4(0.0);

    vec3 sample[9];

    // sample texture from texture offsets if using convolution matrix
    if(chaos || shake)
        for(int i = 0; i < 9; i++)
            sample[i] = vec3(texture(scene, TexCoords.st + offsets[i]));
    
    // process effects
    if(chaos) {
        for(int i = 0; i < 9; i++)
            color += vec4(sample[i] * edge_kernel[i], 0.0);
        color.a = 1.0;
    } else if(confuse) {
        color = vec4(1.0 - texture(scene, TexCoords).rgb, 1.0);
    } else if(shake) {
        for(int i = 0; i < 9; i++)
            color += vec4(sample[i] * blur_kernel[i], 0.0f);
        
        color.a = 1.0f;
    } else if(grayscale) {
        vec3 grayscale_transform = vec3(0.3,0.59,0.11);
        color = vec4(vec3(dot(grayscale_transform, texture(scene, TexCoords).rgb)), 1.0);
    } else {
        color = texture(scene, TexCoords);
    }
}