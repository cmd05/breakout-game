#include "game.h"
#include "resource_manager.h"
#include "sprite_renderer.h"

// common render object to render our sprites
SpriteRenderer* Renderer;

GameObject* Player;

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
}

void Game::Update(float dt) {

}

void Game::ProcessInput(float deltaTime) {
    if(this->State == GAME_ACTIVE) {
        float distance = PLAYER_VELOCITY * deltaTime;
        
        if(this->Keys[GLFW_KEY_A]) {
            if(Player->Position.x >= 0)
                Player->Position.x -= distance;
        }
        if(this->Keys[GLFW_KEY_D]) {
            if(Player->Position.x <= (Width - Player->Size.x)) {
                Player->Position.x += distance;
            }
        }
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
    }
}