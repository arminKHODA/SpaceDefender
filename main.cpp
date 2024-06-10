#include <SDL.h>
#include <SDL_ttf.h>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <sstream>

// Screen dimensions
const int SCREEN_WIDTH = 1920;
const int SCREEN_HEIGHT = 1080;

// Player dimensions
const int PLAYER_WIDTH = 20;
const int PLAYER_HEIGHT = 40;

// Bullet speed
const int BULLET_SPEED = 5;

// Enemy speed range (set as floating-point values)
const float ENEMY_SPEED_MIN = 0.02f;
const float ENEMY_SPEED_MAX = 0.1f;

// Trail length
const int TRAIL_LENGTH = 20;

// Enemy spawn rate (increased by factor of 5)
const int ENEMY_SPAWN_RATE = 200 * 5;

// Player movement speed
const int PLAYER_SPEED = 20;

// Minimum and maximum enemy size
const int ENEMY_MIN_SIZE = 5;
const int ENEMY_MAX_SIZE = 20;

struct TrailPoint
{
    int x, y;
};

struct Bullet
{
    int x, y;
    bool active;
    std::vector<TrailPoint> trail;
};

struct Enemy
{
    float x, y;
    int width, height;
    float speed;
    bool active;
};

void RenderText(SDL_Renderer* renderer, TTF_Font* font, const std::string& text, int x, int y, SDL_Color color)
{
    SDL_Surface* surface = TTF_RenderText_Solid(font, text.c_str(), color);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect destRect = { x, y, surface->w, surface->h };
    SDL_RenderCopy(renderer, texture, nullptr, &destRect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

void DrawSpaceship(SDL_Renderer* renderer, int x, int y)
{
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    // Draw the main body
    SDL_Rect body = { x, y, PLAYER_WIDTH, PLAYER_HEIGHT };
    SDL_RenderFillRect(renderer, &body);

    // Draw the left wing
    SDL_Rect leftWing = { x - PLAYER_WIDTH / 2, y + PLAYER_HEIGHT / 3, PLAYER_WIDTH / 2, PLAYER_HEIGHT / 3 };
    SDL_RenderFillRect(renderer, &leftWing);

    // Draw the right wing
    SDL_Rect rightWing = { x + PLAYER_WIDTH, y + PLAYER_HEIGHT / 3, PLAYER_WIDTH / 2, PLAYER_HEIGHT / 3 };
    SDL_RenderFillRect(renderer, &rightWing);

    // Draw the cockpit (a smaller rectangle on top)
    SDL_Rect cockpit = { x + PLAYER_WIDTH / 4, y - PLAYER_HEIGHT / 3, PLAYER_WIDTH / 2, PLAYER_HEIGHT / 3 };
    SDL_RenderFillRect(renderer, &cockpit);
}

int main(int argc, char* argv[])
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        std::cerr << "Could not initialize SDL: " << SDL_GetError() << std::endl;
        return 1;
    }

    if (TTF_Init() < 0)
    {
        std::cerr << "Could not initialize SDL_ttf: " << TTF_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Move the Spaceship",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH, SCREEN_HEIGHT,
        SDL_WINDOW_SHOWN);
    if (!window)
    {
        std::cerr << "Could not create window: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer)
    {
        std::cerr << "Could not create renderer: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    TTF_Font* font = TTF_OpenFont("font.ttf", 24); // Provide the correct path to a TTF font file
    if (!font)
    {
        std::cerr << "Could not load font: " << TTF_GetError() << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Initial position of the spaceship in the center of the screen
    int shipX = (SCREEN_WIDTH - PLAYER_WIDTH) / 2;
    int shipY = ((SCREEN_HEIGHT - PLAYER_HEIGHT) / 2) + 400;

    std::vector<Bullet> bullets;
    std::vector<Enemy> enemies;

    int score = 0;
    bool running = true;
    bool gameStarted = false;
    SDL_Event event;
    srand(static_cast<unsigned int>(time(0)));

    // Game loop
    while (running)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                running = false;
            }
            else if (event.type == SDL_KEYDOWN)
            {
                if (!gameStarted && event.key.keysym.sym == SDLK_RETURN)
                {
                    gameStarted = true;
                }
                else if (gameStarted)
                {
                    switch (event.key.keysym.sym)
                    {
                    case SDLK_UP:
                        shipY -= PLAYER_SPEED;
                        break;
                    case SDLK_DOWN:
                        shipY += PLAYER_SPEED;
                        break;
                    case SDLK_LEFT:
                        shipX -= PLAYER_SPEED;
                        break;
                    case SDLK_RIGHT:
                        shipX += PLAYER_SPEED;
                        break;
                    case SDLK_SPACE:
                        bullets.push_back({ shipX + PLAYER_WIDTH / 2, shipY, true, {} });
                        break;
                    }
                }
            }
        }

        if (!gameStarted)
        {
            // Render start screen
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);
            RenderText(renderer, font, "Press Enter to Start", SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2, { 255, 255, 255, 255 });
            SDL_RenderPresent(renderer);
            continue;
        }

        // Update bullets
        for (auto& bullet : bullets)
        {
            if (bullet.active)
            {
                // Add current position to trail
                bullet.trail.push_back({ bullet.x, bullet.y });
                if (bullet.trail.size() > TRAIL_LENGTH)
                {
                    bullet.trail.erase(bullet.trail.begin());
                }

                bullet.y -= BULLET_SPEED;
                if (bullet.y < 0)
                {
                    bullet.active = false; // Deactivate the bullet if it goes off-screen
                }
            }
        }

        // Spawn enemies
        if (rand() % ENEMY_SPAWN_RATE == 0)
        {
            int enemyX = rand() % (SCREEN_WIDTH - ENEMY_MAX_SIZE);
            int enemySize = ENEMY_MIN_SIZE + rand() % (ENEMY_MAX_SIZE - ENEMY_MIN_SIZE + 1);
            float enemySpeed = ENEMY_SPEED_MIN + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (ENEMY_SPEED_MAX - ENEMY_SPEED_MIN)));
            enemies.push_back({ static_cast<float>(enemyX), 0.0f, enemySize, enemySize, enemySpeed, true });
        }

        // Update enemies
        for (auto& enemy : enemies)
        {
            if (enemy.active)
            {
                enemy.y += enemy.speed;
                if (enemy.y > SCREEN_HEIGHT)
                {
                    enemy.active = false; // Deactivate the enemy if it goes off-screen
                }
            }
        }

        // Check for collisions
        SDL_Rect playerRect = { shipX, shipY, PLAYER_WIDTH, PLAYER_HEIGHT };
        for (auto& enemy : enemies)
        {
            if (enemy.active)
            {
                SDL_Rect enemyRect = { static_cast<int>(enemy.x), static_cast<int>(enemy.y), enemy.width, enemy.height };
                for (auto& bullet : bullets)
                {
                    if (bullet.active)
                    {
                        SDL_Rect bulletRect = { bullet.x, bullet.y, 5, 10 };
                        if (SDL_HasIntersection(&bulletRect, &enemyRect))
                        {
                            enemy.active = false;
                            bullet.active = false;
                            score++;
                            break;
                        }
                    }
                }

                if (SDL_HasIntersection(&playerRect, &enemyRect))
                {
                    running = false;
                }
            }
        }

        // Clear the screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Draw the spaceship
        DrawSpaceship(renderer, shipX, shipY);

        // Draw bullets and their trails
        for (const auto& bullet : bullets)
        {
            if (bullet.active)
            {
                // Draw trail
                for (int i = 0; i < static_cast<int>(bullet.trail.size()); ++i)
                {
                    // Fade effect for the trail
                    int alpha = 255 * (i + 1) / bullet.trail.size();
                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, alpha);
                    SDL_Rect trailRect = { bullet.trail[i].x, bullet.trail[i].y, 5, 10 };
                    SDL_RenderFillRect(renderer, &trailRect);
                }

                // Draw bullet
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                SDL_Rect bulletRect = { bullet.x, bullet.y, 5, 10 };
                SDL_RenderFillRect(renderer, &bulletRect);
            }
        }

        // Draw enemies
        for (const auto& enemy : enemies)
        {
            if (enemy.active)
            {
                SDL_Rect enemyRect = { static_cast<int>(enemy.x), static_cast<int>(enemy.y), enemy.width, enemy.height };
                SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
                SDL_RenderFillRect(renderer, &enemyRect);
            }
        }

        // Render score
        std::stringstream scoreText;
        scoreText << "Score: " << score;
        RenderText(renderer, font, scoreText.str(), 10, 10, { 255, 255, 255, 255 });

        // Update the screen
        SDL_RenderPresent(renderer);
    }

    // Display Game Over message and keep the screen open
    while (true)
    {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        RenderText(renderer, font, "Game Over! Press ESC to Exit", SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2, { 255, 255, 255, 255 });
        RenderText(renderer, font, "Final Score: " + std::to_string(score), SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 + 50, { 255, 255, 255, 255 });
        SDL_RenderPresent(renderer);

        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                SDL_DestroyRenderer(renderer);
                SDL_DestroyWindow(window);
                SDL_Quit();
                return 0;
            }
            else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)
            {
                SDL_DestroyRenderer(renderer);
                SDL_DestroyWindow(window);
                SDL_Quit();
                return 0;
            }
        }
    }

    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
