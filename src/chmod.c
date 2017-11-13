#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

int
main(int argc, char *argv[]) {
  int mode;
  int i;

  if (argc < 2) {
    fprintf(stderr, "no mode give\n");
    exit(1);
  }

  // strtolで8を基数とした数値に変換することができる
  // 第二引数はエラー処理を細かくしたいときに使う
  mode = strtol(argv[1], NULL, 8);
  for (i = 2; i < argc; i++) {
    if (chmod(argv[i], mode) < 0) {
      perror(argv[i]);
    }
  }

  exit(0);
}
