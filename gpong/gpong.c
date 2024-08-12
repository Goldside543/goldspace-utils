// SPDX-License-Identifier: GPL-2.0-only
/*
 * gpong/gpong.c
 * 
 * Pong, ported to Goldspace.
 *
 * Copyright (C) Goldside543
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>

#define WIDTH 40
#define HEIGHT 20
#define PADDLE_HEIGHT 4
#define BALL_CHAR 'O'
#define PADDLE_CHAR '|'
#define EMPTY_CHAR ' '

typedef struct {
    int x, y;
} Position;

typedef struct {
    Position pos;
    Position dir;
} Ball;

typedef struct {
    Position pos;
    int height;
} Paddle;

void clear_screen() {
    printf("\033[H\033[J");
}

void draw_frame(Ball *ball, Paddle *left_paddle, Paddle *right_paddle) {
    char screen[WIDTH][HEIGHT];
    // Initialize screen with empty characters
    for (int x = 0; x < WIDTH; x++) {
        for (int y = 0; y < HEIGHT; y++) {
            screen[x][y] = EMPTY_CHAR;
        }
    }

    // Draw paddles
    for (int i = 0; i < left_paddle->height; i++) {
        if (left_paddle->pos.y + i < HEIGHT) {
            screen[left_paddle->pos.x][left_paddle->pos.y + i] = PADDLE_CHAR;
        }
    }

    for (int i = 0; i < right_paddle->height; i++) {
        if (right_paddle->pos.y + i < HEIGHT) {
            screen[right_paddle->pos.x][right_paddle->pos.y + i] = PADDLE_CHAR;
        }
    }

    // Draw ball
    if (ball->pos.y < HEIGHT && ball->pos.x < WIDTH) {
        screen[ball->pos.x][ball->pos.y] = BALL_CHAR;
    }

    // Print screen
    clear_screen(); // Clear the previous frame
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            printf("%c", screen[x][y]);
        }
        printf("\n");
    }
}

void draw_title_screen() {
    clear_screen();
    printf("************************************************\n");
    printf("*                                              *\n");
    printf("*                     GPONG                    *\n");
    printf("*                                              *\n");
    printf("*         Use 'W' and 'S' to move the left     *\n");
    printf("*        paddle. Use 'I' and 'K' to move the   *\n");
    printf("*         right paddle. Press ENTER to start.  *\n");
    printf("*                                              *\n");
    printf("************************************************\n");

    // Wait for user input to start the game
    while (getchar() != '\n') {
        // Wait for ENTER key
    }
}

int kbhit() {
    struct termios oldt, newt;
    int ch;
    int oldf;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);
    if(ch != EOF) {
        ungetc(ch, stdin);
        return 1;
    }
    return 0;
}

void update_game(Ball *ball, Paddle *left_paddle, Paddle *right_paddle) {
    ball->pos.x += ball->dir.x;
    ball->pos.y += ball->dir.y;

    // Ball collision with top and bottom
    if (ball->pos.y <= 0 || ball->pos.y >= HEIGHT - 1) {
        ball->dir.y = -ball->dir.y;
    }

    // Ball collision with paddles
    if (ball->pos.x == left_paddle->pos.x + 1 &&
        ball->pos.y >= left_paddle->pos.y &&
        ball->pos.y < left_paddle->pos.y + left_paddle->height) {
        ball->dir.x = -ball->dir.x;
    }

    if (ball->pos.x == right_paddle->pos.x - 1 &&
        ball->pos.y >= right_paddle->pos.y &&
        ball->pos.y < right_paddle->pos.y + right_paddle->height) {
        ball->dir.x = -ball->dir.x;
    }

    // Ball out of bounds
    if (ball->pos.x <= 0 || ball->pos.x >= WIDTH - 1) {
        ball->pos.x = WIDTH / 2;
        ball->pos.y = HEIGHT / 2;
        ball->dir.x = (rand() % 2) ? 1 : -1;
        ball->dir.y = (rand() % 2) ? 1 : -1;
    }

    // Move paddles
    if (kbhit()) {
        char c = getchar();
        if (c == 'w' && left_paddle->pos.y > 0) {
            left_paddle->pos.y--;
        } else if (c == 's' && left_paddle->pos.y < HEIGHT - left_paddle->height) {
            left_paddle->pos.y++;
        } else if (c == 'i' && right_paddle->pos.y > 0) {
            right_paddle->pos.y--;
        } else if (c == 'k' && right_paddle->pos.y < HEIGHT - right_paddle->height) {
            right_paddle->pos.y++;
        }
    }
}

int main() {
    Ball ball = {{WIDTH / 2, HEIGHT / 2}, {1, 1}};
    Paddle left_paddle = {{2, HEIGHT / 2 - PADDLE_HEIGHT / 2}, PADDLE_HEIGHT};
    Paddle right_paddle = {{WIDTH - 3, HEIGHT / 2 - PADDLE_HEIGHT / 2}, PADDLE_HEIGHT};

    // Show the title screen
    draw_title_screen();

    // Main game loop
    while (1) {
        update_game(&ball, &left_paddle, &right_paddle);
        draw_frame(&ball, &left_paddle, &right_paddle);
        usleep(100000); // Sleep for 100 ms (adjust as needed)
    }

    return 0;
}
