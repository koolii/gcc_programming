/*
 * 自力のソケット接続
 * damon化
 * syslogを使ったロギング
 * chroot()を使ったセキュリティ向上
 * それに伴うクレデンシャル変更のサポート
 * 以上すべての指示をするためのコマンドオプション解析
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <ctype.h>
#include <signal.h>
#include <pwd.h>
#include <grp.h>
#include <syslog.h>
#define _GNU_SOURCE
#include <getopt.h>

/****** Constants ********************************************************/

#define SERVER_NAME "LittleHTTP"
#define SERVER_VERSION "1.0"
#define HTTP_MINOR_VERSION 0
#define BLOCK_BUF_SIZE 1024
#define LINE_BUF_SIZE 4096
#define MAX_REQUEST_BODY_LENGTH (1024 * 1024)
#define MAX_BACKLOG 5
#define DEFAULT_PORT "80"
#define USAGE "Usage: %s [--port=n] [--chroot --user=u --group=g] [--debug] <docroot>\n"

/****** Data Type Definitions ********************************************/

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

struct FileInfo {
    char *path;
    long size;
    int ok;
};

/****** Function Prototypes **********************************************/

static void setup_environment(char *root, char *user, char *group);
typedef void (*sighandler_t)(int);
static void install_signal_handlers(void);
static void trap_signal(int sig, sighandler_t handler);
static void detach_children(void);
static void signal_exit(int sig);
static void noop_handler(int sig);
static void become_daemon(void);
static int listen_socket(char *port);
static void server_main(int server, char *docroot);
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
static void log_exit(const char *fmt, ...);

/****** Functions ********************************************************/

static int debug_mode = 0;

// debugで第三引数に変数のアドレスを渡していることに注意
// これでdebug_modeに値が代入される
// --debugオプションを使うと変数debug_modeに1が代入
// 第三引数は真偽値を指定するオプションで使うと便利
static struct option longopts[] = {
  {"debug",  no_argument,       &debug_mode, 1},
  {"chroot", no_argument,       NULL, 'c'},
  {"user",   required_argument, NULL, 'u'},
  {"group",  required_argument, NULL, 'g'},
  {"port",   required_argument, NULL, 'p'},
  {"help",   no_argument,       NULL, 'h'},
  {0, 0, 0, 0}
}

int
main(int argc, char *argv[])
{
  int server_fd;
  char *port = NULL;
  char *docroot;
  in do_chroot = 0;
  char *user = NULL;
  char *group = NULL;

  while ((opt = getopt_long(argc, argv, "", longopts, NULL)) != -1) {
    switch (opt) {
    case 0:
      break;
    case 'c':
      do_chroot = 1;
      break;
    case 'u':
      user = optarg;
      break;
    case 'g':
      group = optarg;
      break;
    case 'p':
      port = optarg;
      break;
    case 'h':
      fprintf(stdout, USAGE, argv[0];
      exit(0);
    case '?':
      fprintf(stdout, USAGE, argv[0];
      exit(1);
    }
  }

  if (optind != argc - 1) {
    fprintf(stderr, USAGE, argv[0]);
    exit(1);
  }

  docroot = argv[optind];

  if (do_chroot) {
    setup_environment(docroot, user, group);
    docroot = "";
  }

  install_signal_handlers();

  server_fd = listen_socket(port);

  // デバッグモードのときはデーモンにならず、標準入出力を端末に繋いだままで動作
  // こうしておくとエラーメッセージを標準エラー出力にかけるから
  if (!debug_mode) {
    openlog(SERVER_NAME, LOG_PID|LOG_NDELAY, LOG_DAEMON);
    become_daemon();
  }

  server_main(server_fd, docroot);
  exit(0);
}
