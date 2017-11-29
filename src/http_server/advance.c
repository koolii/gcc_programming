/*
 * 自力のソケット接続
 * damon化
 * syslogを使ったロギング
 * chroot()を使ったセキュリティ向上
 * それに伴うクレデンシャル変更のサポート
 * 以上すべての指示をするためのコマンドオプション解析
 */

#define USAGE "Usage: %s [--port=n] [--chroot --user=u --group=g] [--debug] <docroot>\n"

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
