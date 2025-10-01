#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define SIZE 20

int numbers[SIZE] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20};

// Partial sums for each thread
int partial_sum1 = 0;
int partial_sum2 = 0;

// Structure to pass start and end to threads
typedef struct {
    int from_index;
    int to_index;
} parameters;

// Thread function
void *runner(void *param) {
    parameters *data = (parameters *) param;
    int local_sum = 0;

    // Calculating local sum for sublist
    for (int i = data->from_index; i <= data->to_index; i++) {
        local_sum += numbers[i];
    }

    // Storing local sum in each partial sum
    if (data->from_index == 0)
        partial_sum1 = local_sum;
    else
        partial_sum2 = local_sum;

    free(data); // Free the dynamically allocated memory
    pthread_exit(0);
}

int main() {
    pthread_t tid1, tid2;       
    pthread_attr_t attr;         

    pthread_attr_init(&attr);    // Initialize default attributes

    // Thread 1 parameters
    parameters *data1 = (parameters *) malloc(sizeof(parameters));
    data1->from_index = 0;
    data1->to_index = (SIZE / 2) - 1;

    // Thread 2 parameters
    parameters *data2 = (parameters *) malloc(sizeof(parameters));
    data2->from_index = SIZE / 2;
    data2->to_index = SIZE - 1;

    // Creating threads
    pthread_create(&tid1, &attr, runner, data1);
    pthread_create(&tid2, &attr, runner, data2);

    // Waiting for threads to finish
    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);

    // Main thread = total sum
    int sum = partial_sum1 + partial_sum2;

    printf("Sum of numbers in the list is: %d\n", sum);

    return 0;
}
