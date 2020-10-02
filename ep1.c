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
/*Esse header e' somente usado para determinação do uso de CPU*/
#include <sched.h>
#include <stdbool.h>

#define TRUE 1
#define DEBUG 0
#define MAX_ENTRADA 1024
#define UINT_MAX	4294967295

typedef struct processo processo;
struct processo {
    char *nome;
    int t0;
    int dt;
    int deadline;
    int escalonador;
    int cpu;
    double tRestante;
    double tf;
    double tr;
    pthread_t tid;
    pthread_mutex_t *mutex;
    processo *prox;
    processo *ante;
};

/* Variáveis globais */
struct timespec startMestre;
int numMudancasContexto;
processo *cab;
int quantum;
int nCpu;
int nTerminou;
int nProcessos;
bool dParam = false;

/* Thread generalizada.*/
void *thread(void * arg);
/*Contador*/
void contador (processo *p);
/*Contador Negativo*/
void contNegativa (processo *p);
/*Multiplicador de 2*/
void multi2 (processo *p);

double elapsedTime(struct timespec a,struct timespec b);


int main(int argc, char const *argv[]) {

    FILE *fp, *fp2;
    char *linha = NULL;
    quantum = 1000;
    nTerminou = 1;
    nProcessos = 0;

    size_t len = 0;
    numMudancasContexto = 0;
    double elapsed;
    cab = malloc(sizeof(processo));
    processo *q;
    cab->prox = cab;
    cab->ante = cab;

    /*Vetor Semáforos*/
    nCpu = get_nprocs();
    int cCont = 0;
    pthread_mutex_t vMutex[nCpu];
    if(DEBUG) printf("[DEBUG] N processadores lógicos disponíveis: %d \n",nCpu);
    for (int c = 0; c < nCpu; c++) {
      pthread_mutex_init(&vMutex[c], NULL);
    }


    /* Abre o arquivo trace a ser lido e o a ser escrito */
    fp = fopen (argv[2],"r");
    if (fp == NULL) {printf("Arquivo \"%s\" não lido Erro: %s\n",argv[2] ,strerror(errno)); exit(1);}
    if(DEBUG) printf("[DEBUG] Arquivo \"%s\" aberto!\n",argv[2]);
    fp2 = fopen(argv[3],"w");
    if (fp2 == NULL) {printf("Arquivo \"%s\" não criado Erro: %s\n",argv[3] ,strerror(errno)); exit(1);}
    if(DEBUG) printf("[DEBUG] Arquivo \"%s\" criado!\n",argv[3]);
    if(DEBUG) printf("[DEBUG] Arquivo \"%s\" criado!\n",argv[3]);
    if (argc > 4) {
        dParam = true;
    }

    /* lê cada linha do arquivo de trace */
    linha = malloc(sizeof(char *));
    while (EOF != getline(&linha, &len, fp)) {
        processo *novo = malloc(sizeof(processo));
        nProcessos++;

        if(DEBUG) printf("[DEBUG] Linha de comando lida com getline %s\n", linha);
        novo->nome = malloc(sizeof(char *));
        novo->nome = strcpy(novo->nome,strtok(linha, " "));
        if(DEBUG) printf("[DEBUG] nome: %s\n",novo->nome);
        novo->t0 = atoi(strtok(NULL, " "));
        if(DEBUG) printf("[DEBUG] t0: %d\n",novo->t0);
        novo->dt = atoi(strtok(NULL, " "));
        if(DEBUG) printf("[DEBUG] dt: %d\n",novo->dt);
        novo->deadline = atoi(strtok(NULL, " "));
        if(DEBUG) printf("[DEBUG] deadline: %d\n",novo->deadline);

        novo->escalonador = atoi(argv[1]);
        if(DEBUG) printf("[DEBUG] escalonador: %d\n",novo->escalonador);
        novo->ante = cab->ante;
        cab->ante->prox = novo;
        cab->ante = novo;
        novo->prox = cab;

        /* define alternadamente um semáfaro de cpu para cada thread*/
        novo->mutex = &vMutex[cCont];
        cCont = (cCont + 1) % nCpu ;
        novo->tRestante = -1;
    }
    free(linha);
    if(DEBUG) printf("[DEBUG] Todas linhas do trace lidas!\n");


    /*Inicializa o tempo*/
    struct timespec  mid, end;
    clock_gettime(CLOCK_REALTIME, &startMestre);


    /*Inicia os processos como threads*/
    for (q = cab->prox; q != cab; q = q->prox) {

        clock_gettime(CLOCK_REALTIME, &mid);
        elapsed = elapsedTime(startMestre, mid);
        /*Espera pelo inicio da thread */
        while (elapsed < q->t0) {
            clock_gettime(CLOCK_REALTIME, &mid);
            elapsed = elapsedTime(startMestre, mid);
        }
        if (dParam)
            fprintf(stderr, "[t = %.2lf s] Chegada de um processo.\n\t\t%s %d %d %d\n\n", elapsed, q->nome, q->t0, q->dt, q->deadline);

        /* Cria a thread */
        q->tRestante = q->dt - elapsed;

        if (pthread_create(&q->tid, NULL, thread , (void*)q)) {
            printf("\n Thread \"%s\" não criada! ERRO: %s",q->nome, strerror(errno));
            exit(1);
        }
        if(DEBUG) printf("[DEBUG] Thread \"%s\" criada! no instante %f seg.\n",q->nome, elapsed);
    }

    /*Espera pelas threads finalizarem*/
    for (q = cab->prox; q != cab; q = q->prox) {
        if (pthread_join(q->tid, NULL))  {
            printf("\n Thread \"%s\" não terminou! ERRO: %s",q->nome, strerror(errno));
            exit(1);
        }
        fprintf (fp2, "%s %.2lf %.2lf\n", q->nome, q->tf, q->tr);
    }
    fprintf (fp2, "%d\n",numMudancasContexto);
    if (dParam)
        fprintf(stderr, "Total de mudanças de contexto: %d\n",numMudancasContexto);

    nTerminou = 0;

    /*fecha o arquivos*/
    fclose (fp);
    fclose (fp2);

    processo *t;
    for (q = cab->prox; q != cab; q = q->prox) {
      free(q->nome);
      t = q;
      q = q->prox;
      free(t);
    }
    free(cab);

    for (int i = 0; i < nCpu; i++) {
        pthread_mutex_destroy(&vMutex[i]);
    }

    /*Isso aqui é só para testar o time*/
    clock_gettime(CLOCK_REALTIME, &end);

    printf("[DEBUG] Terminou ep1 em: %f segundos\n",elapsedTime(startMestre,end));

    return 0;
}


/* Essa é uma função thread generalizada.                            */
/* Separa argumentos vindos junto com a chamada de criação de thread */
/* e direciona elas para as funcões incorporadas                     */
void *thread(void * arg) {

    struct timespec mid;
    processo *q;
    processo *p = (processo *) arg;
    p->cpu = sched_getcpu();
    clock_gettime(CLOCK_REALTIME, &mid);
    if(p->escalonador == 1 ) {
        pthread_mutex_lock(p->mutex);
        if (dParam)
            fprintf(stderr, "[t = %.2lf s] Processo %s começou a usar a CPU %d.\n\n",elapsedTime(startMestre, mid), p->nome, sched_getcpu());
    }
    /*inicia o contador para consumo de tempo real*/
    contador(arg);
    if(p->escalonador == 1) {
      /*Essa parte verefica se a thread que assumira em seguida utilizara imeditamente a CPU*/
      q = p->prox;
      while (q != p) {
          if ((p->mutex == q->mutex) && (q != cab) &&
              (p->tf  > q->t0)) { numMudancasContexto++; break;}
          q = q->prox;
      }
      if (dParam) {
          fprintf(stderr, "[t = %.2lf s] Processo %s liberou a CPU %d.\n\n", p->tf, p->nome, sched_getcpu());
      }
      pthread_mutex_unlock(p->mutex);
    }
}

/*Contador*/
void contador (processo *p) {

    struct timespec start, mid, midMestre, end;
    long int conta = 0;
    int n = 0;
    int uCpu = -1;
    processo * q;

    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);


    if(DEBUG) printf("[DEBUG] Thread: contador inicializada!\n");
    if((p->escalonador == 2) || (p->escalonador == 3)) { // v2
        pthread_mutex_lock(p->mutex);
        fprintf(stderr, "[t = %.2lf s] Processo %s começou a usar a CPU %d. [1ª vez]\n\n", elapsedTime(startMestre, midMestre), p->nome, sched_getcpu());
    }
    while (1) {

        /*Examina a condicao de tempo das threads*/
        if (conta % 1000000  == 0) {

            /*Verefica se a thread mudou de cpu*/ ///  @@@ Não deveria ser só pra escalonadores 2 e 3??? Está dando mudanças de contexto no escalonador 1!
            uCpu  = sched_getcpu();
            if (uCpu != p->cpu) {
              clock_gettime(CLOCK_REALTIME, &midMestre);
              fprintf(stderr, "[t = %lf s] Processo %s deixou de usar a CPU %d\n", elapsedTime(startMestre,midMestre), p->nome, p->cpu);
              fprintf(stderr, "[t = %lf s] Processo %s começou a usar a CPU %d\n", elapsedTime(startMestre,midMestre), p->nome, p->cpu);
              p->cpu = uCpu;
              numMudancasContexto++;
            }

            /*Encerra a thread em caso de fim de exec. ou deadline*/
            clock_gettime(CLOCK_THREAD_CPUTIME_ID, &mid);
            p->tRestante = p->dt - elapsedTime(start,mid);
            if (elapsedTime(start,mid) >= p->dt) {
                  p->tRestante = -1;
                  break;
            }
            if (p->escalonador == 2) {
                q = p->prox;
                while (q != p) {
                    if ((p->mutex == q->mutex) && (q != cab) &&
                        (p->tRestante > q->tRestante) && (q->tRestante != -1)) {
                        if(1) printf("[DEBUG] %s %s revezaram\n",p->nome, q->nome);
                        numMudancasContexto++;
                        if (dParam) {
                            fprintf(stderr, "[t = %.2lf s] Processo %s revezou com %s e começou a usar a CPU %d. [remover]\n", elapsedTime(startMestre, midMestre),q->nome, p->nome, sched_getcpu());
                            fprintf(stderr, "[t = %.2lf s] Processo %s liberou a CPU %d. (Revezamento)\n\n", elapsedTime(startMestre, midMestre), p->nome, sched_getcpu());
                        }
                        pthread_mutex_unlock(p->mutex);
                        sleep(1);
                        pthread_mutex_lock(p->mutex);
                        clock_gettime(CLOCK_REALTIME, &midMestre);
                        if (dParam) {
                            fprintf(stderr, "[t = %.2lf s] Processo %s começou a usar a CPU %d. [2ªvez ou mais / Revezamento] (OBS: usei q, o p é '%s')\n\n", elapsedTime(startMestre, midMestre), q->nome, sched_getcpu(), p->nome);
                        }
                        break;
                    } else {
                      q = q->prox;
                    }
                }
            }
            if (p->escalonador == 3) {
                if (dParam) {
                    fprintf(stderr, "[t = %.2lf s] Processo %s liberou a CPU %d. (Revezamento)\n\n", elapsedTime(startMestre, midMestre), p->nome, sched_getcpu());
                }
                pthread_mutex_unlock(p->mutex);
                usleep(quantum);
                pthread_mutex_lock(p->mutex);
                clock_gettime(CLOCK_REALTIME, &midMestre);
                if (dParam) {
                    fprintf(stderr, "[t = %.2lf s] Processo %s começou a usar a CPU %d. [2ªvez ou mais / Revezamento]\n\n", elapsedTime(startMestre, midMestre), p->nome, sched_getcpu());
                }
                numMudancasContexto++;
            }
        }
        conta++;
        if (conta == (UINT_MAX -1)) {
            conta = 0;
            n++;
        }

    }
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
    clock_gettime(CLOCK_REALTIME, &midMestre);
    p->tr = elapsedTime(start,end);
    p->tf = elapsedTime(startMestre,midMestre);
    p->tRestante = -1;
    if (p->escalonador == 1 && dParam)
        fprintf(stderr, "[t = %.2lf s] Processo %s finalizou sua execução.\n", p->tf, p->nome);
    if((p->escalonador == 2) || (p->escalonador == 3)) {
        if (dParam)
            fprintf(stderr, "[t = %.2lf s] Processo %s finalizou sua execução.\n[t = %.2lf s] Processo %s liberou a CPU %d. (novo)\n\t\t%s %.2f %.2f\n\n", p->tf, p->nome, p->tf, p->nome, sched_getcpu(), p->nome, p->tf, p->tr);
        pthread_mutex_unlock(p->mutex);
    }
    //printf("\n contagem contou até [%li],e UINT_MAX %d vezes\n", conta,n);
    printf("[DEBUG]  Tempo uso de CPU de %s: %f segundos\n",p->nome, p->tr);
    fprintf(stderr, "[t = %lf s] Tempo uso de CPU%d de %s: %f segundos\n",p->tf, sched_getcpu(), p->nome, p->tr);
}




double elapsedTime(struct timespec a,struct timespec b)
{
    long seconds = b.tv_sec - a.tv_sec;
    long nanoseconds = b.tv_nsec - a.tv_nsec;
    double elapsed = seconds + nanoseconds/1000000000;
    return elapsed;
}
