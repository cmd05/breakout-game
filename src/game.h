#ifndef GAME_H
#define GAME_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

// current state of the game
enum GameState {
    GAME_ACTIVE,
    GAME_MENU,
    GAME_WIN
};

// game holds all game-related state and functionality
// combines all game realted data in a single class for easy acess to all components and manageability
class Game {
public:
    // game state
    GameState State;
    bool Keys[1024];
    unsigned int Width, Height;

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