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

#define TRUE 1
#define DEBUG 1
#define MAX_ENTRADA 1024

typedef struct processo processo;
struct processo {
    char *nome;
    int t0;
    int dt;
    int deadline;
    pthread_t tid;
    processo *prox;
};


/* Define tipo de função como argumento*/
typedef void (*functiontype)();

/* Thread generalizada.*/
void *thread(void * arg);
/*Contador*/
void contador ();
/*Contador Negativo*/
void contNegativa ();


int main(int argc, char const *argv[]) {

  FILE *fp, *fp2;
  char *linha = NULL;
  size_t len = 0;
  int nLinhas = 0;
  double elapsed;
  processo *cab = malloc(sizeof(processo));
  processo *q;
  cab->prox = NULL;

  /* Abre o arquivo trace a ser lido e o a ser escrito */
  fp = fopen (argv[2],"r");
  if (fp == NULL) {printf("Arquivo \"%s\" não lido Erro: %s\n",argv[2] ,strerror(errno)); exit(1);}
  if(DEBUG) printf("[DEBUG] Arquivo \"%s\" aberto!\n",argv[2]);
  fp2 = fopen(argv[3],"w");
  if (fp2 == NULL) {printf("Arquivo \"%s\" não criado Erro: %s\n",argv[3] ,strerror(errno)); exit(1);}
  if(DEBUG) printf("[DEBUG] Arquivo \"%s\" criado!\n",argv[3]);

  /*Aqui um código lento e burro para contar o número de linhas. Seria bom que esse número estivesse no trace.txt*/
  char c;
  for (c = getc(fp); c != EOF; c = getc(fp))
        if (c == '\n')
            nLinhas = nLinhas + 1;
  fclose (fp);
  fp = fopen (argv[2],"r");
  if (fp == NULL) {printf("Arquivo \"%s\" não lido Erro: %s\n",argv[2] ,strerror(errno)); exit(1);}
  if(DEBUG) printf("[DEBUG] Número de linhas do trace: %d\n", nLinhas);

  /*Inicializa o tempo*/
  clock_t start, end;
  start = clock();

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
    novo->prox = cab->prox;
    cab->prox = novo;
  }
  if(DEBUG) printf("[DEBUG] Todas linhas do trace lidas!\n");

  /*Inicia os processos como threads*/
  for (q = cab->prox; q != NULL; q = q->prox) {
    if (pthread_create(&q->tid, NULL, thread , q->nome)) {
        printf("\n Thread \"%s\" não criada! ERRO: %s",q->nome, strerror(errno));
        exit(1);
    }
  }
  /*Espera pelas threads finalizarem*/
  for (q = cab->prox; q != NULL; q = q->prox) {
    if (pthread_join(q->tid, NULL))  {
        printf("\n Thread \"%s\" não terminou! ERRO: %s",q->nome, strerror(errno));
        exit(1);
    }
  }

  /*fecha o arquivos*/
  fclose (fp);
  fclose (fp2);

  /*Isso aqui é só para testar o time*/
  end = clock();
  elapsed = ((double) (end - start)) / CLOCKS_PER_SEC;
  if(DEBUG) printf("[DEBUG] Terminou ep1 em: %f segundos\n",elapsed);

  return 0;
}


/* Essa é uma função thread generalizada.                           */
/* Separa argumentos vindos junto com a chamada de criação de thread*/
/* e direciona elas para as funcões incorporadas                    */
void *thread(void * arg) {
  if(DEBUG) printf("[DEBUG] thread() iniciou com argumento '%s'\n", arg);
  functiontype func;
  if (!strcmp(arg, "contador")) {
    func = &contador;
    func();
  } else if (!strcmp(arg, "contNegativa")){
    func = &contNegativa;
    func();
  } else {
    printf("ERRO: Processo não definido\n");
  }
}

/*Contador*/
void contador () {
    if(DEBUG) printf("[DEBUG] Thread: contador inicializada!\n");
    long int conta = 0;

    for (long int i=0; i<1000; i++) {
        if(DEBUG )printf("[info] contador: Estou em %ld\n", conta );
        conta++;
    }

    printf("\n contagem terminou em  [%li]\n", conta);

}

/*Contador Negativo*/
void contNegativa () {
    if(DEBUG) printf("[DEBUG] Thread: contNegativa inicializada!\n");
    long int conta = 0;

    for (long int i=0; i< 1000; i++) {
        if(DEBUG)printf("[info] contNegativo: Estou em %ld\n", conta );
        conta--;
    }

    printf("\n contNegativa terminou em  [%li]\n", conta);

}
