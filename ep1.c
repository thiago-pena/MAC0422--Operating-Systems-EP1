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
    pthread_attr_t *attr;
    pthread_mutex_t *mutex;
    processo *prox;
    processo *ante;
    bool chegou;
    bool comecou;
    bool terminou;
};

/* Variáveis globais */
struct timespec startMestre;
int numMudancasContexto;
processo *cab;
int quantum;
int nCpu;
int nTerminou;
int nProcessos;
int nCumprimentoDeadline = 0;
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


    /*Vetor Semáforos e afinidades*/
    nCpu = get_nprocs();
    int cCont = 0;
    pthread_mutex_t vMutex[nCpu];
    pthread_attr_t vAttr[nCpu];
    cpu_set_t cpuset;
    if(DEBUG) printf("[DEBUG] N processadores lógicos disponíveis: %d \n",nCpu);
    for (int c = 0; c < nCpu; c++) {
      CPU_ZERO(&cpuset);
      CPU_SET(c, &cpuset);
      pthread_attr_init(&vAttr[c]);
      pthread_mutex_init(&vMutex[c], NULL);
      pthread_attr_setaffinity_np(&vAttr[c], sizeof(cpu_set_t), &cpuset);
    }


    /* Abre o arquivo trace a ser lido e o a ser escrito */
    fp = fopen (argv[2],"r");
    if (fp == NULL) {printf("Arquivo \"%s\" não lido Erro: %s\n",argv[2] ,strerror(errno)); exit(1);}
    if(DEBUG) printf("[DEBUG] Arquivo \"%s\" aberto!\n",argv[2]);
    fp2 = fopen(argv[3],"w");
    if (fp2 == NULL) {printf("Arquivo \"%s\" não criado Erro: %s\n",argv[3] ,strerror(errno)); exit(1);}
    if(DEBUG) printf("[DEBUG] Arquivo \"%s\" criado!\n",argv[3]);
    if(DEBUG) printf("Parâmetro 4: %s\n", argv[4]);
    if(DEBUG) printf("argc: %d\n", argc);
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
        novo->attr = &vAttr[cCont];
        cCont = (cCont + 1) % nCpu ;
        novo->tRestante = -1;

        novo->chegou = false;
        novo->comecou = false;
        novo->terminou = false;
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
        if (dParam) {
            fprintf(stderr, "[t = %.2lf s] Chegada de um processo.\n\t\t%s %d %d %d\n\n", elapsed, q->nome, q->t0, q->dt, q->deadline);
        }

        /* Cria a thread */
        // q->tRestante = q->dt - elapsed; // @@@ Incorreto!
        q->tRestante = q->dt;
        q->chegou = true;

        if (pthread_create(&q->tid, q->attr, thread , (void*)q)) {
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



    /* Free na lista duplamente ligada*/
    processo *t;
    for (q = cab->prox; q != cab;) {
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

    printf("\nNúmero de processos que cumpriram o deadline: %d\n", nCumprimentoDeadline);
    printf("Número total de processos: %d\n", nProcessos);
    printf("Número de mudanças de contexto: %d\n\n", numMudancasContexto);
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
        if (dParam) {
            clock_gettime(CLOCK_REALTIME, &mid);
            fprintf(stderr, "[t = %.2lf s] Processo %s começou a usar a CPU %d.\n\n",elapsedTime(startMestre, mid), p->nome, sched_getcpu());
        }
    }
    /*inicia o contador para consumo de tempo real*/
    contador(arg);

    /* Verifica se a thread que assumirá em seguida utilizará imeditamente a CPU */
    q = p->prox;
    while (q != p) { // saída no término de um processo
        if ((p->mutex == q->mutex) && (q != cab) &&
            (!q->terminou) && (p->tf  > q->t0)) {
                numMudancasContexto++;
                if (dParam) fprintf(stderr, "\t>>> Quantidade de mudanças de contexto: %d\t(sai %s, entra %s)\n\n", numMudancasContexto, p->nome, q->nome);
                break;
            }
        q = q->prox;
    }
    if (p->escalonador == 1) {
        if (dParam) fprintf(stderr, "[t = %.2lf s] Processo %s liberou a CPU %d.\n\n", p->tf, p->nome, sched_getcpu());
        pthread_mutex_unlock(p->mutex);
    }
    return NULL;
}

/*Contador*/
void contador (processo *p) {

    struct timespec start, mid, midMestre, end;
    long int conta = 0;
    int n = 0;
    processo * q;

    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);


    if(DEBUG) printf("[DEBUG] Thread: contador inicializada!\n");
    if((p->escalonador == 2) || (p->escalonador == 3)) { // v2
        pthread_mutex_lock(p->mutex);
        p->comecou = true;
        clock_gettime(CLOCK_REALTIME, &midMestre);
        fprintf(stderr, "[t = %.2lf s] Processo %s começou a usar a CPU %d. [1ª vez]\n\n", elapsedTime(startMestre, midMestre), p->nome, sched_getcpu());
    }
    while (1) {

        /*Examina a condicao de tempo das threads*/
        if (conta % 1000000  == 0) {


            /*Encerra a thread em caso de fim de exec. ou deadline*/
            clock_gettime(CLOCK_THREAD_CPUTIME_ID, &mid);
            p->tRestante = p->dt - elapsedTime(start,mid);
            if (elapsedTime(start,mid) >= p->dt) {
                  p->tRestante = -1; // @@@ remover
                  p->terminou = true;
                  break;
            }
            if (p->escalonador == 2) {
                q = p->prox;
                while (q != p) {
                    clock_gettime(CLOCK_REALTIME, &midMestre);
                    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &mid);
                    p->tRestante = p->dt - elapsedTime(start,mid); // Atualiza tempo restante de p para comparação
                    if ((p->mutex == q->mutex) && (q != cab) &&
                        (p->tRestante > q->tRestante) && (!q->terminou) && (q->chegou)) { // @@@ Double check se precisa de q->tRestante != -1, sendo que estou usando !q->terminou
                        if(1) printf("[DEBUG] %s %s revezaram\n",p->nome, q->nome);
                        numMudancasContexto++;
                        if (dParam) {
                            fprintf(stderr, "[t = %.2lf s] Processo %s revezou com %s. Tempos restantes: %s->%.2lf, %s->%.2lf\n", elapsedTime(startMestre, midMestre), q->nome, p->nome, p->nome, p->tRestante, q-> nome, q->tRestante);
                            fprintf(stderr, "[t = %.2lf s] Processo %s liberou a CPU %d. (Revezamento)\n\n", elapsedTime(startMestre, midMestre), p->nome, sched_getcpu());
                            if (dParam) fprintf(stderr, "\t>>> Quantidade de mudanças de contexto: %d\t(sai %s, entra %s)\n\n", numMudancasContexto, p->nome, q->nome);
                        }
                        pthread_mutex_unlock(p->mutex);
                        sleep(1);
                        pthread_mutex_lock(p->mutex);
                        clock_gettime(CLOCK_REALTIME, &midMestre);
                        if (dParam) {
                            clock_gettime(CLOCK_REALTIME, &midMestre);
                            fprintf(stderr, "[t = %.2lf s] Processo %s começou a usar a CPU %d. [Retorno]\n\n", elapsedTime(startMestre, midMestre), p->nome, sched_getcpu());
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
    p->terminou = true;
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
    clock_gettime(CLOCK_REALTIME, &midMestre);
    p->tr = elapsedTime(start,end);
    p->tf = elapsedTime(startMestre,midMestre);
    p->tRestante = -1;
    if (p->escalonador == 1 && dParam)
        fprintf(stderr, "[t = %.2lf s] Processo %s finalizou sua execução.\n\t\t%s %.2f %.2f\n\n", p->tf, p->nome, p->nome, p->tf, p->tr);
    if((p->escalonador == 2) || (p->escalonador == 3)) {
        if (dParam)
            fprintf(stderr, "[t = %.2lf s] Processo %s finalizou sua execução.\n\t\t%s %.2f %.2f\n\n[t = %.2lf s] Processo %s liberou a CPU %d. [Término]\n\n", p->tf, p->nome, p->nome, p->tf, p->tr, p->tf, p->nome, sched_getcpu());
        pthread_mutex_unlock(p->mutex);
    }
    //printf("\n contagem contou até [%li],e UINT_MAX %d vezes\n", conta,n);
    printf("[DEBUG]  Tempo uso de CPU de %s: %f segundos (tf: %.2lf, deadline: %d) Teste bool: %d\n",p->nome, p->tr, p->tf, p->deadline, (p->tf <= (double) p->deadline));
    if (p->tf <= (double) p->deadline) nCumprimentoDeadline++;
    // if (dParam) fprintf(stderr, "[t = %lf s] Tempo uso de CPU%d de %s: %f segundos\n",p->tf, sched_getcpu(), p->nome, p->tr);
}




double elapsedTime(struct timespec a,struct timespec b)
{
    long seconds = b.tv_sec - a.tv_sec;
    long nanoseconds = b.tv_nsec - a.tv_nsec;
    double elapsed = seconds + nanoseconds/1000000000;
    return elapsed;
}
