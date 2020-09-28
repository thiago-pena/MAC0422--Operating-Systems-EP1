#include <stdio.h>
#include <stdlib.h>

int main(int argc, char const *argv[]) {

  FILE *fp;
  int multi = 0;
  time_t t;
  srand((unsigned) time(&t));

  fp = fopen ("trace10000.txt","w");
  if (fp!=NULL)
  {
    for (int  i = 0; i < 10000; i++) {
      fprintf (fp, "p%d %d %d %d\n", i, multi, (rand()%3)*10 , i*50);
      if(i%12 == 0) multi += 10;
    }
    fclose (fp);
  }
  printf("trace10000 criado!\n");
  return 0;
}
