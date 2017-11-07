#include <stdio.h>
#include <stdlib.h>

static void do_head(FILE *f, long nlines);

int
main(int argc, char *argv[])
{
  long nlines;

  if (argc < 2) {
    fprintf(stderr, "Usage: %s n\n", argv[0]);
    exit(1);
  }

  // atolは文字列をlong型に変換
  // atoiは文字列をint型に変換
  // 一番最初の引数は何行表示させるのか設定
  nlines = atol(argv[1]);
  if (argc == 2) {
    do_head(stdin, nlines);
  } else {
    int i;

    // 引数2つ目以降は出力するパスを指定する(可変長引数)
    for (i = 2; i < argc; i++) {
      FILE *f;

      f = fopen(argv[i], "r");
      if (f == NULL) {
        perror(argv[i]);
	exit(1);
      }

      do_head(f, nlines);
      fclose(f);
    }
  }

  exit(0);
}

static void
do_head(FILE *f, long nlines)
{
  int c;

  if (nlines <= 0) return;
  // fgetsよりも優位な点
  // * getcはバッファが必要ないから楽
  // * fgets()だと行の長さが制限される
  // * getcで解決できる
  while ((c = getc(f)) != EOF) {
    if (putchar(c) < 0) exit(1);
    //「行」は「\nで終わる文字列」となるので
    // \nがn回出てくるまでループをすればOK
    if (c == '\n') {
      nlines--;
      if (nlines == 0) return;
    }

  }

}
