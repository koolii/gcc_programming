#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <ctype.h>
#include <signal.h>

#define SERVER_NAME "LittleHTTP"
#define SERVER_VERSION "1.0"
#define HTTP_MINOR_VERSION 0
#define BLOCK_BUF_SIZE 1024
#define LINE_BUF_SIZE 4096
#define MAX_REQUEST_BODY_LENGTH (1024 * 1024)

typedef void (*sighandler_t)(int);

// HTTPHeaderFieldは自分自身のstructも持っており、リンクリストとなっている
// HTTPHeaderField -> (next) -> HTTPHeaderField -> ... -> NULL
struct HTTPHeaderField {
  char *name;
  char *value;
  struct HTTPHeaderField *next;
};
struct HTTPRequest {
  int protocol_minor_version;
  char *method;
  char *path;
  struct HTTPHeaderField *header;
  char *body;
  long length;
};

// free()した時点でhは無効になっているため、h->nextを参照するとsegmentation-faultが発生する
// 予めh->nextを取得しておかければならない
static void
free_request(struct HTTPRequest *req) {
  struct HTTPHeaderField *h, *head;

  head = req->header;
  while(head) {
    h = head;
    // warning!!
    head = head->next;
    free(h->name);
    free(h->value);
    free(h);
    // ERROR: ここだとエラーとなる
    // head = head->next;
  }
  free(req->method);
  free(req->path);
  free(req->body);
  free(req);
}

// エラーの際の標準出力処理
static void
log_exit(char *fmt, ...)
{
  va_list ap;

  // 可変長の開始宣言
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fputc('\n', stderr);
  // 可変長の終了宣言
  va_end(ap);
  exit(1);
}

// メモリ割り当てが失敗した時のエラー処理を追加したmalloc処理
// ここでは一度失敗したらlog_exit()のexit(1)をするためNULLが帰ることはない
static void*
xmalloc(size_t sz)
{
  void* p;

  p = malloc(sz);
  if (!p) log_exit("failed to allocate memory");
  return p;
}

// シグナル用のエラー処理
static void
signal_exit(int sig)
{
  log_exit("exit by signal %d", sig);
}

// 13章の内容とほぼ同じで、sigaction()を使ってsignal()と似たようなインターフェースを作る
static void
trap_signal(int sig, sighandler_t handler)
{
  struct sigaction act;

  act.sa_handler = handler;
  sigemptyset(&act.sa_mask);
  act.sa_flags = SA_RESTART;

  if (sigaction(sig, &act, NULL) < 0) {
    log_exit("sigaction() failed: %s", strerror(errno));
  }
}

// ソケット接続は何かのはずみで突然切れるものなので
// 切断時の対応を行う(xinetdはシグナルSIGPIPEが送られる)
static void
install_signal_handlers(void)
{
  trap_signal(SIGPIPE, signal_exit);
}

// 大体はread_request_lineと同じ
static struct HTTPHeaderField*
read_header_field(FILE *in)
{
  struct HTTPHeaderField *h;
  char buf[LINE_BUF_SIZE];
  char *p;

  if (!fgets(buf, LINE_BUF_SIZE, in)) {
    log_exit("failed to read request header field: %s", strerror(errno));
  }

  // 読み込んだ行が空行かどうか判定
  if ((buf[0] == '\n') || (strcmp(buf, "\r\n") == 0)) return NULL;

  p = strchr(buf, ':');
  if (!p) log_exit("parse error on request header field: %s", buf);

  *p++ = '\0';
  h = xmalloc(sizeof(struct HTTPHeaderField));
  h->name = xmalloc(p - buf);
  strcpy(h->name, buf);

  // strspn(const char *str, const char *accept)
  // 文字列に含まれる文字acceptだけで構成される部分が文字列strの戦闘に何文字あるか数え、その長さを返す
  p += strspn(p, " \t");
  h->value = xmalloc(strlen(p) + 1);
  strcpy(h->value, p);

  return h;
}

// 下記のような文字列を解析する箇所
// GET''/path/to/file''HTTP/1.0\0
static void
read_request_line(struct HTTPRequest *req, FILE *in)
{
  char buf[LINE_BUF_SIZE];
  char *path, *p;

  // 一行読み込む
  if (!fgets(buf, LINE_BUF_SIZE, in)) log_exit("no request line");

  // 空白(' ')まで読み込む
  p = strchr(buf, ' '); /* p(1) = GET */
  if (!p) log_exit("parse error on request line (1): %s", buf);

  // 「現在指している位置に'\0'を代入してからポインタを1進める」という意味
  *p++ = '\0';
  req->method = xmalloc(p - buf);
  strcpy(req->method, buf);
  upcase(req->method);

  path = p;
  p = strchr(path, ' '); /* p (2) = /path/to/file */
  if (!p) log_exit("parse error on request line (2): %s", buf);

  *p++ = '\0';
  req->path = xmalloc(p - path);
  strcpy(req->path, path);

  // アルファベットの大文字小文字の区別を無視して文字列を比較する
  // 内容が同じなら0を返す
  if (strncasecmp(p, "HTTP/1.", strlen("HTTP/1.")) != 0) {
    log_exit("parse error on request line (3): %s", buf);
  }

  p += strlen("HTTP/1."); /* p (3) = HTTP/1.0の0が代入される */
  req->protocol_minor_version = atoi(p);
}

static struct HTTPRequest*
read_request(FILE *in)
{
  struct HTTPRequest *req;
  struct HTTPHeaderField *h;

  req = xmalloc(sizeof(struct HTTPRequest));

  // リクエストライン ("GET /path/to/file HTTP/1/1")を読み、解析してHTTPRequestに書き込む
  read_request_line(req, in);
  req->header = NULL;

  // リクエストヘッダを一つ読み込んでHTTPHeaderFieldを返す
  while (h = read_header_field(in)) {
    h->next = req->header;
    req->header = h;
  }

  // HTTPリクエストにエンティティボディが存在するときは、
  // クライアントがエンティティボディの長さをContent-Lengthフィールドに書くことになっている
  // content_lengthは必ず0以上を返す
  req->length = content_length(req);

  if (req->length != 0) {
    // サイズチェック
    if (req->length > MAX_REQUEST_BODY_LENGTH) {
      log_exit("request body too long");
    }

    req->body = xmalloc(req->length);
    // freadでエンティティボディを読み込む
    if (fread(req->body, req->length, 1, in) < 1) {
      log_exit("failed to read request body");
    }
  } else {
    req->body = NULL;
  }

  return req;
}

// 今この時点でこの処理がこのなかで一番大事
// やっていることはHTTPリクエストを展開して、reqのレスポンスをoutに書き込む
// 今回はdocrootを書き込むことになる
static void
service(FILE *in, FILE *out, char *docroot)
{
  struct HTTPRequest *req;

  // TODO
  req = read_request(in);
  respond_to(req, out, docroot);
  free_request(req);
}

int
main(int argc, char *argv[])
{
  // 必ずdocrootが必要
  if (argc != 2) {
    fprinf(stderr, "Usage: %s <docroot>\n", argv[0]);
    exit(1);
  }

  // シグナルのハンドラを宣言
  install_signal_handlers();
  // サービスを展開
  service(stdin, stdout, argv[1]);
  exit(0);
}
