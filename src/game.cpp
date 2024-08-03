#include "game.h"
#include "resource_manager.h"
#include "sprite_renderer.h"
#include "ball_object_collisions.h"

#include <iostream>

// common render object to render our sprites
SpriteRenderer* Renderer;

GameObject* Player;

// Initial velocity of the Ball
// always go in a fixed direction when ball moves for the first time
const glm::vec2 INITIAL_BALL_VELOCITY(100.0f, -350.0f);

// Radius of the ball object
const float BALL_RADIUS = 12.5f;

BallObject* Ball;

Game::Game(unsigned int width, unsigned int height):
    State{GAME_ACTIVE}, Keys{}, Width{width}, Height{height} {

}

Game::~Game() {
    delete Renderer;
    delete Player;
}

void Game::Init() {
    // load shaders
    ResourceManager::LoadShader("shaders/sprite.vs", "shaders/sprite.fs", nullptr, "sprite");
    // configure shaders
    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(this->Width), static_cast<float>(this->Height), 0.0f, -1.0f, 1.0f);
    // ResourceManager::GetShader("sprite").Use().SetInteger("sprite", 0);
    ResourceManager::GetShader("sprite").Use().SetMatrix4("projection", projection);
    // set render specific controls
    Renderer = new SpriteRenderer(ResourceManager::GetShader("sprite"));
    // load textures
    ResourceManager::LoadTexture("textures/awesomeface.png", true, "face");
    // load textures
    ResourceManager::LoadTexture("textures/background.jpg", false, "background");
    ResourceManager::LoadTexture("textures/awesomeface.png", true, "face");
    ResourceManager::LoadTexture("textures/block.png", false, "block");
    ResourceManager::LoadTexture("textures/block_solid.png", false, "block_solid");
    ResourceManager::LoadTexture("textures/paddle.png", true, "paddle");
    // load levels
    GameLevel one; one.Load("levels/one.lvl", this->Width, this->Height / 2);
    GameLevel two; two.Load("levels/two.lvl", this->Width, this->Height / 2);
    GameLevel three; three.Load("levels/three.lvl", this->Width, this->Height / 2);
    GameLevel four; four.Load("levels/four.lvl", this->Width, this->Height / 2);
    this->Levels.push_back(one);
    this->Levels.push_back(two);
    this->Levels.push_back(three);
    this->Levels.push_back(four);
    this->Level = 0;

    glm::vec2 playerPos = glm::vec2(this->Width / 2.0f - PLAYER_SIZE.x / 2.0f, this->Height - PLAYER_SIZE.y);
    Player = new GameObject(playerPos, PLAYER_SIZE, ResourceManager::GetTexture("paddle"));

    glm::vec2 ballPos = playerPos + glm::vec2(PLAYER_SIZE.x / 2.0f - BALL_RADIUS, -BALL_RADIUS * 2.0f);
    Ball = new BallObject(ballPos, BALL_RADIUS, INITIAL_BALL_VELOCITY, ResourceManager::GetTexture("face"));
}

void Game::Update(float dt) {
    // ball movement
    Ball->Move(dt, Width);
    // check for collisions
    DoCollisions();
}

void Game::ProcessInput(float deltaTime) {
    if(State == GAME_ACTIVE) {
        float distance = PLAYER_VELOCITY * deltaTime;
        // move paddle
        if(this->Keys[GLFW_KEY_A]) {
            if(Player->Position.x >= 0) {
                Player->Position.x -= distance;
                if(Ball->Stuck)
                    Ball->Position.x -= distance;
            }
        }

        if(this->Keys[GLFW_KEY_D]) {
            if(Player->Position.x <= (Width - Player->Size.x)) {
                Player->Position.x += distance;
                if(Ball->Stuck)
                    Ball->Position.x += distance;
            }
        }

        if(this->Keys[GLFW_KEY_SPACE])
            Ball->Stuck = false;
    }
}

void Game::Render() {
    if(this->State == GAME_ACTIVE) {
        // draw background
        Renderer->DrawSprite(ResourceManager::GetTexture("background"), glm::vec2(0.0f, 0.0f), glm::vec2(this->Width, this->Height), 0.0f);
        // draw level
        this->Levels[this->Level].Draw(*Renderer);
        // draw player
        Player->Draw(*Renderer);
        // draw bricks
        this->Levels[this->Level].Draw(*Renderer);
        // draw ball
        Ball->Draw(*Renderer);
    }
}

// AABB - AABB
bool Game::CheckCollision(GameObject& one, GameObject& two) {
    bool collisionX = one.Position.x + one.Size.x >= two.Position.x && 
                        one.Position.x <= (two.Position.x + two.Size.x);

    bool collisionY = (one.Position.y + one.Size.y) >= two.Position.y &&
                        one.Position.y <= (two.Position.y + two.Size.y);
    
    return collisionX && collisionY;
}

// ball is axis aligned bounding circle and `two` is AABB
bool Game::CheckCollision(BallObject& one, GameObject& two) {
    // center of `two`
    glm::vec2 aabb_half_extent = two.Size / 2.0f;
    glm::vec2 two_center = two.Position + aabb_half_extent;

    // center of `one`
    glm::vec2 ball_center = one.Position + one.Radius;

    glm::vec2 diff1 = ball_center - two_center;
    glm::vec2 diff1_clamped = glm::clamp(diff1, -aabb_half_extent, aabb_half_extent);

    glm::vec2 closest_aabb = two_center + diff1_clamped; // closest point to circle
    glm::vec2 diff2 = ball_center - closest_aabb;

    return glm::length(diff2) <= one.Radius;
}

void Game::DoCollisions() {
    for(GameObject& brick : Levels[Level].Bricks)
        if(!brick.Destroyed)
            if(CheckCollision(*Ball, brick)) {
                if(!brick.IsSolid)
                    brick.Destroyed = true;
                // else {
                //     brick.Color = glm::vec3(1.0);
                //     std::cout << "solid\n";
                // }
            }
}