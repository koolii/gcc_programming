#include <stdio.h>
#include <stdlib.h>

// getopt_long()のプロトタイプ宣言を取得するための定義
#define _GNU_SOURCE
#include <getopt.h>

static void do_head(FILE *f, long nlines);

#define DEFAULT_N_LINES 10

// 下記を同時に実行
// * longopts変数を宣言
// * 配列を定義(※最後の0の配列はgetopt_long()側にオプションが最後であることを伝えたいから)
static struct option longopts[] = {
  {"lines", required_argument, NULL, 'n'},
  {"help", no_argument, NULL, 'h'}, // 「--help == -h」と定義
  {0, 0, 0, 0},
};

int
main(int argc, char *argv[])
{
  int opt;
  long nlines = DEFAULT_N_LINES;

  while ((opt = getopt_long(argc, argv, "n:", longopts, NULL)) != -1) {
    switch (opt) {
    case 'n':
      // optarg内にオプションに渡したパラメータが格納されている
      nlines = atol(optarg);
      break;
    case 'h':
      fprintf(stdout, "Usage: %s [-n LINES] [FILE ...]\n", argv[0]);
      exit(0);
    case '?':
      fprintf(stderr, "Usage: %s [-n LINES] [FILE ...]\n", argv[0]);
      exit(1);
    }
  }

  // optindはgetopt() or getopt_long()をループを抜けた時点での
  // オプションではない最初の引数のインデックスになる
  if (optind == argc) {
    do_head(stdin, nlines);
  } else {
    int i;

    for (i = optind; i < argc; i++) {
      FILE *f;

      f = fopen(argv[i], "r");
      if (!f) {
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
