#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

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
