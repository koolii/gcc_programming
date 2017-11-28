// RESULT
// ~/w/linux_system ❯❯❯ ./binary/httpd .
// GET /src/hello.c HTTP/1.0
//
// HTTP/1.0 200 OK
// Date: Mon, 27 Nov 2017 22:59:37 GMT
// Server: LittleHTTP/1.0
// Connection: close
// Content-Length: 97
// Content-Type: text/plain
//
// #include <stdio.h>
//
// int
// main(int argc, char *argv[])
// {
//   printf("Hello, world\n");
//   return 0;
// }
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
#define TIME_BUF_SIZE 64

/****** Function Prototypes **********************************************/
typedef void (*sighandler_t)(int);
static void install_signal_handlers(void);
static void trap_signal(int sig, sighandler_t handler);
static void signal_exit(int sig);
static void service(FILE *in, FILE *out, char *docroot);
static struct HTTPRequest* read_request(FILE *in);
static void read_request_line(struct HTTPRequest *req, FILE *in);
static struct HTTPHeaderField* read_header_field(FILE *in);
static void upcase(char *str);
static void free_request(struct HTTPRequest *req);
static long content_length(struct HTTPRequest *req);
static char* lookup_header_field_value(struct HTTPRequest *req, char *name);
static void respond_to(struct HTTPRequest *req, FILE *out, char *docroot);
static void do_file_response(struct HTTPRequest *req, FILE *out, char *docroot);
static void method_not_allowed(struct HTTPRequest *req, FILE *out);
static void not_implemented(struct HTTPRequest *req, FILE *out);
static void not_found(struct HTTPRequest *req, FILE *out);
static void output_common_header_fields(struct HTTPRequest *req, FILE *out, char *status);
static struct FileInfo* get_fileinfo(char *docroot, char *path);
static char* build_fspath(char *docroot, char *path);
static void free_fileinfo(struct FileInfo *info);
static char* guess_content_type(struct FileInfo *info);
static void* xmalloc(size_t sz);
static void log_exit(char *fmt, ...);
/****** Functions ********************************************************/

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

int
main(int argc, char *argv[])
{
  // 必ずdocrootが必要
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <docroot>\n", argv[0]);
    exit(1);
  }

  // シグナルのハンドラを宣言
  install_signal_handlers();
  // サービスを展開
  service(stdin, stdout, argv[1]);
  exit(0);
}

// 今この時点でこの処理がこのなかで一番大事
// やっていることはHTTPリクエストを展開して、reqのレスポンスをoutに書き込む
// 今回はdocrootを書き込むことになる
static void
service(FILE *in, FILE *out, char *docroot)
{
  struct HTTPRequest *req;

  req = read_request(in);
  respond_to(req, out, docroot);
  free_request(req);
}


/* ここからはHTTPRequestの処理 */

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

static char*
lookup_header_field_value(struct HTTPRequest *req, char *name)
{
  struct HTTPHeaderField *h;

  for (h = req->header; h; h = h->next) {
    if (strcasecmp(h->name, name) == 0) {
      return h->value;
    }
  }

  return NULL;
}

static long
content_length(struct HTTPRequest *req)
{
  char *val;
  long len;

  val = lookup_header_field_value(req, "Content-Length");
  if (!val) return 0;

  len = atoi(val);
  if (len < 0) log_exit("negative Content-Length value");

  return len;
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

// 大文字に置換する
static void
upcase(char *str)
{
  char *p;

  for (p = str; *p; p++) {
    *p = (char)toupper((int)*p);
  }
}

/* ここからはHTTPResponseの処理 */
// 下記の要項を揃えてレスポンスが完成
// ファイルの情報を得る
// その情報に従ってレスポンスを生成する

struct FileInfo {
  // 絶対パス
  char *path;
  // ファイルサイズ
  long size;
  int ok;
};

static struct FileInfo*
get_fileinfo(char *docroot, char *urlpath)
{
  struct FileInfo *info;
  struct stat st;

  info = xmalloc(sizeof(struct FileInfo));
  // docrootとurlからファイルシステム上のパスを生成する
  info->path = build_fspath(docroot, urlpath);
  info->ok = 0;

  // lstatで存在するか・そして普通のファイルか確認する
  if (lstat(info->path, &st) < 0) return info;
  if (!S_ISREG(st.st_mode)) return info;
  info->ok = 1;
  info->size = st.st_size;

  return info;
}

static char *
build_fspath(char *docroot, char *urlpath)
{
  char *path;

  // 夫々の + 1は "/" と末尾の "\0" のためにメモリの割当量を増やしている
  path = xmalloc(strlen(docroot) + 1 + strlen(urlpath) + 1);
  sprintf(path, "%s%s", docroot, urlpath);

  return path;
}

// 現在はHEAD、GETしか対応していない
static void
respond_to(struct HTTPRequest *req, FILE *out, char *docroot) {
  if (strcmp(req->method, "GET") == 0) do_file_response(req, out, docroot);
  else if (strcmp(req->method, "HEAD") == 0) do_file_response(req, out, docroot);
  else if (strcmp(req->method, "POST") == 0) method_not_allowed(req, out);
  else not_implemented(req, out);
}

static void
do_file_response(struct HTTPRequest *req, FILE *out, char *docroot)
{
  struct FileInfo *info;

  // docrootとパスからファイル情報を取得する
  info = get_fileinfo(docroot, req->path);
  if (!info->ok) {
    free_fileinfo(info);
    not_found(req, out);
    return;
  }

  // ここでヘッダー情報の出力を行う
  // output_common_header_fieldsはステータスコードを生成する共通処理
  output_common_header_fields(req, out, "200 OK");
  fprintf(out, "Content-Length: %ld\r\n", info->size);
  fprintf(out, "Content-Type: %s\r\n", guess_content_type(info));
  fprintf(out, "\r\n");

  // HEADじゃなければファイルの中身を出力する（今回の内容だとほぼcatと同一になる
  // レスポンスボディに相当する
  if (strcmp(req->method, "HEAD") != 0) {
    int fd;
    char buf[BLOCK_BUF_SIZE];
    ssize_t n;

    // ファイルディスクリプタを取得
    fd = open(info->path, O_RDONLY);
    if (fd < 0) log_exit("failed to open %s: %s", info->path, strerror(errno));
    for (;;) {
      // fdを読み込む
      n = read(fd, buf, BLOCK_BUF_SIZE);

      if (n < 0) log_exit("failed to read %s: %s", info->path, strerror(errno));
      if (n == 0) break;
      if (fwrite(buf, 1, n, out) < n) log_exit("failed to write to socket");
    }
    close(fd);
  }
  fflush(out);
  free_fileinfo(info);
}

static void
method_not_allowed(struct HTTPRequest *req, FILE *out)
{
  output_common_header_fields(req, out, "405 Method Not Allowed");
  fprintf(out, "Content-Type: text/html\r\n");
  fprintf(out, "\r\n");
  fprintf(out, "<html>\r\n");
  fprintf(out, "<header>\r\n");
  fprintf(out, "<title>405 Method Not Allowed</title>\r\n");
  fprintf(out, "<header>\r\n");
  fprintf(out, "<body>\r\n");
  fprintf(out, "<p>The request method %s is not allowed</p>\r\n", req->method);
  fprintf(out, "</body>\r\n");
  fprintf(out, "</html>\r\n");
  fflush(out);
}

static void
not_implemented(struct HTTPRequest *req, FILE *out)
{
  output_common_header_fields(req, out, "501 Not Implemented");
  fprintf(out, "Content-Type: text/html\r\n");
  fprintf(out, "\r\n");
  fprintf(out, "<html>\r\n");
  fprintf(out, "<header>\r\n");
  fprintf(out, "<title>501 Not Implemented</title>\r\n");
  fprintf(out, "<header>\r\n");
  fprintf(out, "<body>\r\n");
  fprintf(out, "<p>The request method %s is not implemented</p>\r\n", req->method);
  fprintf(out, "</body>\r\n");
  fprintf(out, "</html>\r\n");
  fflush(out);
}

static void
not_found(struct HTTPRequest *req, FILE *out)
{
  output_common_header_fields(req, out, "404 Not Found");
  fprintf(out, "Content-Type: text/html\r\n");
  fprintf(out, "\r\n");
  if (strcmp(req->method, "HEAD") != 0) {
      fprintf(out, "<html>\r\n");
      fprintf(out, "<header><title>Not Found</title><header>\r\n");
      fprintf(out, "<body><p>File not found</p></body>\r\n");
      fprintf(out, "</html>\r\n");
  }
  fflush(out);
}

static void
free_fileinfo(struct FileInfo *info)
{
  free(info->path);
  free(info);
}

// 今回は必ずtext/plainとなる
static char*
guess_content_type(struct FileInfo *info)
{
  return "text/plain";
}

static void
output_common_header_fields(struct HTTPRequest *req, FILE *out, char *status)
{
  time_t t;
  struct tm *tm;
  char buf[TIME_BUF_SIZE];

  t = time(NULL);
  tm = gmtime(&t);
  if (!tm) log_exit("gmtime() failed: %s", strerror(errno));

  strftime(buf, TIME_BUF_SIZE, "%a, %d %b %Y %H:%M:%S GMT", tm);
  fprintf(out, "HTTP/1.%d %s\r\n", HTTP_MINOR_VERSION, status);
  fprintf(out, "Date: %s\r\n", buf);
  fprintf(out, "Server: %s/%s\r\n", SERVER_NAME, SERVER_VERSION);
  fprintf(out, "Connection: close\r\n");
}
