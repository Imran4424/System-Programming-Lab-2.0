// PLthreads.c
// Lab 2 – Part II: Sum 20 integers using two Pthreads, each summing half the array.
// Build:   gcc -Wall -Wextra -std=c11 -pthread -o PLthreads PLthreads.c
// Run:     ./PLthreads
//
// Approach: Each thread receives [from_index, to_index] via a heap-allocated struct,
// computes a local sum, returns it via pthread_exit((void*)pointer). The parent joins
// both threads, adds the two partial sums, prints the total, and frees memory.

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

typedef struct {
    int from_index;
    int to_index;
} parameters;

static const int SIZE = 20;
static int list_data[20] = {
    1,2,3,4,5,6,7,8,9,10,
    11,12,13,14,15,16,17,18,19,20
};

static void* runner(void* arg) {
    parameters* p = (parameters*)arg;
    long long* partial = malloc(sizeof(long long));
    if (!partial) { perror("malloc"); pthread_exit(NULL); }
    *partial = 0;
    for (int i = p->from_index; i <= p->to_index; ++i) {
        *partial += list_data[i];
    }
    free(p); // free the parameters passed on the heap
    pthread_exit(partial); // return pointer to partial sum
}

int main(void) {
    pthread_t t1, t2;
    pthread_attr_t attr;
    pthread_attr_init(&attr);

    // First half [0 .. SIZE/2 - 1]
    parameters* p1 = (parameters*)malloc(sizeof(parameters));
    if (!p1) { perror("malloc"); return EXIT_FAILURE; }
    p1->from_index = 0;
    p1->to_index   = (SIZE/2) - 1;

    // Second half [SIZE/2 .. SIZE - 1]
    parameters* p2 = (parameters*)malloc(sizeof(parameters));
    if (!p2) { perror("malloc"); return EXIT_FAILURE; }
    p2->from_index = (SIZE/2);
    p2->to_index   = SIZE - 1;

    if (pthread_create(&t1, &attr, runner, p1) != 0) { perror("pthread_create t1"); return EXIT_FAILURE; }
    if (pthread_create(&t2, &attr, runner, p2) != 0) { perror("pthread_create t2"); return EXIT_FAILURE; }

    void* r1; void* r2;
    if (pthread_join(t1, &r1) != 0) { perror("pthread_join t1"); return EXIT_FAILURE; }
    if (pthread_join(t2, &r2) != 0) { perror("pthread_join t2"); return EXIT_FAILURE; }

    long long total = 0;
    if (r1) { total += *(long long*)r1; free(r1); }
    if (r2) { total += *(long long*)r2; free(r2); }

    printf("Sum of numbers in the list is: %lld\n", total);
    return 0;
}
