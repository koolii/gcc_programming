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

// HTTPHeaderFieldは自分自身のstructも持っており、リンクリストとなっている
// HTTPHeaderField -> (next) -> HTTPHeaderField -> ... -> NULL
struct HTTPHeaderField {
  char *name;
  char *value;
  struct HTTPHeaderField *next;
}
struct HTTPRequest {
  int protocol_minor_version;
  char *method;
  char *path;
  struct HTTPHeaderField *header;
  char *body;
  long length;
}

// free()した時点でhは無効になっているため、h->nextを参照するとsegmentation-faultが発生する
// 予めh->nextを取得しておかければならない
static void
free_request(struct HTTPRequest *req) {
  struct HTTPHeaderField *h, *head;

  head = req->head;
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

// ソケット接続は何かのはずみで突然切れるものなので
// 切断時の対応を行う(xinetdはシグナルSIGPIPEが送られる)
static void
install_signal_handlers(void)
{
  trap_signal(SIGPIPE, signal_exit);
}

static struct HTTPRequest*
read_request(FILE *in)
{
  struct HTTPRequest *req;
  struct HTTPHeaderField *h;

  req = xmalloc(sizeof(struct HTTPRequest));

  // リクエストライン ("GET /path/to/file HTTP/1/1")を読み、解析してHTTPRequestに書き込む
  read_request_line(req, in);
  req->head = NULL;

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

// シグナル用のエラー処理
static void
signal_exit(int sig)
{
  log_exit("exit by signal %d", sig);
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
