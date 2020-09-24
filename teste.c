// gcc -Wall -pthread -o teste teste.c
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
    // exit
#include <unistd.h>
    // sleep
#include <limits.h>
    //para verifica INTMAX etc

#include <time.h>
    // nanosleep
    // clock();

#define NPROCESSOS 1
#define NITER 10000000000
    // 10^8

void * funcao_teste (void * a) {
    long long int count = 0;

     clock_t start_time, current_time, end_time;
     start_time = clock();
     printf("Início thread\n");

    for (long long int i = 0; i < (long int)NITER; i++) {
        // pthread_mutex_lock(&mutex2);
        if (i % 1000 == 0) {
            current_time = clock();
            if ((double)(current_time - start_time)/CLOCKS_PER_SEC >= 10) break;

        }
        count++;
        // int nanosleep(const struct timespec *req, struct timespec *rem);

        // pthread_mutex_unlock(&mutex2);
    }
    end_time = clock();
    printf("DeltaTime: %f\n", (double)(end_time - start_time)/CLOCKS_PER_SEC);
    return NULL;
}

int main() {
    // pthread_t tid[NPROCESSOS];
    pthread_t tid;



    int k = 100000000; //10^9
    for (long int i = 0; i < k; i++)
        ;

    if (pthread_create(&tid, NULL, funcao_teste, NULL)) {
        printf("\n ERROR creating thread 1");
        exit(1);
    }

    if (pthread_join(tid, NULL))  {
        printf("\n ERROR joining thread");
        exit(1);
    }

}
