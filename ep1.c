#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <signal.h>
#include <time.h>
#include <sys/wait.h>
#include <pthread.h>
#include <math.h>
#include <sys/sysinfo.h>
#include <stdbool.h>
#include "processo.h"

#define TRUE 1
#define DEBUG 1
#define MAX_ENTRADA 1024
#define UINT_MAX	4294967295



/* Variáveis globais */
int escalonador;
time_t startMestre;
int revezou;
int numMudancasContexto = 0;
int numProcessos = 0;
int numProcessosNovos = 0;
Processo cab;
int quantum;
clock_t start_time, current_time, end_time; // pena
double elapsed_time; // pena
pthread_mutex_t mutex_chegada;
pthread_mutex_t mutex_pena;
Processo novo_candidato; // para o SRTN

/* Atualiza as variáveis globais current_time e elapsed_time*/
void updateCurrentTime();
/* Thread generalizada. */
void *thread(void * arg);
/*Contador*/
void contador (Processo p);


int main(int argc, char const *argv[]) {

    FILE *fp, *fp2;
    char *linha = NULL;
    revezou = 0;
    quantum = 1000000;

    /*Define o tipo de escalonador*/
    escalonador = atoi(argv[1]);
    if (DEBUG) printf("[DEBUG] escalonador: %d\n", escalonador);
    if (escalonador == 2) { // pena
        if (pthread_mutex_init(&mutex_chegada, NULL) == 0) printf("Mutex de chegada de novos processos inicializado com sucesso\n");
        else {
            printf("Erro na inicialização do mutex.\n");
            exit(1);
        }
    }
    pthread_mutex_init(&mutex_pena, NULL);

    size_t len = 0;
    double elapsed;
    cab = malloc(sizeof(No));
    Processo q;
    cab->prox = cab;
    cab->ante = cab;

    /*Vetor Semáforos*/
    int nCpu = get_nprocs();
    nCpu = 1; // temp pena
    int cCont = 0;
    pthread_mutex_t vMutex[nCpu];
    pthread_cond_t vCond[nCpu];
    if (DEBUG) printf("[DEBUG] N processadores lógicos disponíveis: %d \n",nCpu);
    for (int c = 0; c < nCpu; c++) {
        pthread_mutex_init(&vMutex[c], NULL);
        pthread_cond_init(&vCond[c], NULL);
    }

    /* Abre o arquivo trace a ser lido e o a ser escrito */
    fp = fopen (argv[2],"r");
    if (fp == NULL) {printf("Arquivo \"%s\" não lido Erro: %s\n",argv[2] ,strerror(errno)); exit(1);}
    if (DEBUG) printf("[DEBUG] Arquivo \"%s\" aberto!\n",argv[2]);
    fp2 = fopen(argv[3],"w");
    if (fp2 == NULL) {printf("Arquivo \"%s\" não criado Erro: %s\n",argv[3] ,strerror(errno)); exit(1);}
    if (DEBUG) printf("[DEBUG] Arquivo \"%s\" criado!\n",argv[3]);


    /* lê cada linha do arquivo de trace */
    linha = malloc(sizeof(char *));
    while (EOF != getline(&linha, &len, fp)) {
        Processo novo = malloc(sizeof(No));
        if (DEBUG) printf("[DEBUG] Linha de comando lida com getline %s\n", linha);
        novo->nome = malloc(sizeof(char *));
        novo->nome = strcpy(novo->nome,strtok(linha, " "));
        if (DEBUG) printf("[DEBUG] nome: %s\n", novo->nome);
        novo->t0 = atoi(strtok(NULL, " "));
        if (DEBUG) printf("[DEBUG] t0: %d\n", novo->t0);
        novo->dt = atoi(strtok(NULL, " "));
        if (DEBUG) printf("[DEBUG] dt: %d\n", novo->dt);
        novo->deadline = atoi(strtok(NULL, " "));
        if (DEBUG) printf("[DEBUG] deadline: %d\n", novo->deadline);

        novo->ante = cab->ante;
        cab->ante->prox = novo;
        cab->ante = novo;
        novo->prox = cab;

        /* define alternadamente um semáfaro de cpu para cada thread*/
        novo->mutex = &vMutex[cCont];
        novo->cond = &vCond[cCont];
        cCont = (cCont + 1) % nCpu;
        novo->tRestante = -1;
    }
    if (DEBUG) printf("[DEBUG] Todas linhas do trace lidas!\n");

    /*Inicializa o tempo*/
    time_t  mid, end;
    time(&startMestre);
    start_time = clock();

    /*Inicia os processos como threads*/
    for (q = cab->prox; q != cab; q = q->prox) {
        printf("----------------------------------\n");
        updateCurrentTime();

        time(&mid); // remover?
        elapsed = ((double) (mid - startMestre)); // remover?

        /*Espera pelo inicio da thread */
        while (elapsed_time <= q->t0) {
            updateCurrentTime();
        }
        q->tRestante = (double)q->dt;
        if (DEBUG) printf("\n[DEBUG] %s tem %lf seg. restantes\n\n", q->nome, q->tRestante );

        /*Cria a thread*/
        if (pthread_create(&q->tid, NULL, thread , (void*)q)) {
            printf("\n Thread \"%s\" não criada! ERRO: %s",q->nome, strerror(errno));
            exit(1);
        }
        if (DEBUG) printf("[DEBUG] Thread \"%s\" criada! no instante %f seg.\n", q->nome, elapsed_time);
        fprintf(stderr, "[t = %.2lf s] chegada de um processo:\t %s %d %d %d\n", elapsed_time, q->nome, q->t0, q->dt, q->deadline);
        if (escalonador == 2) {
            pthread_mutex_lock(&mutex_chegada);
            // if (DEBUG) printf("\n[DEBUG] mutex_chegada foi travado (chegada de processo)! numProcessosNovos: %d; numProcessos: %d\n", numProcessosNovos, numProcessos);
            if (numProcessos > 0) // Não é o primeiro processo (pena)
                numProcessosNovos++;
            numProcessos++;
            novo_candidato = q;
            // if (DEBUG) printf("\n[DEBUG] mutex_chegada foi aberto! (fim da chegada de processo) numProcessosNovos: %d, numProcessos: %d\n", numProcessosNovos, numProcessos);
            pthread_mutex_unlock(&mutex_chegada);
        }
    }

    /*Espera pelas threads finalizarem*/
    for (q = cab->prox; q != cab; q = q->prox) {
        if (pthread_join(q->tid, NULL))  {
            printf("\n Thread \"%s\" não terminou! ERRO: %s",q->nome, strerror(errno));
            exit(1);
        }
        fprintf (fp2, "%s %f %f\n",q->nome, q->tf, q->tr);
    }
    fprintf (fp2, "%d\n", revezou);


    /*fecha o arquivos*/
    fclose (fp);
    fclose (fp2);

    /*Isso aqui é só para testar o time*/
    time(&end);
    elapsed = ((double) (end - startMestre));
    if (DEBUG) printf("[DEBUG] Terminou ep1 em: %f segundos\n",elapsed);

    return 0;
}

void updateCurrentTime() {
    current_time = clock();
    elapsed_time = (double)(current_time - start_time)/CLOCKS_PER_SEC;
}

/* Essa é uma função thread generalizada.                            */
/* Separa argumentos vindos junto com a chamada de criação de thread */
/* e direciona elas para as funcões incorporadas                     */
void *thread(void * arg) {
    Processo p = (Processo ) arg;
    if (escalonador == 1 ) {
        pthread_mutex_lock(p->mutex);
        fprintf(stderr, "[t = x s] Processo %s começou a usar a CPU y(207)\n", p->nome);
    }
    static int alternador = 0;
    alternador = (alternador + 1)%3;
    alternador = 0; // temp pena
    contador(arg);
    /*
    if (alternador == 0) {
        contador(arg);
    } else if (alternador == 2) {
        contNegativa(arg);
    } else {
        multi2(arg);
    }
    */
    if (escalonador == 1) {
        fprintf(stderr, "[t = x s] Mudanças de contexto: %d (não contar o último)\n", ++numMudancasContexto);
        pthread_mutex_unlock(p->mutex);
    }

    return NULL;
}

/*Contador*/
void contador (Processo p) {
    clock_t process_start_time, process_current_time;
    double processo_dt_consumido;
    process_start_time = clock();

    time_t start, mid, end;
    long int conta = 0;
    int n = 0;
    Processo  q;

    time(&start);
    if (DEBUG) printf("[DEBUG] Thread: contador inicializada! (%s | %f)\n", p->nome, p->tRestante);
    if ((escalonador == 2) || (escalonador == 3)) {
        pthread_mutex_lock(&mutex_pena);
        fprintf(stderr, "[t = x s] Processo %s começou a usar a CPU y\n", p->nome);
    }


    while (1) {

        /*Examina a condicao de tempo das threads*/
        if (conta % 1000000  == 0) {
            /*Encerra a thread em caso de fim de exec.*/
            time(&mid);
            process_current_time = clock();
            processo_dt_consumido = (double)(process_current_time - process_start_time)/CLOCKS_PER_SEC;
            // p->tRestante = (double)(p->dt - (mid - start));
            p->tRestante = (double)(p->dt - processo_dt_consumido);
            // if ((double)(mid - start) >= p->dt) {
            if (p->tRestante < 0) {
                p->tRestante = -1;
                break;
            }
            if (escalonador == 2) {
                bool troca = false;
                pthread_mutex_lock(&mutex_chegada);
                if (numProcessosNovos > 0) { // No momento de chegada de um novo processo
                    // Problema: funciona com 1 CPU. Mas com múltiplas, precisaria checar em todas, ou então dar sort na fila
                    numProcessosNovos--;
                    troca = true;
                }
                pthread_mutex_unlock(&mutex_chegada);
                if (troca) {
                    if (p->tRestante > novo_candidato->tRestante && novo_candidato->tRestante != -1) {
                        fprintf(stderr, "\tTeste p: %s, %f\n", p->nome, p->tRestante);
                        fprintf(stderr, "\tTeste novo_candidato: %s, %f\n", novo_candidato->nome, novo_candidato->tRestante);

                        // Parar p
                        // Começar a rodar novo_candidato

                        // Remover novo_candidato da lista
                        novo_candidato->ante->prox = novo_candidato->prox;
                        novo_candidato->prox->ante = novo_candidato->ante;

                        // Inserir no antes de p
                        p->ante->prox = novo_candidato;
                        novo_candidato->ante = p->ante;
                        novo_candidato->prox = p;
                        p->ante = novo_candidato;
                        // Fim das atualizações

                        if (DEBUG) printf("[DEBUG] %s %s revezaram\n", p->nome, novo_candidato->nome);
                        fprintf(stderr, "[t = X s] O processo %s deixou de usar a CPU Y [tRestante: %f; mid-start: %f]\n", p->nome, p->tRestante, (double)(mid - start));
                        fprintf(stderr, "[t = X s] O processo %s deixou de usar a CPU Y [tRestante: %f; processo_dt_consumido: %f]\n", p->nome, p->tRestante, processo_dt_consumido);

                        // Guardar tRestante
                        process_current_time = clock(); // novo
                        processo_dt_consumido = (double)(process_current_time - process_start_time)/CLOCKS_PER_SEC; // novo
                        p->tRestante = (double)(p->dt - processo_dt_consumido);
                        revezou++;
                        // pthread_mutex_unlock(p->mutex);
                        pthread_mutex_unlock(&mutex_pena);
                        // nanosleep((const struct timespec[]){{0, 5000000000L}}, NULL); // 5*10^9 ns = 1s
                        sleep(1);
                        // pthread_yield();
                        // pthread_mutex_lock(p->mutex);
                        pthread_mutex_lock(&mutex_pena);
                        // Restaurar tRestante
                        fprintf(stderr, "[t = X s] O processo %s voltou a usar a CPU Y [tRestante: %f; mid-start: %f]\n", p->nome, p->tRestante, (double)(mid - start));
                        fprintf(stderr, "[t = X s] O processo %s voltou a usar a CPU Y [tRestante: %f; processo_dt_consumido: %f]\n", p->nome, p->tRestante, processo_dt_consumido);
                    }
                }
            }
            else if (escalonador == 3) {
              pthread_mutex_unlock(p->mutex);
              fprintf(stderr, "[t = X s] O processo %s deixou de usar a CPU Y\n", p->nome);
              usleep(quantum);
              pthread_mutex_lock(p->mutex);
            }
        }
        conta++;
        if (conta == (UINT_MAX -1)) {
            conta = 0;
            n++;
        }

    }
    if ((escalonador == 2) || (escalonador == 3)) pthread_mutex_unlock(&mutex_pena);
    time(&end);
    p->tr = (double)(end - start);
    p->tf = (double)(end - startMestre);
    p->tRestante = -1;
    printf("\n contagem contou até [%li],e UINT_MAX %d vezes\n", conta, n);
    if (DEBUG) printf("[DEBUG]  Tempo uso de CPU de %s: %f segundos\n\n", p->nome, p->tr);
    process_current_time = clock();
    processo_dt_consumido = (double)(process_current_time - process_start_time)/CLOCKS_PER_SEC;
    if (DEBUG) printf("[DEBUG]  Tempo uso de CPU de %s: %f segundos(b)\n\n", p->nome, processo_dt_consumido);
    fprintf(stderr, "[t = X s] Fim da execução do processo %s (falta linha) | %f\n", p->nome, processo_dt_consumido);
}
