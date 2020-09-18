#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <signal.h>

#include <sys/wait.h>

#define TRUE 1
#define DEBUG 1
#define MAX_ENTRADA 1024

int main(int argc, char const *argv[]) {


  FILE *fp, *fp2;
  char *linha = NULL;
  size_t len = 0;
  char *tk[4];
  char *nome;
  int t0, dt, deadline;

  /* Abre o arquivo trace a ser lido e o a er escrito */
  fp = fopen (argv[2],"r");
  if (fp == NULL) {printf("Arquivo \"%s\" não lido Erro: %s\n",argv[2] ,strerror(errno)); exit(1);}
  if(DEBUG) printf("[DEBUG] Arquivo \"%s\" aberto!\n",argv[2]);
  fp2 = fopen(argv[3],"w");
  if (fp2 == NULL) {printf("Arquivo \"%s\" não criado Erro: %s\n",argv[3] ,strerror(errno)); exit(1);}
  if(DEBUG) printf("[DEBUG] Arquivo \"%s\" criado!\n",argv[3]);

  /* lê cada linha do arquivo de trace */
  linha = malloc(sizeof(char *));
  for (int i = 0;  i < 4;  i++) {
    tk[1] = malloc(sizeof(char *));
  }
  while (EOF != getline(&linha, &len, fp)) {
    if(DEBUG) printf("[DEBUG] Linha de comando lida com getline %s\n", linha);
    nome = malloc(sizeof(char*));
    nome = strtok(linha, " ");
    t0 = atoi(strtok(NULL, " "));
    dt = atoi(strtok(NULL, " "));
    deadline = atoi(strtok(NULL, " "));
    if(DEBUG) printf("[DEBUG] Tokens nome: %s; t0: %d; dt: %d; deadline: %d\n", nome, t0 ,dt ,deadline);
  }


  fclose (fp);
  fclose (fp2);

  if(DEBUG) printf("[DEBUG] Terminou ep1!\n");
  return 0;
}
