#include "raylib.h"
#include <stdio.h>
#include <math.h>

void normalize(Vector2 *v) {
    float mag = sqrtf(v->x * v->x + v->y * v->y);
    if (mag != 0.0f) {
        v->x /= mag;
        v->y /= mag;
    }
}

/*==============================================================================
    STRUCTS
==============================================================================*/

typedef struct {
    bool active;
    Vector2 position;
    Vector2 direction;   // normalized
    float speed;
    float radius;
    Color color;
    Shooter bullet_owner;
} Bullet;

typedef struct {

    Vector2 position;
    Vector2 direction;   // normalized
    float speed;
    float radius;
    Color color;
} Shooter;

typedef struct {

    Vector2 position;
    Vector2 direction;   // normalized
    float speed;
    float width;
    float height;
    Color color;
} Shooter;

void draw_bullet(const Ball *ball) {
    DrawCircleV(ball->position, ball->radius, ball->color);
}

void draw_shooter(const Ball *ball) {
    DrawCircleV(ball->position, ball->radius, ball->color);
}

void draw_obstacle(const Ball *ball) {
    DrawCircleV(ball->position, ball->radius, ball->color);
}

int main(void) {
    const int window_width = 960;
    const int window_height = 540;
    const char game_title[] = "Pong Clone (Single File, Organized)";

    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_UNDECORATED);
    InitWindow(window_width, window_height, game_title);
    SetTargetFPS(240);
    DisableCursor();

    while (!WindowShouldClose()) {
        // === INPUT ===

        // === LOGIC ===

        // === RENDER ===

        BeginDrawing();
        ClearBackground(BLACK);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}