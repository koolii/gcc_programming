#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int
main(int argc, char *argv[])
{
  int i;

  if (argc < 2) {
    fprintf(stderr, "%s: no arguments\n", argv[0]);
    exit(1);
  }

  for (i = 0; i < argc; i++) {
    // 0のときに正常終了
    if (unlink(argv[i]) < 0) {
      perror(argv[i]);
      exit(0);
    }
  }

  exit(0);
}
