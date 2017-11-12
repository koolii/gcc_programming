#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int
main(int argc, char *argv[])
{
  if (argc != 3) {
    fprintf(stderr, "%s: wrong arguments\n", argv[0]);
    exit(1);
  }

  // sysmlinkはファイルのシンボリックリンクを生成するシステムコール
  // 特徴
  // * シンボリックリンクには対応する実態が存在しなくても良い
  // * ファイルシステムをまたいで別名を付けられる(ハードリンクはできない)
  // * ディレクトリにも別名が付けられる(ハードリンクはできない)
  if (symlink(argv[1], argv[2]) < 0) {
    perror(argv[1]);
    exit(1);
  }

  exit(0);
}

// $ ./binary/sysmlink src/symlink.c anothername
// $ ls -l
// lrwxr-xr-x   1 koolii  staff     8B Nov 12 22:21 anothername -> src/symlink.c
