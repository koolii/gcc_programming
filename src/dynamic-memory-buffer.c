#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#define INIT_BUFSIZE 1024

char*
my_getcwd(void)
{
  char *buf, *tmp;
  size_t size = INIT_BUFSIZE;

  // 一旦バッファを確保し、パスを取得する
  // 失敗したらrealloc()でバッファの領域を増やし再試行する
  buf = malloc(size);
  if (!buf) return NULL;

  for (;;) {
    errno = 0;
    if (getcwd(buf, size)) {
      return buf;
    }

    if (errno != ERANGE) break;

    size *= 2;
    tmp = realloc(buf, size);

    if (!tmp) break;
    buf = tmp;
  }

  free(buf);
  return NULL;
}
