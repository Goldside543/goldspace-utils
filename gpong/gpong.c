#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#define WIDTH 40
#define HEIGHT 20
#define PADDLE_HEIGHT 4
#define PADDLE_WIDTH 1
#define BALL_SIZE 1

typedef struct {
    int x, y;
} Point;

typedef struct {
    Point pos;
    int height, width;
} Paddle;

typedef struct {
    Point pos;
    Point vel;
} Ball;

void draw_paddle(Paddle *paddle) {
    for (int i = 0; i < paddle->height; i++) {
        printf("\033[%d;%dH#", paddle->pos.y + i, paddle->pos.x);
    }
}

void draw_ball(Ball *ball) {
    printf("\033[%d;%dH*", ball->pos.y, ball->pos.x);
}

void clear_screen() {
    printf("\033[H\033[J");
}

void move_paddle(Paddle *paddle, int dy) {
    paddle->pos.y += dy;
    if (paddle->pos.y < 1) paddle->pos.y = 1;
    if (paddle->pos.y + paddle->height > HEIGHT - 1) paddle->pos.y = HEIGHT - 1 - paddle->height;
}

void move_ball(Ball *ball) {
    ball->pos.x += ball->vel.x;
    ball->pos.y += ball->vel.y;

    if (ball->pos.y <= 1 || ball->pos.y >= HEIGHT - 1) ball->vel.y = -ball->vel.y;
}

bool check_collision(Ball *ball, Paddle *paddle) {
    return (ball->pos.x == paddle->pos.x + paddle->width &&
            ball->pos.y >= paddle->pos.y &&
            ball->pos.y < paddle->pos.y + paddle->height);
}

int main() {
    Paddle left_paddle = {{1, HEIGHT / 2 - PADDLE_HEIGHT / 2}, PADDLE_HEIGHT, PADDLE_WIDTH};
    Paddle right_paddle = {{WIDTH - 2, HEIGHT / 2 - PADDLE_HEIGHT / 2}, PADDLE_HEIGHT, PADDLE_WIDTH};
    Ball ball = {{WIDTH / 2, HEIGHT / 2}, {1, 1}};

    while (true) {
        clear_screen();
        draw_paddle(&left_paddle);
        draw_paddle(&right_paddle);
        draw_ball(&ball);

        usleep(100000); // Sleep for 0.1 second
        move_ball(&ball);

        if (check_collision(&ball, &left_paddle) || check_collision(&ball, &right_paddle)) {
            ball.vel.x = -ball.vel.x;
        }
    }

    return 0;
}
