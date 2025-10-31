// gunfight.c
// Compile with: gcc gunfight.c -o gunfight -lraylib -lopengl32 -lgdi32 -lwinmm
#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

/*==============================================================================
    CONFIG
==============================================================================*/
#define SCREEN_W  1024
#define SCREEN_H  640

#define PLAYER_START_BULLETS 5
#define CPU_START_BULLETS    5
#define START_LIVES          1

#define MAX_BULLETS 64
#define MAX_OBSTACLES 2

/*==============================================================================
    HELPERS
==============================================================================*/
static inline float signf(float v) { return (v > 0) - (v < 0); }

static void vec2_normalize(Vector2 *v) {
    float mag = sqrtf(v->x * v->x + v->y * v->y);
    if (mag > 0.0f) { v->x /= mag; v->y /= mag; }
}

/*==============================================================================
    TYPES
==============================================================================*/
typedef enum {
    OWNER_PLAYER,
    OWNER_CPU
} BulletOwner;

typedef struct {
    bool active;
    Vector2 pos;
    Vector2 dir;   // normalized
    float speed;
    float radius;
    BulletOwner owner;
} Bullet;

typedef struct {
    float width;
    float height;
    Color color;
    Vector2 position;   // top-left
    Vector2 dir;        // vertical direction normalized Y (-1 or +1)
    float speed;        // current speed (pixels/s)
    float accel;        // amount to increase speed when a shot is fired
} Obstacle;

typedef struct {
    Vector2 pos;
    Vector2 vel;  // velocity (for movement inputs)
    float radius;
    Color color;
    int bullets_left;
    int lives;
    float speed_x; // pixels / second
    float speed_y;
    Rectangle area; // allowed area for movement
} PlayerEntity;

/*==============================================================================
    GAME STATE
==============================================================================*/
static Bullet bullets[MAX_BULLETS];
static Obstacle obstacles[MAX_OBSTACLES];

static PlayerEntity player;
static PlayerEntity cpu_entity; // keep same fields for symmetry

static bool game_over = false;
static float cpu_shot_timer = 0.0f;
static float cpu_shot_interval_min = 1.0f;
static float cpu_shot_interval_max = 2.5f;

/*==============================================================================
    GAME LOGIC / UTIL
==============================================================================*/
static void bullets_init(void) {
    for (int i = 0; i < MAX_BULLETS; ++i) bullets[i].active = false;
}

static int spawn_bullet(Vector2 pos, Vector2 dir, float speed, float radius, BulletOwner owner) {
    // find inactive
    for (int i = 0; i < MAX_BULLETS; ++i) {
        if (!bullets[i].active) {
            bullets[i].active = true;
            bullets[i].pos = pos;
            bullets[i].dir = dir;
            vec2_normalize(&bullets[i].dir);
            bullets[i].speed = speed;
            bullets[i].radius = radius;
            bullets[i].owner = owner;
            return i;
        }
    }
    return -1; // none available
}

static void spawn_player_shot(void) {
    if (player.bullets_left <= 0) return;

    // bullet initial direction: to the right, with slight random vertical component
    Vector2 dir = { 1.0f, (GetRandomValue(-30,30) / 100.0f) };
    vec2_normalize(&dir);
    Vector2 spawn = { player.pos.x + player.radius + 4.0f, player.pos.y };

    int idx = spawn_bullet(spawn, dir, 450.0f, player.radius * 0.45f, OWNER_PLAYER);
    if (idx >= 0) {
        player.bullets_left--;
        // accelerate obstacles
        for (int o = 0; o < MAX_OBSTACLES; ++o) obstacles[o].speed += obstacles[o].accel;
    }
}

static void spawn_cpu_shot(void) {
    if (cpu_entity.bullets_left <= 0) return;

    Vector2 dir = { -1.0f, (GetRandomValue(-30,30) / 100.0f) };
    vec2_normalize(&dir);
    Vector2 spawn = { cpu_entity.pos.x - cpu_entity.radius - 4.0f, cpu_entity.pos.y };

    int idx = spawn_bullet(spawn, dir, 420.0f, cpu_entity.radius * 0.5f, OWNER_CPU);
    if (idx >= 0) {
        cpu_entity.bullets_left--;
        for (int o = 0; o < MAX_OBSTACLES; ++o) obstacles[o].speed += obstacles[o].accel;
    }
}

/*==============================================================================
    COLLISIONS
==============================================================================*/
static bool circle_vs_rect_collision(Vector2 cpos, float cr, Rectangle r) {
    // raylib has CheckCollisionCircleRec but to keep logic here, do same:
    float closestX = fmaxf(r.x, fminf(cpos.x, r.x + r.width));
    float closestY = fmaxf(r.y, fminf(cpos.y, r.y + r.height));
    float dx = cpos.x - closestX;
    float dy = cpos.y - closestY;
    return (dx*dx + dy*dy) <= (cr*cr);
}

/*==============================================================================
    INIT
==============================================================================*/
static void init_game_state(void) {
    bullets_init();
    game_over = false;

    // Player area: left third of screen, draw vertical line at area.x + area.width
    player.radius = 18.0f;
    player.color = BLUE;
    float area_x = 40.0f;
    float area_w = (float)SCREEN_W * 0.28f; // player X movement allowed roughly up to ~28% of width
    player.area = (Rectangle){ area_x, 40.0f, area_w, (float)SCREEN_H - 80.0f };
    // start near center of that area
    player.pos = (Vector2){ player.area.x + player.radius + 8.0f, player.area.y + player.area.height*0.5f };
    player.speed_x = 260.0f;
    player.speed_y = 260.0f;
    player.bullets_left = PLAYER_START_BULLETS;
    player.lives = START_LIVES;

    // CPU (mirror on right)
    cpu_entity.radius = 18.0f;
    cpu_entity.color = RED;
    cpu_entity.speed = 160.0f; // vertical patrol speed base
    cpu_entity.pos = (Vector2){ (float)SCREEN_W - 40.0f - cpu_entity.radius, SCREEN_H * 0.5f };
    cpu_entity.bullets_left = CPU_START_BULLETS;
    cpu_entity.lives = START_LIVES;
    // CPU uses area only for limiting vertical (not X)
    cpu_entity.area = (Rectangle){ (float)SCREEN_W * 0.6f, 40.0f, (float)SCREEN_W * 0.35f, (float)SCREEN_H - 80.0f };

    // Obstacles: two central vertical paddles moving up/down
    for (int i = 0; i < MAX_OBSTACLES; ++i) {
        obstacles[i].width = 20.0f;
        obstacles[i].height = 120.0f;
        obstacles[i].color = GRAY;
        // position them in the middle area: one slightly left center, one slightly right center
        float cx = SCREEN_W * 0.45f + (i == 0 ? -80.0f : 80.0f);
        obstacles[i].position.x = cx - obstacles[i].width * 0.5f;
        obstacles[i].position.y = SCREEN_H * 0.5f - obstacles[i].height * 0.5f;
        obstacles[i].dir.y = (i % 2 == 0) ? 1.0f : -1.0f; // alternate directions
        obstacles[i].speed = 80.0f + i * 40.0f; // different base speeds
        obstacles[i].accel = 8.0f + i * 2.0f;   // each shot increases speed by this
    }

    // CPU shot timer initial
    cpu_shot_timer = (float)GetRandomValue((int)(cpu_shot_interval_min*1000),(int)(cpu_shot_interval_max*1000)) / 1000.0f;
}

/*==============================================================================
    UPDATES
==============================================================================*/
static void update_player(float dt) {
    // input: WASD and arrows
    float dx = 0.0f, dy = 0.0f;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) dx = -1.0f;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) dx = 1.0f;
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) dy = -1.0f;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) dy = 1.0f;

    player.pos.x += dx * player.speed_x * dt;
    player.pos.y += dy * player.speed_y * dt;

    // clamp inside player.area (note: using center position)
    float half = player.radius;
    if (player.pos.x - half < player.area.x) player.pos.x = player.area.x + half;
    if (player.pos.x + half > player.area.x + player.area.width) player.pos.x = player.area.x + player.area.width - half;
    if (player.pos.y - half < player.area.y) player.pos.y = player.area.y + half;
    if (player.pos.y + half > player.area.y + player.area.height) player.pos.y = player.area.y + player.area.height - half;
}

static void update_cpu(float dt) {
    // CPU moves only vertical inside its area top/bottom
    float half = cpu_entity.radius;
    cpu_entity.pos.y += cpu_entity.speed * cpu_entity.direction.y * dt;

    if (cpu_entity.pos.y - half < cpu_entity.area.y) {
        cpu_entity.pos.y = cpu_entity.area.y + half;
        cpu_entity.direction.y *= -1.0f;
    } else if (cpu_entity.pos.y + half > cpu_entity.area.y + cpu_entity.area.height) {
        cpu_entity.pos.y = cpu_entity.area.y + cpu_entity.area.height - half;
        cpu_entity.direction.y *= -1.0f;
    }

    // CPU shot timer
    cpu_shot_timer -= dt;
    if (cpu_shot_timer <= 0.0f) {
        // shoot if has bullets
        if (cpu_entity.bullets_left > 0) spawn_cpu_shot();
        // reset timer
        cpu_shot_timer = (float)GetRandomValue((int)(cpu_shot_interval_min*1000),(int)(cpu_shot_interval_max*1000)) / 1000.0f;
    }
}

static void update_obstacles(float dt) {
    for (int i = 0; i < MAX_OBSTACLES; ++i) {
        Obstacle *o = &obstacles[i];
        o->position.y += o->speed * o->dir.y * dt;
        // bounce top/bottom region (leave some margin)
        if (o->position.y < 60.0f) {
            o->position.y = 60.0f;
            o->dir.y *= -1.0f;
        } else if (o->position.y + o->height > SCREEN_H - 60.0f) {
            o->position.y = SCREEN_H - 60.0f - o->height;
            o->dir.y *= -1.0f;
        }
    }
}

static void update_bullets(float dt) {
    for (int i = 0; i < MAX_BULLETS; ++i) {
        if (!bullets[i].active) continue;
        Bullet *b = &bullets[i];

        // move
        b->pos.x += b->dir.x * b->speed * dt;
        b->pos.y += b->dir.y * b->speed * dt;

        // bounce top/bottom
        if (b->pos.y - b->radius <= 0.0f) {
            b->pos.y = b->radius; b->dir.y *= -1.0f;
        } else if (b->pos.y + b->radius >= SCREEN_H) {
            b->pos.y = SCREEN_H - b->radius; b->dir.y *= -1.0f;
        }

        // collide with obstacles
        bool destroyed = false;
        for (int o = 0; o < MAX_OBSTACLES; ++o) {
            Rectangle rect = (Rectangle){ obstacles[o].position.x, obstacles[o].position.y, obstacles[o].width, obstacles[o].height };
            if (circle_vs_rect_collision(b->pos, b->radius, rect)) {
                // bullet disappears
                b->active = false;
                destroyed = true;
                break;
            }
        }
        if (destroyed) continue;

        // check lateral limits -> disappear
        if (b->pos.x + b->radius < 0.0f || b->pos.x - b->radius > SCREEN_W) {
            b->active = false;
            continue;
        }

        // collision with targets (player or CPU)
        if (b->owner == OWNER_PLAYER) {
            // can hit cpu
            float dist2 = (b->pos.x - cpu_entity.pos.x)*(b->pos.x - cpu_entity.pos.x) + (b->pos.y - cpu_entity.pos.y)*(b->pos.y - cpu_entity.pos.y);
            float rsum = b->radius + cpu_entity.radius;
            if (dist2 <= rsum*rsum) {
                // hit cpu
                b->active = false;
                if (cpu_entity.lives > 0) cpu_entity.lives -= 1;
            }
        } else { // OWNER_CPU
            float dist2 = (b->pos.x - player.pos.x)*(b->pos.x - player.pos.x) + (b->pos.y - player.pos.y)*(b->pos.y - player.pos.y);
            float rsum = b->radius + player.radius;
            if (dist2 <= rsum*rsum) {
                b->active = false;
                if (player.lives > 0) player.lives -= 1;
            }
        }
    }
}

/*==============================================================================
    GAME CHECKS
==============================================================================*/
static void check_game_over(void) {
    // game ends when both have no bullets left OR lives reached 0 for one?
    // Requirements: "when the bullets are finished, the game ends, and who has bigger life wins"
    if (player.bullets_left <= 0 && cpu_entity.bullets_left <= 0) game_over = true;
    // Also end early if lives both zero or one zero and both out? We'll keep bullets rule primary.
}

/*==============================================================================
    RENDER
==============================================================================*/
static void draw_ui(void) {
    DrawText("Gun Fight (Atari-like) - Left=Player, Right=CPU", 14, 8, 18, LIGHTGRAY);

    // area boundary line
    DrawText(FormatText("PLAYER AREA (X min=%.0f, max=%.0f)", player.area.x, player.area.x + player.area.width), 14, 28, 12, GRAY);
    DrawLine((int)(player.area.x + player.area.width), 40, (int)(player.area.x + player.area.width), SCREEN_H - 40, DARKGRAY);

    // bullets/lives
    DrawText(FormatText("Player bullets: %d", player.bullets_left), 14, SCREEN_H - 60, 18, SKYBLUE);
    DrawText(FormatText("Player lives: %d", player.lives), 14, SCREEN_H - 36, 18, SKYBLUE);

    DrawText(FormatText("CPU bullets: %d", cpu_entity.bullets_left), SCREEN_W - 220, SCREEN_H - 60, 18, PINK);
    DrawText(FormatText("CPU lives: %d", cpu_entity.lives), SCREEN_W - 220, SCREEN_H - 36, 18, PINK);

    DrawFPS(SCREEN_W - 90, 8);

    if (game_over) {
        const char *msg;
        if (player.lives > cpu_entity.lives) msg = "PLAYER WINS!";
        else if (player.lives < cpu_entity.lives) msg = "CPU WINS!";
        else msg = "DRAW!";
        int fontSize = 42;
        int tw = MeasureText(msg, fontSize);
        DrawText(msg, SCREEN_W/2 - tw/2, SCREEN_H/2 - 40, fontSize, YELLOW);
        DrawText("Press R to restart or ESC to quit", SCREEN_W/2 - 200, SCREEN_H/2 + 8, 20, LIGHTGRAY);
    }
}

static void draw_game(void) {
    // background
    ClearBackground(BLACK);

    // obstacles
    for (int i = 0; i < MAX_OBSTACLES; ++i) {
        DrawRectangleRec((Rectangle){obstacles[i].position.x, obstacles[i].position.y, obstacles[i].width, obstacles[i].height}, obstacles[i].color);
    }

    // bullets
    for (int i = 0; i < MAX_BULLETS; ++i) {
        if (!bullets[i].active) continue;
        DrawCircleV(bullets[i].pos, (int)bullets[i].radius, WHITE);
    }

    // player & cpu
    DrawCircleV(player.pos, (int)player.radius, player.color);
    DrawCircleLines((int)cpu_entity.pos.x, (int)cpu_entity.pos.y, cpu_entity.radius, DARKBLUE); // ring for CPU
    DrawCircleV(cpu_entity.pos, (int)cpu_entity.radius, cpu_entity.color);

    // UI overlay
    draw_ui();
}

/*==============================================================================
    RESTART
==============================================================================*/
static void restart_game(void) {
    init_game_state();
    // also clear bullets
    bullets_init();
}

/*==============================================================================
    MAIN
==============================================================================*/
int main(void) {
    SetConfigFlags(FLAG_VSYNC_HINT);
    InitWindow(SCREEN_W, SCREEN_H, "Gun Fight - Atari-like (raylib)");
    SetTargetFPS(60);

    init_game_state();

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        // input for shooting
        if (!game_over) {
            if (IsKeyPressed(KEY_SPACE)) {
                spawn_player_shot();
            }

            // update player and CPU
            update_player(dt);
            update_cpu(dt);
            update_obstacles(dt);
            update_bullets(dt);
            check_game_over();
        } else {
            // when game over, allow restart
            if (IsKeyPressed(KEY_R)) {
                restart_game();
                game_over = false;
            }
        }

        // drawing
        BeginDrawing();
            draw_game();
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
