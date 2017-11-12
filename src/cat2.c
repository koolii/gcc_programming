#include <stdio.h>
#include <stdlib.h>

int
main(int argc, char *argv[])
{
  int i;

  for (i = 1; i < argc; i++) {
    FILE *f;
    int c;

    f = fopen(argv[i], "r");
    if (!f) {
      perror(argv[i]);
      exit(1);
    }

    // EOFに当たるまでfgetc()し続ける
    while ((c = fgetc(f)) != EOF) {
      // 標準出力への書き込みについてのエラーチェック
      // 標準出力がパイプの場合は、
      // そのパイプの先にいるプロセスが終了した後に書き込みでエラーが発生
      if (putchar(c) < 0) exit(1); // = putc(c, stdout)
    }
    fclose(f);
  }

  exit(0);
}
