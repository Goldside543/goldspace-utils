// SPDX-License-Identifier: GPL-2.0-only
/*
 * gcalc/gcalc.c
 * 
 * Simple calculator for Goldspace.
 *
 * Copyright (C) 2024 Goldside543
 *
 */

#include <stdio.h>

void calculate(double a, double b, char op) {
    switch (op) {
        case '+':
            printf("Result: %.2f\n", a + b);
            break;
        case '-':
            printf("Result: %.2f\n", a - b);
            break;
        case '*':
            printf("Result: %.2f\n", a * b);
            break;
        case '/':
            if (b != 0)
                printf("Result: %.2f\n", a / b);
            else
                printf("Error: Division by zero\n");
            break;
        default:
            printf("Error: Unsupported operator\n");
    }
}

int main() {
    double a, b;
    char op;

    printf("Enter calculation (e.g., 3.5 + 4.2): ");
    if (scanf("%lf %c %lf", &a, &op, &b) == 3) {
        calculate(a, b, op);
    } else {
        printf("Invalid input\n");
    }

    return 0;
}
