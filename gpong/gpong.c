// SPDX-License-Identifier: GPL-2.0-only
/*
 * gpong/gpong.c
 * 
 * Pong, ported to Goldspace.
 *
 * Copyright (C) 2024 Goldside543
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>

#define WIDTH 80
#define HEIGHT 24
#define PADDLE_HEIGHT 5
#define BALL_CHAR 'O'
#define PADDLE_CHAR '|'
#define EMPTY_CHAR ' '
#define WINNING_SCORE 10
#define CENTER_LINE_CHAR '|'

// Structures for Ball, Paddle, and Game Mode
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

typedef enum {
    SINGLE_PLAYER,
    MULTIPLAYER
} GameMode;

// Function to clear the screen
void clear_screen() {
    printf("\033[H\033[J");
}

// Function to draw the game frame
void draw_frame(Ball *ball, Paddle *left_paddle, Paddle *right_paddle, int left_score, int right_score) {
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

    // Draw center line
    for (int y = 0; y < HEIGHT; y++) {
        screen[WIDTH / 2][y] = CENTER_LINE_CHAR;
    }

    // Draw scoreboard
    clear_screen(); // Clear the previous frame
    printf("Scoreboard: Player 1: %d  |  Player 2: %d\n", left_score, right_score);
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            printf("%c", screen[x][y]);
        }
        printf("\n");
    }
}

// Function to draw the title screen
void draw_title_screen(GameMode *mode) {
    clear_screen();
    printf("************************************************\n");
    printf("*                                              *\n");
    printf("*                       GPONG                  *\n");
    printf("*                                              *\n");
    printf("*        1. Single Player                      *\n");
    printf("*        2. Multiplayer                        *\n");
    printf("*                                              *\n");
    printf("*        Single Player Controls:               *\n");
    printf("*        Player 1: Move Paddle Up: 'w'         *\n");
    printf("*                   Move Paddle Down: 's'      *\n");
    printf("*                                              *\n");
    printf("*        Multiplayer Controls:                 *\n");
    printf("*        Player 1: Move Paddle Up: 'w'         *\n");
    printf("*                   Move Paddle Down: 's'      *\n");
    printf("*        Player 2: Move Paddle Up: 'i'         *\n");
    printf("*                   Move Paddle Down: 'k'      *\n");
    printf("*                                              *\n");
    printf("*        Press 1 for Single Player             *\n");
    printf("*        Press 2 for Multiplayer               *\n");
    printf("************************************************\n");

    // Wait for user input to select the game mode
    char c;
    while (1) {
        c = getchar();
        if (c == '1') {
            *mode = SINGLE_PLAYER;
            break;
        } else if (c == '2') {
            *mode = MULTIPLAYER;
            break;
        }
    }
}

// Function to check if a key is pressed
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

// Function to update the game state
void update_game(Ball *ball, Paddle *left_paddle, Paddle *right_paddle, GameMode mode, int *left_score, int *right_score) {
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
    if (ball->pos.x <= 0) {
        (*right_score)++;
        if (*right_score >= WINNING_SCORE) {
            clear_screen();
            printf("Player 2 Wins!\n");
            exit(0);
        }
        ball->pos.x = WIDTH / 2;
        ball->pos.y = HEIGHT / 2;
        ball->dir.x = (rand() % 2) ? 1 : -1;
        ball->dir.y = (rand() % 2) ? 1 : -1;
    } else if (ball->pos.x >= WIDTH - 1) {
        (*left_score)++;
        if (*left_score >= WINNING_SCORE) {
            clear_screen();
            printf("Player 1 Wins!\n");
            exit(0);
        }
        ball->pos.x = WIDTH / 2;
        ball->pos.y = HEIGHT / 2;
        ball->dir.x = (rand() % 2) ? 1 : -1;
        ball->dir.y = (rand() % 2) ? 1 : -1;
    }

    // Move paddles
    if (kbhit()) {
        char c = getchar();
        if (mode == MULTIPLAYER) {
            // Player controls for both paddles in Multiplayer mode
            if (c == 'w' && left_paddle->pos.y > 0) {
                left_paddle->pos.y--;
            } else if (c == 's' && left_paddle->pos.y < HEIGHT - left_paddle->height) {
                left_paddle->pos.y++;
            } else if (c == 'i' && right_paddle->pos.y > 0) {
                right_paddle->pos.y--;
            } else if (c == 'k' && right_paddle->pos.y < HEIGHT - right_paddle->height) {
                right_paddle->pos.y++;
            }
        } else if (mode == SINGLE_PLAYER) {
            // Player controls for left paddle in Single Player mode
            if (c == 'w' && left_paddle->pos.y > 0) {
                left_paddle->pos.y--;
            } else if (c == 's' && left_paddle->pos.y < HEIGHT - left_paddle->height) {
                left_paddle->pos.y++;
            }
        }
    }

    // AI for right paddle in Single Player mode
    if (mode == SINGLE_PLAYER) {
        int ai_speed = 1;

        // Slow down AI if its score is high and the player's score is low
        if (*right_score > 5 && *left_score < 5) {
            ai_speed = 1; // Slow down AI
        } else {
            ai_speed = 2; // Normal AI speed
        }

        if (ball->pos.y < right_paddle->pos.y) {
            right_paddle->pos.y -= ai_speed;
        } else if (ball->pos.y > right_paddle->pos.y + right_paddle->height - 1) {
            right_paddle->pos.y += ai_speed;
        }

        // Ensure AI paddle stays within the bounds
        if (right_paddle->pos.y < 0) {
            right_paddle->pos.y = 0;
        } else if (right_paddle->pos.y > HEIGHT - right_paddle->height) {
            right_paddle->pos.y = HEIGHT - right_paddle->height;
        }
    }
}

int main() {
    Ball ball = {{WIDTH / 2, HEIGHT / 2}, {1, 1}};
    Paddle left_paddle = {{1, HEIGHT / 2 - PADDLE_HEIGHT / 2}, PADDLE_HEIGHT};
    Paddle right_paddle = {{WIDTH - 2, HEIGHT / 2 - PADDLE_HEIGHT / 2}, PADDLE_HEIGHT};
    GameMode mode;
    int left_score = 0;
    int right_score = 0;

    draw_title_screen(&mode);

    while (1) {
        update_game(&ball, &left_paddle, &right_paddle, mode, &left_score, &right_score);
        draw_frame(&ball, &left_paddle, &right_paddle, left_score, right_score);
        usleep(50000); // Sleep for 50 ms
    }

    return 0;
}
