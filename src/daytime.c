#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

static int open_connection(char *host, char *service);

int
main(int argc, char *argv[])
{
  int sock;
  FILE *f;
  char buf[1024];

  // 第二引数のdaytimeはポート番号を指定する必要があり、ウェルノウンポートの文字列ならば指定できる
  sock = open_connection((argc > 1 ? argv[1] : "localhost"), "daytime");
  f = fdopen(sock, "r");

  if (!f) {
    perror("fdopen(3)");
    exit(1);
  }

  // ソケットを読み込んだらあとは単純に標準出力するだけ
  fgets(buf, sizeof buf, f);
  fclose(f);
  fputs(buf, stdout);
  exit(0);
}

static int
open_connection(char *host, char *service)
{
  int sock;
  struct addrinfo hints, *res, *ai;
  int err;

  // 候補を絞り込むためのヒントを与える必要がある
  // 今回は下記のような設定にしている
  // AF_UNSPEC: IPv4,IPv6のどちらでもよい
  // SOCK_STREAM: パケットではなくストリーム形式の接続を使う=TCP
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  if ((err = getaddrinfo(host, service, &hints, &res)) != 0) {
    fprintf(stderr, "getaddrinfo(3): %s\n", gai_strerror(err));
    exit(1);
  }

  // 結局ここで行っているのは接続がうまく行った接続先を選択しているに過ぎない
  for (ai = res; ai; ai = ai->ai_next) {
    sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if (sock < 0) {
      continue;
    }

    if (connect(sock, ai->ai_addr, ai->ai_addrlen) < 0) {
      close(sock);
      continue;
    }

    /* success */
    freeaddrinfo(res);
    return sock;
  }

  /* failure */
  fprintf(stderr, "sock(2)/connect(2) failed");
  freeaddrinfo(res);
  exit(1);
}

// 今回は検証にinetdやxinetdをというサーバの内部プログラムとして用意されているものを使う
// inetd: ネットワーク接続部分だけを引き受けてくれる特別なサーバ(インターネットスーパーサーバとも呼ばれる)
// inetdは指定されたポートで待機し、クライアントが接続してくるのを待つ
// 接続が完了したら、シェルと同じ要領でdup()を使ってソケットを標準入力と標準出力に移し、サーバプログラムをexecする
// 結果、プログラムは標準入出力を相手にしているだけでネットワーク通信ができてしまう(xinetdは改良版)
