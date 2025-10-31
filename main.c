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

typedef struct Ball {
    float radius;
    Color color;
    float acceleration;
    Vector2 init_position;
    Vector2 position;
    Vector2 direction;
    float base_speed;
    float current_speed;
} Ball;

typedef struct Paddle {
    float width;
    float height;
    Color color;
    Vector2 init_position;
    Vector2 position;
    Vector2 direction;
    float speed;
} Paddle;

/*==============================================================================
    BALL FUNCTIONS
==============================================================================*/

Ball create_ball(float pos_x, float pos_y, float radius, Color color, float speed, float acceleration) {
    Ball ball;

    ball.radius = radius;
    ball.color = color;
    ball.init_position = (Vector2){pos_x, pos_y};
    ball.position = ball.init_position;
    ball.base_speed = speed;
    ball.current_speed = speed;
    ball.direction = (Vector2){1.0f, 1.0f};
    ball.acceleration = acceleration;

    normalize(&ball.direction);

    return ball;
}

void reset_ball(Ball *ball) {
    ball->position = ball->init_position;
    ball->current_speed = ball->base_speed;
    ball->direction.x *= -1;  
    normalize(&ball->direction);
}

void update_ball(Ball *ball, int *player_score, int *cpu_score) {
    float delta = GetFrameTime();

    ball->position.x += ball->current_speed * ball->direction.x * delta;
    ball->position.y += ball->current_speed * ball->direction.y * delta;

    if (ball->position.y + ball->radius >= GetScreenHeight() ||
        ball->position.y - ball->radius <= 0) {
        ball->direction.y *= -1;
        play_ball_hit(); // toca som ao bater na parede
    }

    if (ball->position.x + ball->radius >= GetScreenWidth()) {
        (*player_score)++;
        play_win();      // toca som de vitória
        reset_ball(ball);
    }
    else if (ball->position.x - ball->radius <= 0) {
        (*cpu_score)++;
        play_lose();     // toca som de derrota
        reset_ball(ball);
    }
}


void draw_ball(const Ball *ball) {
    DrawCircleV(ball->position, ball->radius, ball->color);
}

void ball_handle_collision(Ball *ball, const Paddle *paddle) {
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

Paddle create_paddle(float pos_x, float pos_y, float width, float height, Color color, float speed) {
    Paddle paddle;

    paddle.width = width;
    paddle.height = height;
    paddle.color = color;
    paddle.init_position = (Vector2){pos_x, pos_y};
    paddle.position = paddle.init_position;
    paddle.speed = speed;
    paddle.direction = (Vector2){0.0f, 0.0f};

    return paddle;
}

Rectangle paddle_get_rect(const Paddle *paddle) {
    return (Rectangle){
        paddle->position.x,
        paddle->position.y,
        paddle->width,
        paddle->height
    };
}

void draw_paddle(const Paddle *paddle) {
    DrawRectangleRec(paddle_get_rect(paddle), paddle->color);
}


void update_paddle(Paddle *paddle) {
    float delta = GetFrameTime();

    paddle->position.y += paddle->speed * paddle->direction.y * delta;

    if (paddle->position.y + paddle->height >= GetScreenHeight())
        paddle->position.y = GetScreenHeight() - paddle->height;
    if (paddle->position.y <= 0)
        paddle->position.y = 0;
}

void update_cpu_paddle(Paddle *paddle, float ball_pos_y) {
    float center_y = paddle->position.y + paddle->height / 2.0f;

    if (center_y > ball_pos_y)
        paddle->direction.y = -1;
    else
        paddle->direction.y = 1;

    update_paddle(paddle);
}

/*==============================================================================
    SFX
==============================================================================*/


Sound ball_hit_sfx;
Sound player_win_sfx;
Sound player_lose_sfx;
Music bg_music;

void audio_init() {
    InitAudioDevice();

    ball_hit_sfx   = LoadSound("C:\\Users\\ginoc\\mycode\\pong_c_raylib\\sfx\\ball_hit.wav");
    player_win_sfx = LoadSound("C:\\Users\\ginoc\\mycode\\pong_c_raylib\\sfx\\win.wav");
    player_lose_sfx = LoadSound("C:\\Users\\ginoc\\mycode\\pong_c_raylib\\sfx\\lose.wav");
    bg_music       = LoadMusicStream("C:\\Users\\ginoc\\mycode\\pong_c_raylib\\sfx\\bg.ogg");
    bg_music.looping = true;

    PlayMusicStream(bg_music);
}

static void play_ball_hit() { PlaySound(ball_hit_sfx); }
static void play_win()      { PlaySound(player_win_sfx); }
static void play_lose()     { PlaySound(player_lose_sfx); }


/*==============================================================================
    MAIN
==============================================================================*/

int main(void) {
    const int window_width = 960;
    const int window_height = 540;
    const char game_title[] = "Pong Clone (Single File, Organized)";

    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_UNDECORATED);
    InitWindow(window_width, window_height, game_title);

    // === SFX ===
    audio_init();
    // InitAudioDevice();

    // Sound ball_hit_sfx = LoadSound("C:\\Users\\ginoc\\mycode\\pong_c_raylib\\sfx\\ball_hit.ogg");
    // Sound player_win_sfx =  LoadSound("C:\\Users\\ginoc\\mycode\\pong_c_raylib\\sfx\\win.ogg");
    // Sound player_lose_sfx =  LoadSound("C:\\Users\\ginoc\\mycode\\pong_c_raylib\\sfx\\lose.ogg");
    // Music bg_music = LoadMusicStream("C:\\Users\\ginoc\\mycode\\pong_c_raylib\\sfx\\bg.ogg");
    // bg_music.looping = true;  
    // PlayMusicStream(bg_music);

    // ===========

    SetTargetFPS(240);
    DisableCursor();

    Ball ball = create_ball(window_width / 2, window_height / 2, 20, WHITE, 400, 40);
    Paddle player = create_paddle(10, window_height / 2 - 60, 20, 120, RAYWHITE, 400);
    Paddle cpu = create_paddle(window_width - 30, window_height / 2 - 60, 20, 120, RAYWHITE, 400);

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

    CloseWindow();
    return 0;
}
