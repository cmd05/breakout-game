#include <iostream>
#include <algorithm>
#include <sstream>

#include <glm/glm.hpp>

#include "miniaudio_split.h"

#include "game.h"

#include "resource_manager.h"
#include "sprite_renderer.h"
#include "ball_object_collisions.h"
#include "particle_generator.h"
#include "post_processor.h"
#include "text_renderer.h"

// common render object to render our sprites
SpriteRenderer* Renderer;
GameObject* Player;
BallObject* Ball;
ParticleGenerator* Particles;
PostProcessor* Effects;
TextRenderer* Text;

static ma_engine g_engine;
static ma_result g_result;

float ShakeTime = 0.0f;

const float explosionWait = 3;
int explosionColor = 1;
float explosionTime = explosionWait;
bool will_explode = false;
std::vector<GameObject*> bricksToExplode = {};

Game::Game(unsigned int width, unsigned int height):
    State{GAME_ACTIVE}, Keys{}, Width{width}, Height{height}, Level{0}, Lives{3} {}

Game::~Game() {
    // clean audio resources
    for(auto it : mySounds) {
        ma_sound_stop(&it.second);
        ma_sound_uninit(&it.second);
    }

    ma_engine_stop(&g_engine);
    ma_engine_uninit(&g_engine);

    delete Renderer;
    delete Player;
    delete Ball;
    delete Particles;
    delete Text;
}

void Game::init_audio() {
    g_result = ma_engine_init(NULL, &g_engine);
    
    if (g_result != MA_SUCCESS)
        throw std::runtime_error("Failed to initialize audio engine.");
    
    // load audio files
    std::vector<std::string> audio_files = {"bleep.mp3", "breakout.mp3", "powerup.wav", "solid.wav", "fireworks.mp3", "game-won.wav", "game-lost.wav"};

    for(std::string file : audio_files) {
        std::string name = file;
        int n = name.length();
        
        for(std::string::reverse_iterator it = name.rbegin(); it != name.rend(); it++, n--)
            if(*it == '.')
                break;

        name.resize(n-1);

        mySounds[name];

        std::string path = std::string(FS_SRC_PATH) + "audio/";
        path += file;

        g_result = ma_sound_init_from_file(&g_engine, path.c_str(), MA_SOUND_FLAG_ASYNC , NULL, NULL, &mySounds[name]);
        
        if (g_result != MA_SUCCESS) {
            std::cout << "Failed to initialize sound: " << path << std::endl;
            throw std::runtime_error("MINIAUDIO_USER::ERROR");
        }
    }
}

void Game::Init() {
    srand(time(NULL));
    
    // Initialize audio engine
    init_audio();

    // load shaders
    ResourceManager::LoadShader("shaders/sprite.vs", "shaders/sprite.fs", nullptr, "sprite");
    ResourceManager::LoadShader("shaders/particle.vs", "shaders/particle.fs", nullptr, "particle");
    ResourceManager::LoadShader("shaders/post_processing.vs", "shaders/post_processing.fs", nullptr, "postprocessing");
    
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

    ResourceManager::LoadTexture("textures/powerup_speed.png", true, "powerup_speed");
    ResourceManager::LoadTexture("textures/powerup_sticky.png", true, "powerup_sticky");
    ResourceManager::LoadTexture("textures/powerup_increase.png", true, "powerup_increase");
    ResourceManager::LoadTexture("textures/powerup_confuse.png", true, "powerup_confuse");
    ResourceManager::LoadTexture("textures/powerup_chaos.png", true, "powerup_chaos");
    ResourceManager::LoadTexture("textures/powerup_passthrough.png", true, "powerup_passthrough");
    ResourceManager::LoadTexture("textures/powerup_ball-decrease.png", true, "powerup_ball-decrease");
    ResourceManager::LoadTexture("textures/powerup_ball-increase.png", true, "powerup_ball-increase");
    ResourceManager::LoadTexture("textures/powerup_fireworks.png", true, "powerup_fireworks");

    // set render specific controls
    Renderer = new SpriteRenderer(ResourceManager::GetShader("sprite"));
    Particles = new ParticleGenerator(ResourceManager::GetShader("particle"), ResourceManager::GetTexture("particle"), 500);
    Effects = new PostProcessor(ResourceManager::GetShader("postprocessing"), this->Width, this->Height);
    Text = new TextRenderer(this->Width, this->Height);
    Text->Load(std::string(FS_SRC_PATH) + "fonts/OCRAEXT.ttf", 24);

    // load levels
    GameLevel one; one.Load((std::string(FS_SRC_PATH) + "levels/one.lvl").c_str(), this->Width, this->Height / 2);
    GameLevel two; two.Load((std::string(FS_SRC_PATH) + "levels/two.lvl").c_str(), this->Width, this->Height / 2);
    GameLevel three; three.Load((std::string(FS_SRC_PATH) + "levels/three.lvl").c_str(), this->Width, this->Height / 2);
    GameLevel four; four.Load((std::string(FS_SRC_PATH) + "levels/four.lvl").c_str(), this->Width, this->Height / 2);

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

    // audio
    ma_sound_start(&mySounds["breakout"]);
    ma_sound_set_looping(&mySounds["breakout"], MA_TRUE);

    this->State = GAME_MENU;
}

void Game::fireworks_explosion() {
    bool explosionEffect = false;

    for(GameObject* brick : bricksToExplode) {
        if(!brick->Destroyed && !explosionEffect) explosionEffect = true;
        brick->Destroyed = true; // some bricks may already be destroyed by player
                                 // but this should not have an effect
    }

    if(explosionEffect) {
        ma_sound_start(&mySounds["fireworks"]);
        
        ShakeTime = 0.25f;
        Effects->Shake = true;
    }
}

void Game::Update(float dt) {
    // ball movement
    Ball->Move(dt, Width);
    // check for collisions
    DoCollisions();
    // update particles
    Particles->Update(dt, *Ball, 2, glm::vec2(Ball->Radius / 2.0f));
    // update powerups
    this->UpdatePowerUps(dt);
    // reduce shake time
    if (ShakeTime > 0.0f) {
        ShakeTime -= dt;
        if (ShakeTime <= 0.0f)
            Effects->Shake = false;
    }
    
    // explosion
    if(will_explode) {
        explosionTime -= dt;
        
        // change light color each step
        // glm::vec3 col {0.78f, 0.4f, 0.1f};
        glm::vec3 col {1.0f, 0.f, 0.0f};

        if(!explosionColor)
            // col = {0.8f, 0.8f, 0.1f};
            col = {0.0f, 0.0f, 1.0f};

        explosionColor = !explosionColor;

        for(GameObject* brick : bricksToExplode)
            if(!brick->Destroyed)
                brick->Color = col;

        if(explosionTime <= 0.0f) {
            fireworks_explosion();

            // clean up
            will_explode = false;
            explosionTime = 0.0f;
            explosionColor = 1;
            bricksToExplode.clear();
        }
    }
    
    // check loss condition
    if(Ball->Position.y >= this->Height) { // bottom edge
        --this->Lives;

        if(this->Lives == 0) {
            this->ResetLevel();
            // this->State = GAME_MENU;
            Effects->Grayscale = true;
            this->State = GAME_LOST;
            ma_sound_start(&mySounds["game-lost"]);
        }

        this->ResetPlayer();
    }

    // check win condition
    if(this->State == GAME_ACTIVE && this->Levels[this->Level].isCompleted()) {
        this->ResetLevel();
        this->ResetPlayer();
        Effects->Chaos = true; // celebration effect
        
        ma_sound_start(&mySounds["game-won"]);
        this->State = GAME_WIN;
    }
}

void Game::ProcessInput(float deltaTime) {
    if(this->State == GAME_MENU) {
        if(this->Keys[GLFW_KEY_ENTER] && !this->KeysProcessed[GLFW_KEY_ENTER]) {
            this->State = GAME_ACTIVE;
            this->KeysProcessed[GLFW_KEY_ENTER] = true;
        }
        if(this->Keys[GLFW_KEY_W] && !this->KeysProcessed[GLFW_KEY_W]) {
            this->Level = (this->Level + 1) % 4;
            this->KeysProcessed[GLFW_KEY_W] = true;
        }
        if(this->Keys[GLFW_KEY_S] && !this->KeysProcessed[GLFW_KEY_S]) {
            if(this->Level > 0)
                --this->Level;
            else
                this->Level = 3;
            
            this->KeysProcessed[GLFW_KEY_S] = true;
        }
    }

    if(this->State == GAME_WIN) {
        if(this->Keys[GLFW_KEY_ENTER]) {
            this->KeysProcessed[GLFW_KEY_ENTER] = true;
            Effects->Chaos = false; // disable celebration effect
            this->State = GAME_MENU;
        }
    }
    
    if(this->State == GAME_LOST) {
        if(this->Keys[GLFW_KEY_ENTER]) {
            this->KeysProcessed[GLFW_KEY_ENTER] = true;
            Effects->Grayscale = false; // disable celebration effect
            this->State = GAME_MENU;
        }
    }

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
    // begin rendering to off screen renderer
    Effects->BeginRender();

    // draw background
    Renderer->DrawSprite(ResourceManager::GetTexture("background"), glm::vec2(0.0f, 0.0f), glm::vec2(this->Width, this->Height), 0.0f);
    // draw level
    this->Levels[this->Level].Draw(*Renderer);
    // draw player
    Player->Draw(*Renderer);
    // draw power ups
    
    for(PowerUp& powerup : this->PowerUps)
        if(!powerup.Destroyed)
            powerup.Draw(*Renderer);

    // draw bricks
    this->Levels[this->Level].Draw(*Renderer);
    // draw particles
    Particles->Draw();
    // draw ball
    Ball->Draw(*Renderer);

    Effects->EndRender(); // copy to normal FBO

    Effects->Render(glfwGetTime()); // render to screen

    // render text (don't include in postprocessing)
    std::stringstream ss; ss << this->Lives;
    Text->RenderText("Lives:" + ss.str(), 5.0f, 5.0f, 1.0f);

    if(this->State == GAME_MENU) {
        Text->RenderText("Press ENTER to start", 250.0f, this->Height / 2.0f, 1.0f);
        Text->RenderText("Press W or S to select level", 245.0f, this->Height / 2.0f + 20.0f, 0.75f);
        Text->RenderText("Level " + std::to_string(this->Level), 350.0f, this->Height / 2.0f + 40.0f, 0.75, glm::vec3(0.568, 0.176, 0.372));
    }
    if (this->State == GAME_WIN)
    {
        Text->RenderText("YOU WON!!!1!", 320.0f, this->Height / 2.0f - 20.0f, 1.0f, glm::vec3(0.0f, 1.0f, 0.0f));
        Text->RenderText("Press ENTER to retry or ESC to quit", 130.0f, this->Height / 2.0f, 1.0f, glm::vec3(1.0f, 1.0f, 0.0f));
    }
    if (this->State == GAME_LOST)
    {
        Text->RenderText("YOU DEER :(", 320.0f, this->Height / 2.0f - 20.0f, 1.0f, glm::vec3(1.0f, 0.0f, 0.0f));
        Text->RenderText("Press ENTER to retry or ESC to quit", 130.0f, this->Height / 2.0f, 1.0f, glm::vec3(1.0f, 1.0f, 0.0f));
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

    this->Lives = 3;
}

void Game::ResetPlayer() {
    // reset player / ball stats
    Player->Size = PLAYER_SIZE;
    Player->Position = glm::vec2(this->Width / 2.0f - PLAYER_SIZE.x / 2.0f, this->Height - PLAYER_SIZE.y);
    
    Ball->Radius = BALL_RADIUS;
    Ball->Size = glm::vec2(BALL_RADIUS * 2.0f, BALL_RADIUS * 2.0f);
    glm::vec2 ballPos {Player->Position.x + (PLAYER_SIZE.x / 2.0f) - Ball->Radius, Player->Position.y - 2 * Ball->Radius};
    Ball->Reset(ballPos, INITIAL_BALL_VELOCITY);

    // disable all power ups and their effects
    Effects->Chaos = Effects->Confuse = false;
    Ball->PassThrough = Ball->Sticky = false;
    Player->Color = glm::vec3(1.0f);
    Ball->Color = glm::vec3(1.0f);

    this->PowerUps.clear();
}

bool IsOtherPowerUpActive(std::vector<PowerUp> &powerUps, std::string type)
{
    // Check if another PowerUp of the same type is still active
    // in which case we don't disable its effect (yet)
    for (const PowerUp &powerUp : powerUps)
    {
        if (powerUp.Activated)
            if (powerUp.Type == type)
                return true;
    }
    return false;
}

void Game::UpdatePowerUps(float dt) {
    for(PowerUp& powerUp : this->PowerUps) {
        powerUp.Position += powerUp.Velocity * dt; // falling down
        if(powerUp.Activated) {
            powerUp.Duration -= dt;

            if(powerUp.Duration <= 0.0f) {
                powerUp.Activated = false; // deactivate power up

                // deactivate effects
                if(powerUp.Type == "sticky") {
                    if(!IsOtherPowerUpActive(this->PowerUps, "sticky")) {
                        // only reset if no other PowerUp of type sticky is active
                        Ball->Sticky = false;
                        Player->Color = glm::vec3(1.0f);
                    }
                } else if (powerUp.Type == "pass-through") {
                    if (!IsOtherPowerUpActive(this->PowerUps, "pass-through"))
                    {	// only reset if no other PowerUp of type pass-through is active
                        Ball->PassThrough = false;
                        Ball->Color = glm::vec3(1.0f);
                    }
                }
                else if (powerUp.Type == "confuse")
                {
                    if (!IsOtherPowerUpActive(this->PowerUps, "confuse"))
                    {	// only reset if no other PowerUp of type confuse is active
                        Effects->Confuse = false;
                    }
                }
                else if (powerUp.Type == "chaos")
                {
                    if (!IsOtherPowerUpActive(this->PowerUps, "chaos"))
                    {	// only reset if no other PowerUp of type chaos is active
                        Effects->Chaos = false;
                    }
                }
                else if (powerUp.Type == "ball-increase")
                {
                    if (!IsOtherPowerUpActive(this->PowerUps, "ball-increase"))
                    {	// only reset if no other PowerUp of type chaos is active
                        Ball->Radius = BALL_RADIUS;
                        Ball->Size = glm::vec2(BALL_RADIUS * 2.0f, BALL_RADIUS * 2.0f);
                    }
                }
                else if (powerUp.Type == "ball-decrease")
                {
                    if (!IsOtherPowerUpActive(this->PowerUps, "ball-decrease"))
                    {	// only reset if no other PowerUp of type chaos is active
                        Ball->Radius = BALL_RADIUS;
                        Ball->Size = glm::vec2(BALL_RADIUS * 2.0f, BALL_RADIUS * 2.0f);
                    }
                }

                // handled by fireworks_explosion()
                // else if (powerUp.Type == "fireworks")
                // {
                //     if (!IsOtherPowerUpActive(this->PowerUps, "fireworks"))
                //     {	
                //         will_explode = false;
                //         explosionTime = 0.0f;
                //     }
                // }
            }
        }

        // Note: unactivated but undestroyed powerups should not be removed
        // Remove all PowerUps from vector that are destroyed AND !activated (thus either off the map or finished)
        // Note we use a lambda expression to remove each PowerUp which is destroyed and not activated
        this->PowerUps.erase(std::remove_if(this->PowerUps.begin(), this->PowerUps.end(),
            [](const PowerUp& powerUp) { return powerUp.Destroyed && !powerUp.Activated; }), this->PowerUps.end());
    }
}

// 1 in `chance` possibility
bool ShouldSpawn(unsigned int chance) {
    unsigned int random = rand() % chance;
    return random == 0;
}

void Game::SpawnPowerUps(GameObject& block) {
    const int positive_chance = 30;
    const int neg_chance = 20;

    if (ShouldSpawn(positive_chance)) 
        this->PowerUps.push_back(PowerUp("speed", glm::vec3(0.5f, 0.5f, 1.0f), 0.0f, block.Position, ResourceManager::GetTexture("powerup_speed")));
    if (ShouldSpawn(positive_chance))
        this->PowerUps.push_back(PowerUp("sticky", glm::vec3(1.0f, 0.5f, 1.0f), 20.0f, block.Position, ResourceManager::GetTexture("powerup_sticky")));
    if (ShouldSpawn(positive_chance))
        this->PowerUps.push_back(PowerUp("pass-through", glm::vec3(0.5f, 1.0f, 0.5f), 10.0f, block.Position, ResourceManager::GetTexture("powerup_passthrough")));
    if (ShouldSpawn(positive_chance))
        this->PowerUps.push_back(PowerUp("pad-size-increase", glm::vec3(1.0f, 0.6f, 0.4), 0.0f, block.Position, ResourceManager::GetTexture("powerup_increase")));
    if (ShouldSpawn(positive_chance))
        this->PowerUps.push_back(PowerUp("ball-decrease", glm::vec3(1.0f, 0.3f, 0.3f), 20.0f, block.Position, ResourceManager::GetTexture("powerup_ball-decrease")));
    if (ShouldSpawn(positive_chance))
        this->PowerUps.push_back(PowerUp("fireworks", glm::vec3(0.96f, 0.47f, 0.25f), explosionWait, block.Position, ResourceManager::GetTexture("powerup_fireworks")));
    if (ShouldSpawn(positive_chance))
        this->PowerUps.push_back(PowerUp("ball-increase", glm::vec3(1.0f, 0.6f, 0.4), 10.0f, block.Position, ResourceManager::GetTexture("powerup_ball-increase")));
    
    if (ShouldSpawn(neg_chance)) // Negative powerups should spawn more often
        this->PowerUps.push_back(PowerUp("confuse", glm::vec3(1.0f, 0.3f, 0.3f), 15.0f, block.Position, ResourceManager::GetTexture("powerup_confuse")));
    if (ShouldSpawn(neg_chance))
        this->PowerUps.push_back(PowerUp("chaos", glm::vec3(0.9f, 0.25f, 0.25f), 15.0f, block.Position, ResourceManager::GetTexture("powerup_chaos")));
}

int randrange(int min, int max) // range : [min, max]
{
   return min + rand() % ((max + 1) - min);
}

void Game::ActivatePowerUp(PowerUp& powerUp, unsigned int width) {
    if(powerUp.Type == "speed")
        Ball->Velocity *= 1.2;
    else if(powerUp.Type == "sticky") {
        Ball->Sticky = true;
        Player->Color = glm::vec3(1.0f, 0.5f, 1.0f);
    } else if(powerUp.Type == "pass-through") {
        Ball->PassThrough = true;
        Ball->Color = glm::vec3(1.0f, 0.5f, 0.5f);
    } else if(powerUp.Type == "pad-size-increase" && Player->Size.x <= (width / 2)) {
        // increase size only if below limit
        Player->Size.x += 50;
    } else if(powerUp.Type == "ball-increase") {
        Ball->Radius *= 2;
        Ball->Size *= 2;
    } else if(powerUp.Type == "confuse") {
        if(!Effects->Chaos) // activate confuse if chaos is not activated
            Effects->Confuse = true;
    } else if(powerUp.Type == "chaos") {
        if(!Effects->Confuse)
            Effects->Chaos = true;
    } else if(powerUp.Type == "ball-decrease") {
        Ball->Radius /= 2;
        Ball->Size /= 2;
    } else if(powerUp.Type == "fireworks" && !will_explode) {
        // for simplicity don't allow queued explosions
        will_explode = true;
        explosionTime = explosionWait;

        // random brick selection logic - explode upto n bricks
        int n = randrange(1, 7);
        int chance = this->Levels[this->Level].Bricks.size() / n;

        for(int i = 0; i < this->Levels[this->Level].Bricks.size(); i++) {
            GameObject& brick = this->Levels[this->Level].Bricks[i];
            if(!brick.Destroyed && !brick.IsSolid && bricksToExplode.size() < n && ShouldSpawn(chance))
                bricksToExplode.push_back(&brick);
        }

    }
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
    for(GameObject& brick : Levels[Level].Bricks) {
        if(!brick.Destroyed) {
            Collision collision = CheckCollision(*Ball, brick);
            if(std::get<0>(collision)) {
                if(!brick.IsSolid) {
                    brick.Destroyed = true;
                    this->SpawnPowerUps(brick);
                    ma_sound_start(&mySounds["bleep"]);        
                } else {
                    ShakeTime = 0.05f;
                    Effects->Shake = true;
                    ma_sound_start(&mySounds["solid"]);        
                }
                
                // collision resolution
                Direction dir = std::get<1>(collision); // up, down, left or right
                glm::vec2 diff_vector = std::get<2>(collision); // difference of closest point from ball center
                
                // note: we reverse velocity components on collision
                // this does not change the speed of the ball (speed = sqrt(x^2 + y^2))

                if (!(Ball->PassThrough && !brick.IsSolid)) // don't do collision resolution on non-solid bricks if pass-through is activated
                {
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
    }
    
    // check collisions on powerups and if so activate them
    for(PowerUp& powerUp : this->PowerUps) {
        if(!powerUp.Destroyed) {
            if(powerUp.Position.y >= this->Height) {
                powerUp.Destroyed = true;
                powerUp.Activated = false;
            } else if(CheckCollision(*Player, powerUp)) {
                // collided with player, now activate powerup
                ActivatePowerUp(powerUp, this->Width);
                powerUp.Destroyed = true;
                powerUp.Activated = true;
                ma_sound_start(&mySounds["powerup"]);
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

        // if Sticky powerup is activated, also stick ball to paddle once new velocity vectors were calculated
        Ball->Stuck = Ball->Sticky;

        ma_sound_start(&mySounds["bleep"]);
    }
}