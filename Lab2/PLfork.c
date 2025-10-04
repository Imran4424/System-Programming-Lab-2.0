// PLfork.c
// Lab 2 – Part I: fork()/wait() process tree (9 total processes), print each PID exactly once.
// Build:   gcc -Wall -Wextra -std=c11 -o PLfork PLfork.c
// Run:     ./PLfork
//
// Tree used (1 parent + 8 descendants = 9 total):
//          P
//      /   |    \
//     A    B     C
//    / \   |    / \
//   D   E  F   G   H
//
// Each parent waits for its own children. Every process prints exactly its own PID once.

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

static pid_t fork_or_die(void) {
    pid_t p = fork();
    if (p < 0) { perror("fork"); exit(EXIT_FAILURE); }
    return p;
}

static void wait_all_children(void) {
    int status;
    while (1) {
        pid_t w = wait(&status);
        if (w == -1) break; // no more children
    }
}

int main(void) {
    // P prints its PID after creating the tree to avoid duplicates/out-of-order printing
    pid_t a,b,c;

    // Create A
    a = fork_or_die();
    if (a == 0) {
        // We're A
        pid_t d = fork_or_die();
        if (d == 0) {
            // D
            printf("%d\n", getpid());
            _exit(0);
        }
        pid_t e = fork_or_die();
        if (e == 0) {
            // E
            printf("%d\n", getpid());
            _exit(0);
        }
        wait_all_children();
        printf("%d\n", getpid()); // A
        _exit(0);
    }

    // Create B
    b = fork_or_die();
    if (b == 0) {
        // We're B
        pid_t f = fork_or_die();
        if (f == 0) {
            // F
            printf("%d\n", getpid());
            _exit(0);
        }
        wait_all_children();
        printf("%d\n", getpid()); // B
        _exit(0);
    }

    // Create C
    c = fork_or_die();
    if (c == 0) {
        // We're C
        pid_t g = fork_or_die();
        if (g == 0) {
            // G
            printf("%d\n", getpid());
            _exit(0);
        }
        pid_t h = fork_or_die();
        if (h == 0) {
            // H
            printf("%d\n", getpid());
            _exit(0);
        }
        wait_all_children();
        printf("%d\n", getpid()); // C
        _exit(0);
    }

    // Parent P waits for A, B, C (and by extension all grandchildren)
    wait_all_children();
    printf("%d\n", getpid()); // P (printed last)
    return 0;
}
