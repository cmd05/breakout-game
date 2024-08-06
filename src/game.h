#ifndef GAME_H
#define GAME_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>

#include "miniaudio_split.h"

#include "game_level.h"
#include "game_object.h"
#include "ball_object_collisions.h"
#include "power_up.h"

// the globals are available to game.cpp

// current state of the game
enum GameState {
    GAME_ACTIVE,
    GAME_MENU,
    GAME_WIN
};

// Represents the four possible (collision) directions
enum Direction {
    UP,
    RIGHT,
    DOWN,
    LEFT
};

// Defines a Collision typedef that represents collision data
// <collision?, what direction?, difference vector center - closest point>
typedef std::tuple<bool, Direction, glm::vec2> Collision; 

// paddle state
const glm::vec2 PLAYER_SIZE{100.0f, 20.0f};
const float PLAYER_VELOCITY{500.0f};

// Initial velocity of the Ball
// always go in a fixed direction when ball moves for the first time
const glm::vec2 INITIAL_BALL_VELOCITY(100.0f, -350.0f);

// Radius of the ball object
const float BALL_RADIUS = 12.5f;

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
    std::vector<PowerUp> PowerUps;
    std::map<std::string, ma_sound> mySounds;

    // constructor / destructor
    Game(unsigned int width, unsigned int height);
    ~Game();
    // initialize game state (load all shaders / textures / levels)
    void Init();
    void init_audio();
    // game loop
    void ProcessInput(float dt);
    void Update(float dt);
    void Render();
    void DoCollisions();

    void ResetPlayer();
    void ResetLevel();

    // powerups
    void SpawnPowerUps(GameObject& block);
    void UpdatePowerUps(float dt);
private:
    bool CheckCollision(GameObject& one, GameObject& two);
    Collision CheckCollision(BallObject& one, GameObject& two);
};

#endif