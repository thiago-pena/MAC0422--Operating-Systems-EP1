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

#define TRUE 1
#define DEBUG 1
#define MAX_ENTRADA 1024


/*Variaveis globais*/
int n = 0;
char *c;
char *p[MAX_ENTRADA]; /*Ponteiro para serie de strings*/
int status;

void type_prompt() {
    char dir[MAX_ENTRADA];
    printf("{%s@%s}", getpwuid(getuid())->pw_name, getcwd(dir, sizeof(dir)));
}

void read_command()
{
  n = 0;
  /*Le uma linha de comando*/
  char *str;
  str = malloc(sizeof(char*));
  str = readline(" ");
  if (DEBUG) printf("[DEBUG] Leu linha:%s\n",str );

  /*Gera token do comando*/
  c = malloc(sizeof(char*));
  c = strtok(str," ");
  add_history(c);
  if (DEBUG) printf("[DEBUG] Comando:%s\n",c );

  /*Gera token dos proximos parametros*/
  p[n++] = strtok(NULL," ");
  if (DEBUG) printf("[DEBUG] Parametro:%s\n", p[n - 1]);
  while (p[n -1] != NULL) {
    add_history(p[n - 1]);
    p[n] = malloc(sizeof(char*));
    p[n++] = strtok(NULL," ");
    if (DEBUG) printf("[DEBUG] Parametro:%s\n", p[n - 1]);
  }
}

int main()
{
    p[0] = NULL;
    pid_t childpid;
    while (TRUE) /* repeat forever */ {
    /* Mostra o prompt na tela */
    type_prompt();
    /*Inicializa a história*/
    using_history();
    /* Le uma linha de comando do terminal */
    read_command();

    /* MKDIR*/
    if (!strcmp(c,"mkdir")) {
        int status;
        status = mkdir(p[0], S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        if (DEBUG && !status) printf("[DEBUG] Diretório \"%s\" criado!\n", p[0]);
        if (status) printf("ERRO: Diretório não criado!\n", c);
    }
    /* Kill -9*/
    if (!strcmp(c,"kill") && !strcmp(p[0],"-9")) {
        int status;
        pid_t pidKill;
        pidKill = atoi(p[1]);
        status = kill(pidKill, SIGKILL);
        if (DEBUG && !status) printf("[DEBUG] Processo \"%d\" encerrado!\n", pidKill);
        if (status) printf("ERRO: Processo \"%d\" não encerrado!\n", pidKill);
    }
    /* ln -s*/
    if (!strcmp(c,"ln")) {
        int status;
        char* path1 = realpath(p[1], NULL);
        if (DEBUG)printf("[DEBUG] path1: %s\n", path1);
        status = symlink(path1, p[2]);
        if (DEBUG && !status) printf("[DEBUG] link do arquivo \"%s\" de nome \"%s\" criado!\n", p[1],p[2]);
        if (status) printf("ERRO: link do arquivo \"%s\" não criado!\n", p[1]);
    }

    /*DEBUG para rastreio da historia----------------------------------------*/
    if (DEBUG) {
        HISTORY_STATE *myhist = history_get_history_state ();
        HIST_ENTRY **mylist = history_list ();
        printf ("\n[DEBUG] session history:\n");
        for (int i = 0; i < myhist->length; i++) { /* output history list */
            printf ("[DEBUG]                %8s  %s\n",
            mylist[i]->line, mylist[i]->timestamp);
        }
        putchar ('\n');
    }
    /*-----------------------------------------------------------------------*/
    /*Começa o código de fork de processo*/
    //     if (fork() != 0) { /* fork off child process */
    //       /* Parent code. */
    //       waitpid(−1, &status, 0);/* wait for child to exit */
    //     } else {
    //       /* Child code. */
    //       execve(command, parameters, 0); /* execute command */
    //     }
    //   }
    }
    return 0;
 }
