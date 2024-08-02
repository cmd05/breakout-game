#ifndef GAME_H
#define GAME_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>

#include "game_level.h"

// the globals are available to game.cpp

// current state of the game
enum GameState {
    GAME_ACTIVE,
    GAME_MENU,
    GAME_WIN
};

// paddle state
const glm::vec2 PLAYER_SIZE{100.0f, 20.0f};
const float PLAYER_VELOCITY{500.0f};

// game holds all game-related state and functionality
// combines all game realted data in a single class for easy acess to all components and manageability
class Game {
public:
    // game state
    GameState State;
    bool Keys[1024];
    unsigned int Width, Height;
    std::vector<GameLevel> Levels;
    unsigned int Level;
    
    // constructor / destructor
    Game(unsigned int width, unsigned int height);
    ~Game();
    // initialize game state (load all shaders / textures / levels)
    void Init();
    // game loop
    void ProcessInput(float dt);
    void Update(float dt);
    void Render();
};

#endif