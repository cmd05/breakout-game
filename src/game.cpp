#include "game.h"
#include "resource_manager.h"
#include "sprite_renderer.h"
#include "ball_object_collisions.h"

#include "particle_generator.h"

#include <iostream>
#include <glm/glm.hpp>

// common render object to render our sprites
SpriteRenderer* Renderer;
GameObject* Player;
BallObject* Ball;
ParticleGenerator* Particles;

Game::Game(unsigned int width, unsigned int height):
    State{GAME_ACTIVE}, Keys{}, Width{width}, Height{height} {}

Game::~Game() {
    delete Renderer;
    delete Player;
    delete Ball;
    delete Particles;
}

void Game::Init() {
    // load shaders
    ResourceManager::LoadShader("shaders/sprite.vs", "shaders/sprite.fs", nullptr, "sprite");
    ResourceManager::LoadShader("shaders/particle.vs", "shaders/particle.fs", nullptr, "particle");
    
    // configure shaders
    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(this->Width), static_cast<float>(this->Height), 0.0f, -1.0f, 1.0f);

    ResourceManager::GetShader("sprite").Use().SetInteger("sprite", 0);
    ResourceManager::GetShader("sprite").SetMatrix4("projection", projection);
    ResourceManager::GetShader("particle").Use().SetInteger("sprite", 0);
    ResourceManager::GetShader("particle").SetMatrix4("projection", projection);

    // load textures
    ResourceManager::LoadTexture("textures/awesomeface.png", true, "face");
    ResourceManager::LoadTexture("textures/background.jpg", false, "background");
    ResourceManager::LoadTexture("textures/awesomeface.png", true, "face");
    ResourceManager::LoadTexture("textures/block.png", false, "block");
    ResourceManager::LoadTexture("textures/block_solid.png", false, "block_solid");
    ResourceManager::LoadTexture("textures/paddle.png", true, "paddle");
    
    ResourceManager::LoadTexture("textures/particle.png", true, "particle");

    // set render specific controls
    Renderer = new SpriteRenderer(ResourceManager::GetShader("sprite"));
    Particles = new ParticleGenerator(ResourceManager::GetShader("particle"), ResourceManager::GetTexture("particle"), 500);

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

    // paddle
    glm::vec2 playerPos = glm::vec2(this->Width / 2.0f - PLAYER_SIZE.x / 2.0f, this->Height - PLAYER_SIZE.y);
    Player = new GameObject(playerPos, PLAYER_SIZE, ResourceManager::GetTexture("paddle"));

    // ball
    glm::vec2 ballPos = playerPos + glm::vec2(PLAYER_SIZE.x / 2.0f - BALL_RADIUS, -BALL_RADIUS * 2.0f);
    Ball = new BallObject(ballPos, BALL_RADIUS, INITIAL_BALL_VELOCITY, ResourceManager::GetTexture("face"));
}

void Game::Update(float dt) {
    // ball movement
    Ball->Move(dt, Width);
    // check for collisions
    DoCollisions();
    // update particles
    Particles->Update(dt, *Ball, 2, glm::vec2(Ball->Radius / 2.0f));
    // check loss condition
    if(Ball->Position.y >= this->Height) { // bottom edge
        this->ResetLevel();
        this->ResetPlayer();
    }
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
        // draw particles
        Particles->Draw();
        // draw ball
        Ball->Draw(*Renderer);
    }
}

void Game::ResetLevel()
{
    if (this->Level == 0)
        this->Levels[0].Load("levels/one.lvl", this->Width, this->Height / 2);
    else if (this->Level == 1)
        this->Levels[1].Load("levels/two.lvl", this->Width, this->Height / 2);
    else if (this->Level == 2)
        this->Levels[2].Load("levels/three.lvl", this->Width, this->Height / 2);
    else if (this->Level == 3)
        this->Levels[3].Load("levels/four.lvl", this->Width, this->Height / 2);
}

void Game::ResetPlayer() {
    // reset player / ball stats
    Player->Size = PLAYER_SIZE;
    Player->Position = glm::vec2(this->Width, this->Height) - (PLAYER_SIZE / 2.0f);
    glm::vec2 ballPos {Player->Position.x + (PLAYER_SIZE.x / 2.0f) - BALL_RADIUS, Player->Position.y - 2 * BALL_RADIUS};
    Ball->Reset(ballPos, INITIAL_BALL_VELOCITY);
}

// calculates which direction is vector is facing (closest to)
Direction VectorDirection(glm::vec2 target) {
    glm::vec2 compass[] = {
        glm::vec2(0.0f, 1.0f), // up
        glm::vec2(1.0f, 0.0f), // right
        glm::vec2(0.0f, -1.0f), // down
        glm::vec2(-1.0f, 0.0f) // left
    };

    float max = 0.0f;
    unsigned int best_match = -1;

    for(unsigned int i = 0; i < 4; i++) {
        float dot_product = glm::dot(glm::normalize(target), compass[i]);
        if(dot_product > max) {
            max = dot_product;
            best_match = i;
        }
    }

    return (Direction) best_match;
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
Collision Game::CheckCollision(BallObject& one, GameObject& two) {
    // center of `two`
    glm::vec2 aabb_half_extent = two.Size / 2.0f;
    glm::vec2 two_center = two.Position + aabb_half_extent;

    // center of `one`
    glm::vec2 ball_center = one.Position + one.Radius;

    glm::vec2 diff1 = ball_center - two_center;
    glm::vec2 diff1_clamped = glm::clamp(diff1, -aabb_half_extent, aabb_half_extent);

    glm::vec2 closest_aabb = two_center + diff1_clamped; // closest point to circle
    glm::vec2 diff2 = closest_aabb - ball_center;

    if(glm::length(diff2) <= one.Radius) // collision
        return std::make_tuple(true, VectorDirection(diff2), diff2);
    else
        return std::make_tuple(false, UP, glm::vec2(0.0f));

    // return glm::length(diff2) <= one.Radius;
}

#define USE_COM false

struct object_props {
    float m;
    glm::vec2 v;
    float e;
};

// Maybe we can use conservation of momentum and coefficient of restitution
// (taking note of velocities of both the objects in the collision)
// vec3: m, v, e
std::pair<glm::vec2, glm::vec2> conservation_of_momentum(object_props one, object_props two) {
    // use harmonic mean for combined `e`
    float e = (one.e * two.e);
    glm::vec2 vf_one = (one.m * one.v + two.m * two.v + two.m * e * (two.v - one.v)) / (one.m + two.m);
    glm::vec2 vf_two = e * (one.v - two.v) + vf_one;

    return {vf_one, vf_two};
}

// Note: so far throughout the game, speed (i.e magnitude(velocity)) never changes however velocity vector keeps changing
void Game::DoCollisions() {
    for(GameObject& brick : Levels[Level].Bricks)
        if(!brick.Destroyed) {
            Collision collision = CheckCollision(*Ball, brick);
            if(std::get<0>(collision)) {
                if(!brick.IsSolid)
                    brick.Destroyed = true;

                // collision resolution
                Direction dir = std::get<1>(collision); // up, down, left or right
                glm::vec2 diff_vector = std::get<2>(collision); // difference of closest point from ball center
                
                // note: we reverse velocity components on collision
                // this does not change the speed of the ball (speed = sqrt(x^2 + y^2))

                if(dir == LEFT || dir == RIGHT) { // horizontal collision
                    Ball->Velocity.x = -Ball->Velocity.x;

                    // relocate
                    float penetration = Ball->Radius - std::abs(diff_vector.x);
                    if(dir == LEFT)
                        Ball->Position.x += penetration;
                    else
                        Ball->Position.x -= penetration;
                } else { // vertical collision
                    Ball->Velocity.y = -Ball->Velocity.y;
                    
                    // relocate
                    float penetration = Ball->Radius - std::abs(diff_vector.y);
                    if(dir == UP)
                        Ball->Position.y -= penetration;
                    else
                        Ball->Position.y += penetration;
                }

                // direction calculation by dot product method
                // speed calculation by COM 
                if(USE_COM) {
                    // simulate COM for brick-ball
                    object_props ball_aluminium = {5, Ball->Velocity, 0.8};
                    object_props brick_clay = {7, glm::vec2(0.0f), 0.5};
                    
                    // the speed will always lessen due to restitution
                    glm::vec2 com_v = conservation_of_momentum(ball_aluminium, brick_clay).first;
                    // com_v /= 1.5f;
                    
                    std::cout << com_v.x << ' ' << com_v.y << ' ' << glm::length(com_v) << std::endl; 
                    Ball->Velocity = glm::normalize(Ball->Velocity) * glm::length(com_v);
                    Ball->Velocity *= 2.0f; // boost to match world space
                }
            }
        }
    
    // check player paddle and ball collisions
    Collision result = CheckCollision(*Ball, *Player);
    if(!Ball->Stuck && std::get<0>(result)) {
        // check where it hit the board, and change velocity based on where it hit the board
        float centerBoard = Player->Position.x + Player->Size.x / 2.0f;
        float distance = (Ball->Position.x + BALL_RADIUS) - centerBoard;
        float percentage = distance / (Player->Size.x / 2.0f); // value between 0 and 1
        // then move accordingly
        float strength = 2.0f;
        glm::vec2 oldVelocity = Ball->Velocity;
        Ball->Velocity.x = INITIAL_BALL_VELOCITY.x * percentage * strength;
        // Ball->Velocity.y = -Ball->Velocity.y;

        object_props ball_aluminium = {5, Ball->Velocity, 0.8};
        object_props ball_steel = {5, Ball->Velocity, 0.8};

        // dont change speed on collision with paddle
        Ball->Velocity = glm::normalize(Ball->Velocity) * glm::length(oldVelocity); // normalized 2d vector * old_length
    
        if(USE_COM)
            Ball->Velocity *= 1.05f; // boost speed when ball touches paddle
        
        // fix sticky paddle
        Ball->Velocity.y = -1.0f * abs(Ball->Velocity.y);
    }
}