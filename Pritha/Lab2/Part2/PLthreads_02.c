#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define SIZE 20

int numbers[SIZE];          
int partial_sum1 = 0;       
int partial_sum2 = 0;       

typedef struct {
    int from_index;
    int to_index;
} parameters;


void *runner(void *param) {
    parameters *data = (parameters *) param;
    int local_sum = 0;

    
    for (int i = data->from_index; i <= data->to_index; i++) {
        local_sum += numbers[i];
    }

    
    if (data->from_index == 0)
        partial_sum1 = local_sum;
    else
        partial_sum2 = local_sum;

    free(data);            
    pthread_exit(0);
}

int main(int argc, char *argv[]) {
    if (argc != SIZE + 1) {
        printf("Please provide %d numbers as input.\n", SIZE);
        return 1;
    }

    
    for (int i = 0; i < SIZE; i++) {
        numbers[i] = atoi(argv[i + 1]);
    }

    pthread_t tid1, tid2;
    pthread_attr_t attr;

    pthread_attr_init(&attr);    

    
    parameters *data1 = (parameters *) malloc(sizeof(parameters));
    data1->from_index = 0;
    data1->to_index = (SIZE / 2) - 1;

    
    parameters *data2 = (parameters *) malloc(sizeof(parameters));
    data2->from_index = SIZE / 2;
    data2->to_index = SIZE - 1;

    
    pthread_create(&tid1, &attr, runner, data1);
    pthread_create(&tid2, &attr, runner, data2);

    
    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);

    
    int sum = partial_sum1 + partial_sum2;
    printf("Sum of numbers in the list is: %d\n", sum);

    return 0;
}
