#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <regex.h>

static void do_grep(regex_t *pat, FILE *f);

int
main(int argc, char *argv[])
{
  regex_t pat;
  int err;
  int i;

  if (argc < 2) {
    fputs("no pattern\n", stderr);
    exit(1);
  }

  // 文字列を正規表現用のパターンregex_tに変換
  err = regcomp(&pat, argv[1], REG_EXTENDED | REG_NOSUB | REG_NEWLINE);
  if (err != 0) {
    char buf[1024];

    regerror(err, &pat, buf, sizeof buf);
    puts(buf);
    exit(1);
  }

  // パターンしか引数に設定していない場合は標準入力を受け付ける
  if (argc == 2) {
    do_grep(&pat, stdin);
  } else {
    // 引数にパターンとファイル名を指定している場合はファイル内を検索
    for (i = 2; i < argc; i++) {
      FILE *f;

      // ストリームを取得
      f = fopen(argv[i], "r");
      if (!f) {
        perror(argv[i]);
        exit(1);
      }

      do_grep(&pat, f);
      fclose(f);
    }
  }

  regfree(&pat);
  exit(0);
}

static void
do_grep(regex_t *pat, FILE *src) {
  char buf[4096];

  // bufにsrcの中身を一行分代入する
  while (fgets(buf, sizeof buf, src)) {
    // パターンマッチ
    if (regexec(pat, buf, 0, NULL, 0) == 0) {
      fputs(buf, stdout);
    }
  }
}
