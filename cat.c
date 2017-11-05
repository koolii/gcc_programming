#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

static void do_cat(const char *path);
static void die(const char *s);

// より小さい範囲だけを読んで理解できるのが「よい」コード

int
main(int argc, char *argv[]) {
  int i;

  if (argc < 2) {
    fprintf(stderr, "%s: file name not given\n", argv[0]);
    exit(1);
  }

  for (i = 1; i < argc; i++) {
    do_cat(argv[i]);
  }

  exit(0);
}

#define BUFFER_SIZE 2048

static void
do_cat(const char *path)
{
  int fd;
  unsigned char buf[BUFFER_SIZE];
  int n;

  // catコマンドは書き込む必要がないため読み込み専用でファイルを開く
  fd = open(path, O_RDONLY);
  if (fd < 0) die(path);

  for(;;) {
    // ファイルディスクリプタfdに紐づくストリームからバッファを読み込む
    // 読み込むサイズはdefineで設定しているように2048バイトとなる
    n = read(fd, buf, sizeof buf);
    // check error
    if (n < 0) die(path);
    // check file end
    if (n == 0) break;
    // 標準出力に読み込んだバッファを書き込み、結果をエラー判定している
    if (write(STDOUT_FILENO, buf, n) < 0) die(path);
  }

  if (close(fd) < 0) die(path);
}

static void
die(const char *s) {
  // errnoの値に合わせたエラーメッセージを標準エラー出力に出力する関数
  // 一般的にシステムコールが失敗した時は失敗原因の定数がグローバル変数errnoにセットされる
  // perrorの引数には大体、失敗する原因になった呼び出しの引数から主要な文字列を選んで渡せば成功する
  perror(s);
  exit(1);
}

