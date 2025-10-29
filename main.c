#include "raylib.h"
#include <stdio.h> 

typedef struct Ball {
    float radius;
    Color color;
    float pos_x;
    float pos_y;
    float speed_x;
    float speed_y;
} Ball;

void draw_ball(Ball ball) {
    DrawCircle(ball.pos_x, ball.pos_y, ball.radius, ball.color);
}

void update_ball(Ball *ball) {
    float delta_time = GetFrameTime();
    ball->pos_x += ball->speed_x * delta_time;
    
    ball->pos_y += ball->speed_y * delta_time;
    
    if(ball->pos_y + ball->radius >= GetScreenHeight() || ball->pos_y - ball->radius <= 0){
        ball->speed_y *= -1;
    }
    
}

typedef struct Paddle {
    float width;
    float height;
    Color color;
    float pos_x;
    float pos_y;
    float speed_x;
    float speed_y;
} Paddle;

void draw_paddle(Paddle paddle) {
    DrawRectangle(paddle.pos_x, paddle.pos_y, paddle.width, paddle.height, paddle.color);
}

void update_paddle(Paddle *paddle) {
    float delta_time = GetFrameTime();
    paddle->pos_x += paddle->speed_x * delta_time;
    
    paddle->pos_y += paddle->speed_y * delta_time;
    
    if(paddle->pos_y + paddle->height/2 >= GetScreenHeight()){
        paddle->pos_y = GetScreenHeight() - paddle->height/2;
    }
    if(paddle->pos_y - paddle->height/2 <= 0){
        paddle->pos_y = paddle->height/2;
    }
    
}

int main(void){
    printf("Hello World");
    
    const int window_width = 1920 / 2;
    const int window_height = 1080 / 2;
    const char game_title[] = "game title";
    
    InitWindow(window_width, window_height, game_title);
    
    SetTargetFPS(240);
    DisableCursor();
    SetConfigFlags( FLAG_MSAA_4X_HINT | FLAG_WINDOW_UNDECORATED );
    
    Ball ball = {
        .radius = 20,
        .pos_x = window_width / 2,
        .pos_y = window_height / 2,
        .speed_x = 100,
        .speed_y = 100,
        .color = RAYWHITE
    };
    
    Paddle paddle = {
        .height = 120,
        .width = 20,
        .pos_x = 10,
        .pos_y = window_height / 2 - 60,
        .speed_x = 0,
        .speed_y = 100,
        .color = RAYWHITE
    };
    
    while ( !WindowShouldClose() ) { 
        
        
        update_ball(&ball);
        
        BeginDrawing();
        ClearBackground(BLACK);
        
        draw_ball(ball);
        draw_paddle(paddle);
        //DrawCircle(window_width / 2, window_height / 2, 20, RAYWHITE);
        //DrawRectangle(10, window_height/2 - 60, 20, 120, RAYWHITE);
        
        EndDrawing();
    }

    CloseWindow();
    return 0;
}