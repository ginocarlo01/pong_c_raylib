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
    SFX
==============================================================================*/
#include "raylib.h"

static Sound ball_hit_sfx;
static Sound player_win_sfx;
static Sound player_lose_sfx;
static Music bg_music;

void audio_init() {
    InitAudioDevice();

    ball_hit_sfx   = LoadSound("C:\\Users\\ginoc\\mycode\\pong_c_raylib\\sfx\\ball_hit.wav");
    player_win_sfx = LoadSound("C:\\Users\\ginoc\\mycode\\pong_c_raylib\\sfx\\win.wav");
    player_lose_sfx = LoadSound("C:\\Users\\ginoc\\mycode\\pong_c_raylib\\sfx\\lose.wav");

    bg_music = LoadMusicStream("C:\\Users\\ginoc\\mycode\\pong_c_raylib\\sfx\\bg.ogg");
    bg_music.looping = true;

    PlayMusicStream(bg_music);
}


void audio_unload() {
    UnloadSound(ball_hit_sfx);
    UnloadSound(player_win_sfx);
    UnloadSound(player_lose_sfx);
    UnloadMusicStream(bg_music);
    CloseAudioDevice();
}

void play_ball_hit(void) { PlaySound(ball_hit_sfx); }
void play_win(void)      { PlaySound(player_win_sfx); }
void play_lose(void)     { PlaySound(player_lose_sfx); }


/*==============================================================================
    STRUCTS
==============================================================================*/

typedef enum {
    ENTITY_PLAYER,   
    ENTITY_BALL,     
    ENTITY_CPU
} EntityType;

typedef struct Entity {
	float radius;
    float width;
    float height;
    EntityType type;
    Vector2 init_position;
    Vector2 position;
    Vector2 direction;
    float base_speed;
    float current_speed;
    float acceleration;
    Color color;
} Entity;


/*==============================================================================
    BALL FUNCTIONS
==============================================================================*/

Entity create_entity(EntityType type, float pos_x, float pos_y, Color color, 
                     float radius, float width, float height, 
                     float speed, float acceleration) {
    Entity e = {0}; 

    e.type = type;
    e.init_position = (Vector2){pos_x, pos_y};
    e.position = e.init_position;
    e.color = color;

    e.radius = radius;         
    e.width = width;           
    e.height = height;         

    e.base_speed = speed;
    e.current_speed = speed;
    e.acceleration = acceleration;

    if (type == ENTITY_BALL) {
        e.direction = (Vector2){1.0f, 1.0f};
        normalize(&e.direction);
    } else {
        e.direction = (Vector2){0.0f, 0.0f};
    }

    return e;
}

void reverse_direction_entity(Entity *entity){
    entity->direction.x *= -1;  
    normalize(&entity->direction);
}

void reset_entity(Entity *entity) {
    entity->position = entity->init_position;
    entity->current_speed = entity->base_speed;
    reverse_direction_entity(entity);
}

void update_ball(Entity *ball, int *player_score, int *cpu_score) {
    float delta = GetFrameTime();

    ball->position.x += ball->current_speed * ball->direction.x * delta;
    ball->position.y += ball->current_speed * ball->direction.y * delta;

    if (ball->position.y + ball->radius >= GetScreenHeight() ||
        ball->position.y - ball->radius <= 0) {
        ball->direction.y *= -1;
        play_ball_hit(); 
    }

    if (ball->position.x + ball->radius >= GetScreenWidth()) {
        (*player_score)++;
        play_win();      
        reset_entity(ball);
    }
    else if (ball->position.x - ball->radius <= 0) {
        (*cpu_score)++;
        play_lose();     
        reset_entity(ball);
    }
}

static void draw_ball(const Entity *e) {
    DrawCircleV(e->position, e->radius, e->color);
}

static void draw_paddle(const Entity *e) {
    Rectangle rect = {
        e->position.x,
        e->position.y,
        e->width,
        e->height
    };
    DrawRectangleRec(rect, e->color);
}

void draw_entity(const Entity *entity) {
    switch (entity->type) {
        case ENTITY_BALL:   draw_ball(entity);   break;
        case ENTITY_PLAYER:
        case ENTITY_CPU:    draw_paddle(entity); break;
        default: break;
    }
}


void ball_handle_collision(Entity *ball, const Entity *paddle) {
    Rectangle paddle_rect = (Rectangle){
        paddle->position.x,
        paddle->position.y,
        paddle->width,
        paddle->height
    };

    if (CheckCollisionCircleRec(ball->position, ball->radius, paddle_rect)) {
        ball->direction.x *= -1;

        float paddle_center_y = paddle->position.y + paddle->height / 2.0f;
        float hit_offset = (ball->position.y - paddle_center_y) / (paddle->height / 2.0f);

        ball->direction.y = hit_offset;
        ball->current_speed += ball->acceleration;

        normalize(&ball->direction);

        play_ball_hit();
    }
}


/*==============================================================================
    PADDLE FUNCTIONS
==============================================================================*/


void update_paddle(Entity *paddle) {
    float delta = GetFrameTime();

    paddle->position.y += paddle->current_speed * paddle->direction.y * delta;

    if (paddle->position.y + paddle->height >= GetScreenHeight())
        paddle->position.y = GetScreenHeight() - paddle->height;
    if (paddle->position.y <= 0)
        paddle->position.y = 0;
}

void update_cpu_paddle(Entity *paddle, float ball_pos_y) {
    float center_y = paddle->position.y + paddle->height / 2.0f;

    if (center_y > ball_pos_y)
        paddle->direction.y = -1;
    else
        paddle->direction.y = 1;

    update_paddle(paddle);
}


/*==============================================================================
    MAIN
==============================================================================*/

int main(void) {

    const int window_width = 960;
    const int window_height = 540;
    const char game_title[] = "Pong Clone (Single File, Organized)";

    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_UNDECORATED);
    InitWindow(window_width, window_height, game_title);

    audio_init();
    
    SetTargetFPS(240);
    DisableCursor();

    Entity ball = create_entity(ENTITY_BALL, window_width / 2, window_height / 2, WHITE, 20, 0, 0, 400, 40);
    Entity player = create_entity(ENTITY_PLAYER, 10, window_height / 2 - 60, RAYWHITE, 0, 20, 120, 400, 0);
    Entity cpu = create_entity(ENTITY_CPU, window_width - 30, window_height / 2 - 60, RAYWHITE, 0, 20, 120, 400, 0);

    int player_score = 0;
    int cpu_score = 0;

    while (!WindowShouldClose()) {
        // === INPUT ===
        if (IsKeyDown(KEY_UP)) player.direction.y = -1;
        else if (IsKeyDown(KEY_DOWN)) player.direction.y = 1;
        else player.direction.y = 0;

        // === LOGIC ===
        update_paddle(&player);
        update_cpu_paddle(&cpu, ball.position.y);
        update_ball(&ball, &player_score, &cpu_score);
        ball_handle_collision(&ball, &player);
        ball_handle_collision(&ball, &cpu);

        // === RENDER ===
        BeginDrawing();
        ClearBackground(BLACK);

        draw_ball(&ball);
        draw_paddle(&player);
        draw_paddle(&cpu);

        DrawText(TextFormat("%d", player_score), window_width / 4, 20, 40, RAYWHITE);
        DrawText(TextFormat("%d", cpu_score), window_width * 3 / 4, 20, 40, RAYWHITE);

        EndDrawing();

        // === SFX ===
        UpdateMusicStream(bg_music);
    }
    
    audio_unload();
    CloseWindow();
    return 0;
}
