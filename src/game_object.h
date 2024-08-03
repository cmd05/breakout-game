#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H

#include <glad/glad.h>
#include <glm/glm.hpp>

#include "texture.h"
#include "sprite_renderer.h"

// Container object for holding all state relevant for a single
// game object entity. Each object in the game likely needs the
// minimal of state as described within GameObject.
class GameObject {
public:
    // object state
    // for objects in our game velocity directly corresponds to (world_space_moved / second)
    // which is equal to (screen_coords_moved / seconds) since both spaces have the same dimensions
    // in our game
    glm::vec2 Position, Size, Velocity;
    glm::vec3 Color;

    float Rotation;
    bool IsSolid;
    bool Destroyed;

    // render state
    Texture2D Sprite;

    // constructors
    GameObject();
    GameObject(glm::vec2 pos, glm::vec2 size, Texture2D sprite, glm::vec3 color = glm::vec3(1.0f), glm::vec2 velocity = glm::vec2(0.0f, 0.0f));

    // draw sprite
    virtual void Draw(SpriteRenderer& renderer); // virtual method can be overwritten by a child class
};

#endif