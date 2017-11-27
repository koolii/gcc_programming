#include <stdio.h>
#include <stdlib.h>

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
