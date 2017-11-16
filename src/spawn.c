#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

int
main(int argc, char *argv[])
{
  pid_t pid;

  if (argc != 3) {
    fprintf(stderr, "Usage: %s <command> <arg>\n", argv[0]);
    exit(1);
  }

  // fork()は次プロセスを複製して新しいプロセスを作るシステムコール
  // この時点で、「複製前のプロセス＝親プロセス」・「複製後のプロセス＝子プロセス」はどちらもfork()を呼び出した状態になっている
  // その為両方にfork()の戻り値が存在する
  //   * 子プロセスでの戻り値は0、fork()が失敗した場合には子プロセスは作成されない
  //   * 親プロセスの戻り値は子プロセスのプロセスID(自然数)、失敗した祭は-1となる
  pid = fork();
  if (pid < 0) {
    fprintf(stderr, "fork(2) failed\n");
    exit(1);
  }

  // 上述しているが、fork()の結果は親・子プロセスで呼び出しが戻るのでそれに対応させる必要がある
  if (pid == 0) {
    // execは次プロセスを新しいプログラムで上書きするシステムコール
    // execを実行すると、その時点で現在実行しているプログラムが消滅し
    // 自プロセス上に新しいプログラムをロードして実行する
    // exec("/bin/cat", ...)を実行すると今まで動かしていたプロセス上に/bin/catコマンドが実行される
    // 典型的な使い方は、fork()して即座にexecすること。これで新しいプログラムを実行したことになる
    // execlはコマンドライン引数を引数リストとして渡す関数
    // ↓の引数の渡し方に疑問がある場合はP260参照
    execl(argv[1], argv[1], argv[2], NULL);
    perror(argv[1]);
    exit(99);
  } else {
    int status;

    // fork()したプロセスの終了を待つには、wait()、waitpid()のシステムコールを使う
    // wait()は子プロセスのうちどれか一つが終了するのを待つ
    // waitpid()は第一引数と同じプロセスIDを持つプロセスが終了するのを待つ
    // また、第二引数にNULL以外を指定した場合は、そのアドレスに子プロセスの終了ステータスが格納されている
    waitpid(pid, &status, 0);
    printf("child (PID=%d) finished: ", pid);

    if (WIFEXITED(status)) {
      printf("exit, status=%d\n", WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
      printf("signal, sig=%d\n", WTERMSIG(status));
    } else {
      printf("abnormal exit\n");
    }

    // プロセスを自発的に終了するシステムコール_exit()
    // ライブラリ関数(libc)のexit()はシステムコールと異なり_exit()以外にも担保してくれている機能がある
    // * stdioのバッファを全部フラッシュする
    // * atexit()で登録した処理を実行する
    exit(0);
  }
}
