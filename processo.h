#ifndef PROCESSO_H
#define PROCESSO_H
#include <pthread.h>

typedef struct no {
    char *nome;
    int t0;
    int dt;
    int deadline;
    double tRestante;
    double tf;
    double tr;
    pthread_t tid;
    pthread_mutex_t *mutex;
    pthread_cond_t *cond;
    struct no *prox;
    struct no *ante;
} No;
typedef No *Processo;

#endif
