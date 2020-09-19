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

  /* lê cada linha do arquivo de trace */
  pthread_t tid[nLinhas];
  char *nome[nLinhas];
  int t0[nLinhas], dt[nLinhas], deadline[nLinhas];
  int cont = nLinhas;
  linha = malloc(sizeof(char *));
  for (int i = 0; i < nLinhas; i++) {
    nome[i] = malloc(sizeof(char*));
  }

  /*Inicializa o tempo*/
  clock_t start, end;
  start = clock();

  while (EOF != getline(&linha, &len, fp)) {
    if(DEBUG) printf("[DEBUG] Linha de comando lida com getline %s\n", linha);
    nome[cont -1] = strcpy(nome[cont -1],strtok(linha, " "));
    if(DEBUG) printf("[DEBUG] nome[%d]: %s\n",(cont -1),nome[cont -1]);
    t0[cont -1] = atoi(strtok(NULL, " "));
    if(DEBUG) printf("[DEBUG] t0[%d]: %d\n",(cont -1),t0[cont -1]);
    dt[cont -1] = atoi(strtok(NULL, " "));
    if(DEBUG) printf("[DEBUG] dt[%d]: %d\n",(cont -1),dt[cont -1]);
    deadline[cont -1] = atoi(strtok(NULL, " "));
    if(DEBUG) printf("[DEBUG] deadline[%d]: %d\n",(cont -1),deadline[cont -1]);
    cont--;
  }
  if(DEBUG) printf("[DEBUG] Todas linhas do trace lidas!\n");

  /*Inicia os processos como threads*/
  for (int  j = 0; j < nLinhas; j++) {
    if (pthread_create(&tid[j], NULL, thread , nome[j])) {
        printf("\n Thread \"%s\" não criada! ERRO: %s",nome[j], strerror(errno));
        exit(1);
    }
  }
  /*Espera pelas threads finalizarem*/
  for (int  j = 0; j < nLinhas; j++) {
    if (pthread_join(tid[j], NULL))  {
        printf("\n Thread \"%s\" não terminou! ERRO: %s",nome[j], strerror(errno));
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
