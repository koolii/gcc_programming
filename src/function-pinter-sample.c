#include <stdio.h>

int
plus1(int n)
{
  return n + 1;
}

int
main(int argc, char *argv[])
{
  // 関数を指すポインタ変数ｆを定義
  int (*f)(int);
  int result;

  // 関数を代入
  f = plus1;
  result = f(5);
  printf("%d\n", result);

  return 0;
}
