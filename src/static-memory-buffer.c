#include <stdio.h>
#include <limits.h>

int
main(int argc, char const *argv[]) {
  char buf[PATH_MAX];

  // 昔はlimits.hのPATH_MAXを使っていた
  // PATH_MAXを超えたバイトが来るとエラーが発生する
  // そのためこの方法ではなくmalloc()を使用したメモリの確保を行うようにする
  if (!getcwd(buf, sizeof buf)) {
    /* occuer error */
    printf("%s\n", buf);
  }

  return 0;
}
