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


#define TRUE 1
#define DEBUG 1
#define MAX_ENTRADA 1024
#define UINT_MAX	4294967295

typedef struct processo processo;
struct processo {
    char *nome;
    int t0;
    int dt;
    int deadline;
    int escalonador;
    double tRestante;
    double tf;
    double tr;
    pthread_t tid;
    pthread_mutex_t *mutex;
    pthread_cond_t *cond;
    processo *prox;
    processo *ante;
};

/*Cariáveis globais*/
time_t startMestre;
int revezou;
processo *cab;
int quantum;

/* Thread generalizada.*/
void *thread(void * arg);
/*Contador*/
void contador (processo *p);
/*Contador Negativo*/
void contNegativa (processo *p);
/*Multiplicador de 2*/
void multi2 (processo *p);



int main(int argc, char const *argv[]) {

    FILE *fp, *fp2;
    char *linha = NULL;
    revezou = 0;
    quantum = 1000;

    size_t len = 0;
    double elapsed;
    cab= malloc(sizeof(processo));
    processo *q;
    cab->prox = cab;
    cab->ante = cab;

    /*Vetor Semáforos*/
    int nCpu = get_nprocs();
    int cCont = 0;
    pthread_mutex_t vMutex[nCpu];
    pthread_cond_t vCond[nCpu];
    if(DEBUG) printf("[DEBUG] N processadores lógicos disponíveis: %d \n",nCpu);
    for (int c = 0; c < nCpu; c++) {
      pthread_mutex_init(&vMutex[c], NULL);
      pthread_cond_init(&vCond[c], NULL);
    }

    /* Abre o arquivo trace a ser lido e o a ser escrito */
    fp = fopen (argv[2],"r");
    if (fp == NULL) {printf("Arquivo \"%s\" não lido Erro: %s\n",argv[2] ,strerror(errno)); exit(1);}
    if(DEBUG) printf("[DEBUG] Arquivo \"%s\" aberto!\n",argv[2]);
    fp2 = fopen(argv[3],"w");
    if (fp2 == NULL) {printf("Arquivo \"%s\" não criado Erro: %s\n",argv[3] ,strerror(errno)); exit(1);}
    if(DEBUG) printf("[DEBUG] Arquivo \"%s\" criado!\n",argv[3]);


    /* lê cada linha do arquivo de trace */
    linha = malloc(sizeof(char *));
    while (EOF != getline(&linha, &len, fp)) {
        processo *novo = malloc(sizeof(processo));
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

        /*Define o tipo de escalonador*/
        if (!strcmp(argv[1],"1")) {
          novo->escalonador = 1;
        } else if (!strcmp(argv[1],"2")) {
          novo->escalonador = 2;
        } else {
          novo->escalonador = 3;
        }
        if(DEBUG) printf("[DEBUG] escalonador: %d\n",novo->escalonador);
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
    if(DEBUG) printf("[DEBUG] Todas linhas do trace lidas!\n");

    /*Inicializa o tempo*/
    time_t  mid, end;
    time(&startMestre);


    /*Inicia os processos como threads*/
    for (q = cab->prox; q != cab; q = q->prox) {

        time(&mid);
        elapsed = ((double) (mid - startMestre));
        /*Espera pelo inicio da thread */
        while (elapsed <= q->t0) {
            time(&mid);
            elapsed = ((double) (mid - startMestre)) ;
        }
        q->tRestante = q->deadline - q->t0;
        if(DEBUG) printf("\n[DEBUG] %s tem %lf seg. restantes\n\n",q->nome,q->tRestante );

        /*Cria a thread*/
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
        fprintf (fp2, "%s %f %f\n",q->nome, q->tf, q->tr);
    }
    fprintf (fp2, "%d\n",revezou);


    /*fecha o arquivos*/
    fclose (fp);
    fclose (fp2);

    /*Isso aqui é só para testar o time*/
    time(&end);
    elapsed = ((double) (end - startMestre));
    if(DEBUG) printf("[DEBUG] Terminou ep1 em: %f segundos\n",elapsed);

    return 0;
}


/* Essa é uma função thread generalizada.                            */
/* Separa argumentos vindos junto com a chamada de criação de thread */
/* e direciona elas para as funcões incorporadas                     */
void *thread(void * arg) {
    processo *p = (processo *) arg;
    if(p->escalonador == 1 ) pthread_mutex_lock(p->mutex);
    static int alternador = 0;
    alternador = (alternador + 1)%3;
    if (alternador == 0) {
        contador(arg);
    } else if (alternador == 2) {
        contNegativa(arg);
    } else {
        multi2(arg);
    }
    if(p->escalonador == 1) pthread_mutex_unlock(p->mutex);
}

/*Contador*/
void contador (processo *p) {

    time_t start, mid, end;
    long int conta = 0;
    int n = 0;
    processo * q;

    time(&start);
    if(DEBUG) printf("[DEBUG] Thread: contador inicializada!\n");
    if((p->escalonador == 2) || (p->escalonador == 3)) pthread_mutex_lock(p->mutex);
    while (1) {
        /*Examina a condicao de tempo das threads*/
        if (conta % 1000000  == 0) {
            /*Encerra a thread em caso de fim de exec. ou deadline*/
            time(&mid);
            p->tRestante = p->deadline - (mid - startMestre);
            if ((double)(mid - start) >= p->dt ||
                (double)(mid - startMestre) >= p->deadline) {
                  p->tRestante = -1;
                  break;
            }
            if (p->escalonador == 2) {
                q = p->prox;
                while (q != p) {
                    if ((p->mutex == q->mutex) && (q != cab) &&
                        (p->tRestante > q->tRestante) && (q->tRestante != -1)) {
                        if(DEBUG) printf("[DEBUG] %s %s revezaram\n",p->nome, q->nome);
                        revezou++;
                        pthread_mutex_unlock(p->mutex);
                        sleep(1);
                        pthread_mutex_lock(p->mutex);
                        break;
                    } else {
                      q = q->prox;
                    }
                }
            }
            if (p->escalonador == 3) {
              pthread_mutex_unlock(p->mutex);
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
    if((p->escalonador == 2) || (p->escalonador == 3)) pthread_mutex_unlock(p->mutex);
    time(&end);
    p->tr = (double)(end - start);
    p->tf = (double)(end - startMestre);
    p->tRestante = -1;
    printf("\n contagem contou até [%li],e UINT_MAX %d vezes\n", conta,n);
    if(DEBUG)printf("[DEBUG]  Tempo uso de CPU de %s: %f segundos\n\n",p->nome, p->tr);
}

/*Contador Negativo*/
void contNegativa (processo *p) {

    long int conta = 0;
    int n = 0;
    time_t start,mid, end;
    processo * q;

    time(&start);
    if(DEBUG) printf("[DEBUG] Thread: contNegativa inicializada!\n");
    if((p->escalonador == 2) || (p->escalonador == 3)) pthread_mutex_lock(p->mutex);
    while (1) {
        if (conta % 1000000 == 0) {
            /*Encerra a thread em caso de fim de exec. ou deadline*/
            time(&mid);
            p->tRestante = p->deadline - (mid - startMestre);
            if ((double)(mid - start) >= p->dt ||
                (double)(mid - startMestre) >= p->deadline) {
                  p->tRestante = -1;
                  break;
            }
            if (p->escalonador == 2) {
                q = p->prox;
                while (q != p) {
                    if ((p->mutex == q->mutex) && (q != cab) &&
                        (p->tRestante > q->tRestante) && (q->tRestante != -1)) {
                        if(DEBUG) printf("[DEBUG] %s %s revezaram\n",p->nome, q->nome);
                        revezou++;
                        pthread_mutex_unlock(p->mutex);
                        sleep(1);
                        pthread_mutex_lock(p->mutex);
                        break;
                    } else {
                      q = q->prox;
                    }
                }

            }
            if (p->escalonador == 3) {
              pthread_mutex_unlock(p->mutex);
              usleep(quantum);
              pthread_mutex_lock(p->mutex);
            }
        }
        conta--;
        if (conta == -(UINT_MAX + 1)) {
          conta = 0;
          n++;
        }
    }
    if((p->escalonador == 2) || (p->escalonador == 3)) pthread_mutex_unlock(p->mutex);
    time(&end);
    p->tr = (double)(end - start);
    p->tf = (double)(end - startMestre);
    p->tRestante = -1;
    printf("\n contNegativa contou até [%li],e -UINT_MAX %d vezes\n", conta,n);
    if(DEBUG)printf("[DEBUG]  Tempo uso de CPU de %s: %f segundos\n\n",p->nome,p->tr);
}

/*multiplica por 2*/
void multi2 (processo *p) {

    long int conta = 1;
    long int n = -1;
    time_t start,mid, end;
    processo * q;

    time(&start);
    if(DEBUG) printf("[DEBUG] Thread: contNegativa inicializada!\n");
    if((p->escalonador == 2) || (p->escalonador == 3)) pthread_mutex_lock(p->mutex);
    while (1) {
        if (n % 1000000  == 0) {
            /*Encerra a thread em caso de fim de exec. ou deadline*/
            time(&mid);
            p->tRestante = p->deadline - (mid - startMestre);
            if ((double)(mid - start) >= p->dt ||
                (double)(mid - startMestre) >= p->deadline) {
                  p->tRestante = -1;
                  break;
            }
            if (p->escalonador == 2) {
                q = p->prox;
                while (q != p) {
                    if ((p->mutex == q->mutex) && (q != cab) &&
                        (p->tRestante > q->tRestante) && (q->tRestante != -1)) {
                        if(DEBUG) printf("[DEBUG] %s %s revezaram\n",p->nome, q->nome);
                        revezou++;
                        pthread_mutex_unlock(p->mutex);
                        sleep(1);
                        pthread_mutex_lock(p->mutex);
                        break;
                    } else {
                      q = q->prox;
                    }
                }
            }
            if (p->escalonador == 3) {
              pthread_mutex_unlock(p->mutex);
              usleep(quantum);
              pthread_mutex_lock(p->mutex);
            }
        }
        conta *= 2;
        n++;
        if ((n % 31) == 0) conta = 1;
    }
    if((p->escalonador == 2) || (p->escalonador == 3)) pthread_mutex_unlock(p->mutex);
    time(&end);
    p->tr = (double)(end - start);
    p->tf = (double)(end - startMestre);
    p->tRestante = -1;
    printf("\n multi2 multiplicou 2  %li vezes\n",n);
    if(DEBUG)printf("[DEBUG]  Tempo uso de CPU de %s: %f segundos\n\n",p->nome, p->tr);
}
