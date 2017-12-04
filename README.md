https://github.com/aamine/stdlinux2-source

# ふつうのLinuxプログラミング［第二版］Linuxの仕組みから学べるgccプログラミングの王道

## 2章OS

LinuxとはOSやディストリビューションのことのように見えるが実際はカーネルのこと

### Linuxのセカイを構成する３つの要素
* ファイルシステム
* プロセス
* ストリーム

### ハードウェア操作でのカーネルの役割
カーネルは役割としてハードウェアの統括で、CPUやメモリ、HDDやCDドライブ等のデバイスを操作します
上記のように一言でいうと単純だがHDDを一つとっても沢山の種類が夫々に適したデバイスドライバ（単にドライバとも言う）を持っている

### システムコール
普通のプログラムがハードウェアを操作したいときはカーネルに仕事を頼んで間接的に操作するしかありません
カーネルは一番偉いプログラムだが、その一方で一番下っ端の仕事をしている
カーネルに仕事を頼むにはシステムコールの仕組みを使う

システムコールの「システム」は要するにカーネルのことで、「システムを呼び出す（call）」から名前が付いている
Linuxには下記のようなシステムコールが存在している

* open
* read
* write
* fork
* exec
* stat
* unlink

だが、システムコールも別に普通のプログラムで使われていて

```c
n = read(fd, buf, sizeof buf);
```

## ライブラリ

## libc
Linuxに用意されているライブラリは色々ありますが、
その中でも特に重要なライブラリが標準Cライブラリ（libc）
Linuxで普通使われているlibcはGNU libc（glibc）といって、その名の通りGNUプロダクトの一部
GNU libc自体はLinusさんが作ってるわけではないのでよく「Linuxはカーネルだけ」＝Linuxが作ってるのはLinusさんだけ

### API
APIは何かを使ってプログラミングするときのインターフェイスのことを指し、例えば
CのライブラリのAPIは関数やマクロです・カーネルのAPIはシステムコールとなる
かなりの数の人が「API」と「システムコール」を同じ意味だと思っているらしい

## 3章Linuxを生み出す3つの概念

### ファイルシステム
まずはファイルを理解する必要がある

* 広義のファイル
* 狭義のファイル
* ストリーム

### 広義のファイル
簡単に言えば`ls`コマンドで表示される一覧をファイルと呼べる

* 普通のファイル = **狭義のファイル**
  * 内容がそのまま記録されているファイル(テキスト、動画、写真、音声)
* ディレクトリ
  * 他のファイルを複数入れることができるファイル
* シンボリックリンク(ソフトリンク)
  * 主な使いみちは、ファイルやディレクトリに別名をつけること
* デバイスファイル
  * デバイス(ハードウェア)をファイルとして表現したもの
  * /dev/sdaは一台目のSSDまたはHDDを表している
  * /dev/nullは対応デバイスがないファイルで、読み込むと常に空、何かを書き込むとどこかに消えてしまうという不思議ファイル
* 名前付きパイプ
  * プロセス間通信に使うファイルでFIFOとも呼ばれる
* UNIXドメインソケット
  * プロセス間通信で使うファイルで、現在はTCPソケットで代替できる

#### まとめ
* どんなファイルであれ **何れかのデータを保持する**
* ファイルにはパーミッションや更新時刻等の **付帯情報がある**
* **名前（パス）で指定できる** ことはファイルに欠かせない特性

### プロセス
* 簡単に言えば「動作中のプログラム(=ファイルのような形態で存在しているデータも含む)」
* １つのプログラムから複数のプロセスを作成することができる

#### シグナル
* プロセスIDが代表的な例として、シグナルがある
* シグナルは`Ctrl+C`を押下した時の仕組みを支えていて、
* 流れはカーネルが該当プロセスに割り込みシグナル(*SIGINT*)を送り、それを受け取ったプロセスが自発的に終了します

### ストリーム(本書ではバイトストリームと同値)
* ストリームは、バイト列が流れる通り道だと考える
* プロセスがファイルやハードウェア(キーボード等)の内容にアクセスする時の橋渡し
  * ストリームからバイト列を読む = **read**
  * ストリームからバイト列を書く = **write**
* パイプの仕組みもストリームになっている
  * まず各コマンドを独立したプロセスとして同時に実行する
  * そのプロセス間をストリーム(パイプでつなぐ)
* ネットワーク間の通信でもストリーム(どっちかって言うとプロセス)
* パイプのように、プロセス同士がストリームを通じてデータをやりとりしたり意思の疎通を図ることを **プロセス間通信(IPC)** と言う
  * ストリームだけがプロセス間の橋渡しではない

##### 他書では
* 単に「ファイル」や「open file」と呼ばれている
* 「FILE型の値」や「STREAMSカーネルモジュール」の意味で使われている

### まとめ
* ファイルシステム
  * データに名前をつけて保存する場所
* プロセス
  * 何らかの活動をする主体
* ストリーム
  * プロセスがファイルシステムや他のプロセスとデータをやり取りする手段

## 4章Linuxとユーザ

### グループ
* ユーザをグループに所属させることができ、複数のグループに所属させることもでき、これらのグループは補足グループと呼ばれる
### パーミッション
* r=4
* w=2
* x=1
* -=0
* ディレクトリは不鬱のファイルとは少し違っていて、
  * 読み書きについては、ディレクトリを「その中にあるファイルのリストを記録したファイル」と考える
  * 実行に関しては、内部のファイルのパーミッションに関係なく、一切アクセスできなくなります

### クレデンシャル
例えばユーザAが所有しているファイル(rw-r--r--)があったとして、ユーザAとしてアクセスすればこのファイルを読み書きができます
「ユーザAとしてアクセス」するとは「ユーザAの属性を持ったプロセスがアクセスする」ということになる
Linux上で活動する主体はユーザではなくプロセスとなるのでユーザAの属性を持った別のユーザがアクセスすることができる
ここでのプロセスの属性としてのユーザのことをクレデンシャルといい、要するにユーザの代わりに代理として(プロセスが)動作していることを表す証明書
ログインの過程で、証明書をもつプロセスがシステム上に作成され、他のプロセスを実行するときに証明書をコピーされる

### /etc/passwd

```
daemon:x:1:10:daemon:/usr/sbin:/usr/sbin/nologin

```
* ユーザdaemonのユーザIDが1、グループIDが10となる
* グループの情報は/etc/groupに記載されている

### シェルと端末

* Unixではtty(=端末)となるがこれはテレタイプから来ている
* ビットマップディスプレイは細かい色の点の集まりで文字などを表現している

現代的なGUIを備えたPCで「a」を表示するときは、予めメモリ上に「a」を表現する画像を作っておき、「この画像を表示せよ」と命令します
具体的に、端末に「aという文字を表示しろ」と指定するには、文字と数値を対応付けてその数値を伝えていて、最も普及した規則がACSCII
端末もファイルとして表現されていて/dev以下に端末に対応するデバイスファイルが用意されている

このデバイスファイルが接続するためのストリームが得られるので、端末に繋がったストリームを読むとキーボードからの入力が得られ、
端末に繋がったストリームに書くとディスプレイに文字を出力できます

**そのためUNIXでは色々なものをファイルとして表現しようとする特徴があります**

## 5章ストリームにかかわるシステムコール

### 概略(ではあるがLinuxの入出力はこれでほぼ語り尽くせてしまう)
* ストリームからバイト列を読み込むread
* ストリームにバイト列を書き込むwrite
* ストリームを作るopen
* 用済みのストリームを始末するclose

### ファイルディスクリプタ
すでに説明済みだが、プロセスがファイルを読み書きしたり他のプロセスとやり取りをするときにはストリームを使いますが、
その使い方について触れる

プログラムからストリームを扱うにはファイルディスクリプタというものを使います
ファイルディスクリプタとはプログラムから見るとただの整数値(int)
理由はプロセスからカーネル内の実際のストリーム(を管理するデータ構造)を直接触れることができないため、
紐付けられている数値でストリームの操作をカーネルに依頼する

### 標準入出力・エラー出力
一番手間がないストリームが下記(ストリーム番号)
* 標準入出(0番)
* 標準出力(1番)
* 標準エラー出力(2番)

コマンドをパイプで繋いでデータを処理することができるのは
コマンドが標準入力からデータを読み込み、処理結果を標準出力に書き込むようになっているから

具体的な例として`$ grep print < hello.c | head`で考える
hello.cファイルを標準入力としてgrepプロセスに渡し、その標準入力をそのまま標準入力にしてheadプロセスに渡し、最終的に端末に結果を出力

### ストリームの読み書き

#### read(2)
この「(2)」はシステムコールを表す表現

```c
#include <unistd.h>

ssize_t read(int fd, void *buf, size_t bufsize);
```

`ssize_t`や`size_t`はどちらもsys/types.hで定義されている、基本的な整数型のエイリアスにすぎない
別名にする理由はカーネルやOS、CPUにかかわらず同じソースを使えるようにするため

`read`はfd番のストリームからバイト列を読み込むシステムコール
最大bufsizeバイト読み、bufに格納する(慣習としてbufのサイズをそのままbufsizeに指定する)

* 正常完了したら読み込んだバイト数を返す
* ファイル終端に達したときは0を返す
* エラーが起きたときは-1を返す

* read()はbufsizeバイトより少ないバイト数しか読まないケースも頻繁に起きるので返り値チェックが必須
* c言語の文字列は任意のバイト列が格納できるが、人間が読める文字列の場合は'\0'が終端になるのが慣習
* ただAPI毎に'\0'を終端と前提するものとしないものがあり
  * read()は'\0'が終端の前提になっていない
  * print()は'\0'が終端の前提になっている
  * read()で読み込んだ文字列をそのままprint()に渡すと単に結果が変になるだけでなく、セキュリティホールになりえる為間違い

#### write(2)
```c
#include <unistd.h>

ssize_t write(int fd, const void *buf, size_t bufsize);
```

* 正常に書き込んだときは書いたバイト数を返す
* エラーが起きたときは-1を返す

エラーが起きる可能性は低いが、厳密な処理では戻り値をチェックすべき

### ストリームの定義
本書のストリームは、ファイルディスクリプタで表現され、read()またはwrite()で操作できるもののこと
例えば、ファイルをopen()するとread()またはwrite()を実行できるものが作られるから、そこにストリームがある

### ファイルを開く
明示的に新しいストリームを作る方法について

#### open(2)
ファイルに接続するストリームを作成する

````c
# include <sys/types.h>
# include <sys/stat.h>
# include <fcntl.h>

int open(const char *path, int flags);
int open(const char *path, int flags, mode_t mode);
````

第一引数pathのパスで示されるファイルにつながるストリームを作成し、そのストリームを指すファイルディスクリプタを返します
この処理をファイルを開くと言う

#### close(2)
ストリームを始末するシステムコール

````c
# include <unistd.h>

int close(int fd);
````

ファイルディスクリプタfdに関連付けられているストリームを始末する
返り値が0なら問題なし、-1ならエラーとなる

本当のところ、close()をしなくても良いが、同時に接続できるプロセス数にも限度があるし
プロセスが終了できないときもあるので基本的にclose()することが大事

### その他のシステムコール

同じファイルディスクリプタに対して何度もread()システムコールを呼ぶと、必ず前回の続きが帰って来るということは
ファイルをどこまで読んだのかが記憶されているということになる、その位置のことをファイルオフセットという

````c
# include <types.h>
# include <unistd.h>

off_t lseek(int fd, off_t offset, int whence);
````

この値はシステムコールで操作でき、代表的なシステムコールはlseek()で指定した位置offsetに移動する
移動方法が３通りあり、それをwhence変数に指定する（P97参照）

## 6章ストリームにかかわるライブラリ関数
システムコールを直接使ってプログラムを作ると、バイト単位で設定したりシステムコールの処理速度の関係で
標準入出力ライブラリstdio(libcの大部分)のバッファリングを使いカーネルの上に一つ層をまたいだAPIを使う

FILE型はファイルディスクリプタを内部でラッパーした型でバッファ＋ストリームと考えると覚えやすいかも
* fileno(3) 引数のstreamがラップしているファイルディスクリプタを返す
* fdopen(3) ファイルディスクリプタをラップするFILE型の値を新しく生成してポインタを返す

* foepn(3) 標準入出力以外をストリームに対応させる(システムコールのopen)
* fclose(3) システムコールのclose
* fgetc(3) ストリームから1バイト読み込んで返す
* fputc(3) ストリームにバイトを書き込む
* getc(3) fgetcと全く同じだが、こっちはマクロとして定義されている
* putc(3) fputcと。。。
* getchar(3) getc(stdin)と同じ
* putchar(3) putc(c, stdout)と同じ* getc(3) fgetcと全く同じだが、こっちはマクロとして定義されている
* putc(3) fputcと。。。
* getchar(3) getc(stdin)と同じ
* putchar(3) putc(stdin)と同じ
* ungetc(3) １バイトをバッファに「戻す」APIで、１つのストリームに連続使用が出来ない、入力を単語に区切っていく時によく使う
  * e.g. +と=が区切り文字と識別したりするのに使用したりする => 1928+26=1954
* fgets(3) ストリームから１行読み込んで、第一引数に格納し、最大サイズsize-1までしかデータを読み込まない(末尾に\0を書き込むから)
  * getsはバッファサイズを指定できず、もしも第一引数に用意したバッファよりも一行が大きいと即死
* fputs(3) fputsとは違い一行単位ずつ書き込むわけではない、エラーの際はglobal変数のerrnoに値が挿入される
  * putsは必ず一行挿入する(=末尾に\nが挿入)のでファイルの一番最後のバッファにも\nがつくので注意、これは標準出力専用
* fread,fwrite システムコールのread(),write()とほぼ同じ
* fflush(3) 引数streamがバッファリングしている内容を即時write()させる、文字列を開業せずに端末に出力したいときに使う
* setvbuf(3) stdioに強制的に使わせることができ、バッファリングモードを変更することも可能
* clearerr(3) 第一引数streamのエラーフラグとEOFフラグをクリアする(結構使うらしい)


#### ファイルディスクリプタとFILE型の混在
* 基本的にFILE型にはバッファがあるため、FILE型でできることは全てFILE型でやることが望ましい
* でないと、バッファの関係で入出力がバラバラになってしまうことがあり得る
* 例外的にFILE型で操作できないファイルのパーミッション等を扱う際にのみファイルディスクリプタを使用する

#### なぜfread,fwriteが存在するのか？
* read(), write()はUnixのシステムコールだが、fread(),fwrite()はC言語の標準APIなので他の環境でも動作することが期待できる
* stdioには独自バッファがあり、fread(),fwrite()はその独自バッファの順番通りに書き込み読み込みを行うが、read(),write()は即時書き込みを行う

#### なぜclearerrはよく使うのか？
良い例がtailコマンドで一度書き込みが終わった(=EOFとなった)ファイルを
別のプロセスが内容を追加したらそれをread()で読むことができるように値(EOFフラグ)をクリアしてくれる
※ stdioはread()が一度でもEOFを返すとFILEにEOFフラグをセットして、それ以上read()を呼ばなくなる

```c
// これはどこがダメでなぜコメント以下のコードが良いのか？（P119)
char buf[1024];

fgets(buf, sizeof buf, stdin);
printf(buf);  // printf("%s", buf);
```

```c
// これはどこがダメ？（P127)
// feof()はEOFフラグを取得する関数でstreamの読み込み操作がEOFに到達した時にTRUEとなる
char buf[1024];

while (!feof(stdin)) {
  fgets(buf, 1023, stdin);
  fputs(buf, stdout);
}
```

## 7章headコマンドを作る

* オプションについて
  * 「-」と「--」が、ありロングオプションのパラメータは「--lines 5」や「--lines=5」と書いても構わない
  * APIとして`getopt(3)`がある、これはショートオプションしか認識しない
  * `getOpt_long(3)`はロングオプションを認識し、特別なのはoption構造体の配列を第4引数に受け付けること
    * headの「-n」と「--lines」が同一であるというような対応付け等も行うことができる(P147)
* オプション以外について
  * 「-」は「標準入力から入力せよ」と言う意味で使うことがある
  * 「--」は「ここでオプション解析を停止せよ」と言う意味で使うことがある(マイナスから始まる文字列を渡す時等)


### getOpt(3)について
* getOptを呼び出す毎に1つのオプションの結果を返すので無限ループとなり、終了条件は返り値が-1の時
  * 不正なオプションを受け取った場合は`?`を返却する
  * 第三引数は、そのコマンドで解析したいオプションを文字列(char*)で指定
    * "axf" これらはパラメータを取らないオプション(順序は関係ない)
    * "az:q" zオプションのみパラメータを取り、この場合はzの後に`:`を付与する
    * zオプションがgetOpsで返ってきた場合はグローバル変数「`char *optarg`」に格納されている(他にもある)
  * 実際の「-」と「--」の扱いについてはP145


## 8章grepコマンドを作る

### 文字コードについて
* UTF-8がデファクトだが、対応していない言語も多数あり、全ての文字コードを解決できているわけではない
* 他にはASCII、Shift_JISやEUC_JP等があるが、それぞれの言語に偏りがあり、それを統合解決するためにUTF-8は作られた
* ASCIIで表現できるのはchar型1バイト=8ビット分の256種類しか対応していないから日本語には全く使えない
* 文字コード(符号化文字集合、エンコード)
  * ワイドキャラクタ: 全ての文字に対して同じバイト数を使う手法(UTF-16)
  * マルチバイトキャラクタ: 文字の種類によってバイト数を変える手法(UTF-8,Shift JIS)
  * フローは符号化文字集合と言うなの文字と数値の対応表を使ってエンコード(文字を数値に変換)する
* プログラムが処理できる文字コードや文字を増やすことを「多言語化」
* 地域によって表示させる文字等を変更することを「地域化(localization)」

## 9章Linuxのディレクトリ構造
* /bin システムはブートする際に使用するコマンド置き場
* /usr/bin 一般ユーザ向けのコマンドで、自分でコマンドをインストールするなら/usr/local/binが良い
* /sbin ブート時にも必要な管理者用コマンドを配置し、/usr/sbinには平常時の管理コマンドやサーバプログラムを配置
  * 基本的に/usr以下に配置するものはシステム内のユーザが共有するもののみに留め、逆に個人利用だと/varに配置
* /usr/src LinuxKernel等のプログラムが配置
* /usr/include ヘッダファイル
* /usr/share 複数ユーザで共有できるファイルを配置、ドキュメントのみが良い
* /usr/share/man manのドキュメントが配置
* /usr/local /usrの階層とほぼ同じ構成だが、/usr側はディストリ管理、/usr/localはログインユーザ管理と権限が別れている
  * 自分で使用するプログラムなどはここからディレクトリを掘って、配置するなどが良い
  * /usr/localと同様の使われ方をしているのが/opt
  * pathの設定には気をつけて
* /var/run PIDファイル置き場だが最近は/runのシンボリックリンクが多い(/varから離れたいから)
* /proc プロセスをファイルに落とし込んだディレクトリを管理する、プロセスファイルシステムをマウントしたディレクトリ
  * PID=1のプロセスの中身を見たいなら $ ls /proc/1 で表示できる
  * 最近だとカーネルのリアルタイム情報に関する情報も保存されていたりする(数字のディレクトリ以外のファイル)
* /tmp|/var/tmp /tmpはリブートするとファイルが消失し易いが、/var/tmpはファイルが消えない

### ディレクトリを使い分ける基準
* バックアップは必要か
* ユーザ毎に必要か
* パーミッションは分ける必要があるか
* シェルでグロブの対象にすると便利か？グロブ？


## 10章ファイルシステムにかかわるAPI
* `ls -l`で表示した際の左から二番目の欄はファイル名が指す実態を指す名前の数を表している
  * よって、ファイルのハードリンクを作成した時、リンクカウントがあがり、2と表示される(ハードリンクはシンボリックリンクの逆)
  * rmが削除するのはファイル自体ではなく、ファイル名で、実態を指す名前がなくなった時点で実態が削除される
* シンボリックリンクのreadlink()は文字列最後の'\0'が書き込まれないので注意
* Linuxにおいて「ファイルを移動する」とは、「１つの実態に対する名前を付け替える」ことと大体同じ
* = 「別のハードリンクを作ってもとの名前を消す」ことと同義
* シェルにすると下記のようになるが、決定的な違いはリンクを使うと実態が２つの時が存在するがmvコマンドにはそれがない
* lstat(),stat()はファイルの付帯状況を表示するが、違いはlstatではシンボリックリンクを辿らないこと
* 通常だと32ビットの2GBまでしか扱えなかったが、ラージファイル対応を行うことで64ビットまで対応させることができる

```
$mv a b
// =
$ ln a b
$ rm a
```

## 11章プロセスとハードウェア
### 仮想CPUと仮想メモリを作り出す仕組み
* CPUを増やすには、非常に短い時間単位毎に実行するプロセスを次々に切り替えるだけ
  * このプロセス毎に割り当てられる時間単位をスライスタイムと呼ぶ（スライスタイム自体にも優先度があり、公平ではない）
  * タイムスライスを管理するカーネル機構がスケジューラもしくはディスパッチャという
* メモリは実行しているプログラム毎に割り当てられており、移動はできないので、カーネルとCPUで実際のアドレスを変換し、仮想的にプロセスを変更する
  * 実際のアドレスは物理アドレス
  * プロセスから見えるアドレスが論理アドレス
  * 各プロセスに用意されるメモリ全体のことをアドレス空間と呼ぶ（アドレス空間単位にプロセスが別れているのでプロセスが暴走してもそのプロセスを殺すだけで他のプロセスに影響なく処理を行える）

### 仮想メモリ機構の応用
#### ページング(スワッピング)
HDDやSSDを物理メモリの代わりに使う機構
物理メモリが足りなくなるとカーネルがあまり使われていないページを適当にストレージに記録、論理アドレスとの対応を解除する
プロセスがそのページにアクセスした時は、瞬間にカーネルがプロセスを停止、ストレージから読み込み、論理アドレスに対応付けてプロセスを再開する

上記のような処理をページ単位で行う処理がページングで、プロセス全体を単位にする場合はスワッピングと言う

#### メモリマップトファイル(memory mapped file)
ファイルをメモリとしてアクセスできるようにしてしまう仕組みで、メモリを読むとファイルを読んだことになり、
メモリに書き込むとファイルに書き込んだことになる

#### 共有メモリ
特定の物理メモリを複数プロセスで共有する機構
一つの大きい画像ファイルを複数プロセスで編集したいときは、画像データのプロセスの一つの物理メモリを複数のプロセスの論理アドレスに対応付ける
これを利用したいときはPOSIX共有メモリから考えると良い(ほかはmmap()、System V共有メモリがある)

* アドレス空間の構造
  * テキスト領域 機械語プログラムの領域
  * データ領域 グローバル変数や関数内のスタティック変数のうち初期化済みのもの
  * BSS領域 グローバル変数や関数内のスタティック変数のうち初期化が必要ないものが置かれる
  * ヒープ領域 malloc()が管理する領域
  * スタック領域 関数呼び出しに伴って必要になるデータを置くところで、関数の引数やローカル変数など

### プログラムができるまで
* プリプロセス #includeや#define等を処理し、純粋なC言語に変換
* コンパイル アセンブル言語に変換
* アセンブル オブジェクトファイルに変換
* リンク オブジェクトファイルから実行ファイルやライブラリ(*.a, *.so)を生成し、フォーマットはELFフォーマットが多い
* スタティックリンク ほとんど使われないリンカ
* ダイナミックリンク
  * ほとんどで使われているリンカ、ビルド時には最終的なコード結合までは行わず、チェックするだけ
  * ファイル名は通常「*.so」
  * 実行時にリンクローダが実行ファイルと共有ライブラリをメモリに展開し、メモリの上で最終的にコードを結合する
  * fileコマンドを使うとバイナリファイルの内容を表示してくれる
  * lddコマンドはダイナミックリンクされた共有ライブラリを表示してくれる
  * gccコマンドもダイナミックリンク
* ダイナミックロード
  * RubyやPythonなどがこの仕組
  * ダイナミックロードはDynamic Linkでやっていた必要な関数名のチェックすら行わず、全て実行時に行っている
  * プログラムを実行中にメモリ空間にいるリンカを呼び出す

## 12章プロセスに関わるAPI
### ゾンビプロセス
もしも親プロセスがいつまでたってもwait()を呼ばないまま動作し続けたらどうなるか…
カーネルからすると親プロセスがwait()を呼ぶかどうかは予測できない
カーネルは親プロセスが終了するか、またはwait()を呼ぶまでは子プロセスのステータスコードを保存しておかなければならない
この状態の子プロセスのことをゾンビプロセスと呼ぶ
ゾンビになったプロセスはpsコマンドの出力に「zombie」や「defunct」と表示される

回避するには…
* fork()したらwait()する
  * 一番正当
* sigaction()
  * 「自分はwait()をしない」とカーネルに知らせること(後の章で
* ダブルfork()
  * 途中に余分なfork()を挟み、孫プロセスを作成する
  * wait()する権利があるのは親子関係の親プロセスのみなので、子プロセスを即時終了させると、孫にとっての親プロセスがいなくなってしまう
  * この場合はwait()するプロセスが存在しなくなるため、カーネルも孫プロセスをゾンビにせず、終了したらすぐに始末してくれる

### プロセスの関係
プロセスは親子関係が基本なのでLinuxシステムはプロセスツリーができる(pstreeコマンドで確認することが可能)
一番の親はsystemdでブート時にカーネルが直接起動する

### プロセスグループとセッション
プロセスグループはシェルのためにあり、パイプを使ったコマンドを起動した時に途中でプロセスを中断した時、
コマンド全体を構成する全プロセスを殺す仕組みがプロセスグループで「パイプでつながれたプロセス群」を
一つのプロセスグループとしてまとめ、そのグループのプロセスにシグナルをまとめて遅れるようにすれば良い

セッションはユーザのログインからログアウトまでの流れを管理するための概念
ユーザが同じ端末から起動したプロセスを一つにまとめる働きをする
よって、一つのセッションは複数のプロセスグループをまとめるような形になる

### パイプ(プロセスからプロセスに繋がったストリーム)
ファイル同士のパイプは読み書き両用だったが、パイプは基本的に一方向で、読みか書くかどちらかしかできない

#### pipeシステムコール
pipe(int fds[2])は自プロセスにつながったストリームを作成し、その両端のファイルディスクリプタ2つをfdsに書き込む
fds[0]が読み込み専用、fds[1]は書き込み専用

大事なのは**fork()プロセスを複製するときにストリームも全て複製するということ**
pipe()でパイプを作ったあとにfork()すると、親子プロセス全てがパイプで繋がっている
この時、親が読み込み側をclose()し、子が書き込み側をclose()すると、親の書き込みが子の読み込みがつながり、パイプが親子でつながる
もしも、任意のファイルディスクリプタにパイプでつなぎたい場合はdup()かdup2()を使用する

## 13章シグナルにかかわるAPI
* シグナルはいつ来るかいくつ来るかわからないから本質的に厄介で、一つのシグナルを処理中に他のシグナルが来ても保留になるようにシステム側で対処している
* = 「シグナルをブロックする」と言う

### Ctrl+Cについて
まず、ユーザがCtrl+Cを入力すると、カーネルの端末ドライバが検出する
普通にシェルを使っている場合はcookedモードになっているので特殊な働きをするキーが存在する(Ctrl+Cもそう)
sttyコマンドを-aオプション付きで実行するとよくわかる

```
$ stty -a
// 一部抜粋(intrはinterrupt)
intr = ^C;
```

シェルがパイプでつながれたプロセス群を起動するたびに「この端末では今、このプロセスが動いています」と
端末に教えているので「端末で動作中のプロセス」がわかり、SIGINTを送信することができる
そうするとプロセスグループが終了する（パイプ全体が終了する）

フローとしてパイプつなぎのコマンドの生まれから終わりまで辿ると、、
1. シェルがパイプを構成するプロセスをfork()
1. シェルがパイプのプロセスグループIDをtcsetpgrp()で端末に通知
1. forkされた各プロセスがそれぞれのコマンドをexec
1. Ctrl+Cを実行
1. カーネル内の端末ドライバがそれをSIGINTに変換、動作中のプロセスグループに発信
1. プロセスグループがデフォルトの動作に従って終了

## 14章プロセスの環境
プロセスの4つの属性

* カレントディレクトリ
  * 今いるディレクトリ(getcwd()で取得)、自プロセスのカレントディレクトリを移動するにはchdir()を使用
  * 他のプロセスのカレントディレクトリを変更はできない、知るにはシンボリックリンク「/proc/process-id/cwd」で取得可能
* 環境変数
  * プロセス間を通じて伝搬するグローバル変数のようなもので、Cプログラムではグローバル変数environ(char**=２次元配列)を経由する
  * getenv()は引数の値を検索して返す、putenv()は引数のstringを環境変数に設定する(形式は「名前=値」でなければならない)
* クレデンシャル
  * コマンドを実行するユーザに関係なく特定のユーザの権限で実行したい時(例えば、特定のユーザのみに書き込みを許可したい等)にファイルパーミッションのset-uidビットを使用する
  * set-uidを使用することで起動したユーザにかかわらず、プログラムファイルのオーナーの権限で起動される
  * set-uidの確認はls -l等でパーミッションが「s」になっていればset-uidビットが立っていると判定できる
  * 起動したユーザのIDを実ユーザID(real user ID)
  * set-uidプログラムのオーナーのIDを実効ユーザID(effective user ID)
  * set-uidはユーザだけでなく、当然グループ単位にでも指定することが可能
  * クレデンシャルは「ユーザ・グループ」とは別に関わりが無く紐付ける必要はない
* 使用リソース
  * カーネルは、CPUやメモリなどの情報を各プロセス毎にリソース量を記録している
  * システム時間: そのプロセスのためにカーネルが働いた時間(システムコールを実効した時間)
  * ユーザ時間: システム時間以外の、プロセスが完全に自分で消費した時間

### ログイン

下記が簡単なログインまでの流れ
1. systemd(カーネルが直接起動する唯一のプログラム)またはinitが端末の数だけgettyコマンドを起動
1. gettyコマンドは端末からユーザ名が入力されるのを待ち、loginコマンドを起動
1. loginコマンドがユーザを認証
1. ログインシェルを起動

* systemdはログインを待ち受けるgettyというプログラムを起動する役割も持っている
* gettyが何をするかというと、端末をopen(),read()して、ユーザがユーザ名をタイプするまで待機する
  * このときに様々な詳細な設定を行うためgettyという独立したプログラムになっている
* user名が入力されたら、gettyはdup()を使ってファイルディスクリプタの0,1,2番に端末をつなぎ、新しいプログラムloginをexecする
* 認証にはユーザデータベースを使用し、典型的には/etc/passwdを使用するが、ディストリビューションやLDAP等により、一概には言えない
* 最近だと、/etc/passwdに直接書かず、/etc/shadowに分離するシャドウパスワードが導入されている
* ログインには様々なカスタマイズができるように必要があり、それらは/etc/login.defsで設定をするようになっていたが、loginプログラム以外のログインが必要なssh/telnet等でも同様の設定を再度設定し直すのが面倒な為、最近はPAMと言うモジュールの「ユーザを認証する」というAPIを呼び出し認証を行っている
* PAMは共有ライブラリで様々な要望に対応ｓるうためにダイナミックロードを使ってライブラリを分割している
* ログインシェルはコマンド名の頭に「-」をつけて起動するが、動作通常のシェルとは少し異なる
  * ```execl("/bin/sh", "-sh", ...);```
  * ログインしたかどうかはwやlastなどのコマンドを使用して確認することが可能

## 15章ネットワークプログラミングの基礎
* 今まではマシン内でストリームを使っていたが、ネットワーク越しになってもそれは変わらない
* ネットワーク越しの場合では、ネットワークに繋がるコンピュータには通信を待ち受けているプロセスが存在し、それがストリームの接続先になる
* 上記のように接続を待っていて何かをしてくれるプロセスのことをサーバプロセスと言う
* 逆にサーバに接続して何かをしてもらうプロセスのことをクライアントプロセスと言う
* IPの世界ではパケットが存在しており、バケツリレーのようにルーターからルーターへ受信者まで送信を繰り返す
* TCPはパケットをストリームに見えるように仕組み化している
  * ストリームとはバイト列なので、送りたいリクエストを細かく切断し、一つ一つをパケットとして送信する
  * このときに、連番を降っておいて、順番がわかりやすいようにする
  * これらが全て揃ったらストリームとしてマシン上で再現する
* UDPはストリームは考えず、パケットをそのまま活かした機構で、パケットが届く順番は保証されないし、パケロスすることもあるが、速度が早く、処理も簡単という利点がある
* ホスト名をIPアドレスに変換する機構がリゾルバ(DNSや/etc/hostsが関連する)
* 一般に、PCの世界では名前からその実態を得ることを「解決」というので、リゾルバと呼ぶ

### ソケットAPI
* Linuxでネットワークに使うのがソケット(何かをつなぐ端点)
* サーバ・クライアント側の両方を扱える
* IPv4もIPv6も一つのAPIで対処できる利点がある
* クライアント側のソケットAPI
  * socket(2): ソケットを作成するシステムコール
  * connect(2): 接続する相手を指定し、ストリームをつなぐ
* サーバ側
  * socket(2)
  * bind(2): 接続を待つアドレスをソケットに割り当てる
  * listen(2): ソケットを指定して、接続を待っているソケットであることをカーネルに伝える、また最大同時接続コネクション数を指定
  * accept(2): ソケットにクライアントが接続してくるのを待ち、接続が完了したら、接続済みのストリームのファイルディスクリプタを返す
* 上記のシステムコールは全てIPアドレスを指定しなければならず、ホストから名前解決しなければならない
  * getaddrinfo()を使うと候補郡を配列形式で取得することができる

## 17章HTTPサーバを本格化する

### preforkサーバ
今回は元締め=親プロセスがaccept()してその後にfork()したが、その逆の最初に何回かfork()しておいてから、
子プロセスが自分でaccept()する方法もあり、このサーバをプロフォークサーバと言う
今回のサーバを並行サーバという

一般的にはpreforkサーバのほうがパフォーマンスが上がるが、予め用意するプロセスの数の調整や
プロセス処理がややこしく、実装も運用も難易度が上がる
伝統的なUNIXのサーバはほとんどが並行サーバで実装されているので忙しいサーバでなければpreforkは難しい

### マルチスレッド
高速化の手法の一つがスレッド
スレッドはプロセスに似ていて複数の作業を同時に行えるようにする仕組み
スレッドとプロセスの大きな違いはメモリ空間が別れていないこと
つまり、プロセス間通信機構を使うことなく調節データを共有できる
複数のスレッドを使ったプログラムをマルチスレッドプログラムという
スレッドを使うとデータのやり取りが簡単になってとても便利なのだが、
その一方で、*データを共有してしまうこと*による問題も起こる

例えば、グローバル変数やスタティック変数を操作する場合
次のように変数の値を３増やす簡単な操作をするとする

```
int var = 0;

static void
increment(void)
{
  var += 3;
}
```

この命令をコンパイルすると下記のようになり、マルチスレッドだと命令が混ざってしまう可能性がある
例えば２の手順が同時に発生して、スレッドAの結果とスレッドBの結果が同時に発生して
結果変数varは６になっているはずなのに、一回分の３と言う結果になったりする

1. 変数varの値をメモリから取り出す
2. それに3を足す
3. 結果をメモリ上の変数varに書き込む

### chroot()で安全性を高める
chroot()を使うとプロセスから見えるファイルシステム自体を狭くしてしまう手法
ディレクトリpathの外のファイルは絶対に見えないことをカーネルが保証してくれる

ただ、システム管理コストが増える
chroot()はファイルシステムを新しく作るようなものだから、必要なファイルを内外の両方に配置し、同期を取る必要がある
e.g.)/etc/passwd,/dev/null

chroot()はスーパーユーザでないと実行できないが、HTTPサーバをスーパーユーザのまま実行するのは危険
そのため、最初はスーパーユーザ権限で起動し、chroot()したあとで自ら権限を放棄することで対応する
なので、setuid()をする前にchroot()を実行しなければならない


##### 蛇足
* mountコマンドを使うと、現在使っているシステムでどのようなファイルシステムが使われているのか調べられる
* ext4はLinuxで現在最も一般的なファイルシステム(`$ mount -t ext4`)
* システムコールの呼び出しはかなり遅い
* アンバッファードモードのstdioストリームは書き込むと一切バッファリングすることなく即座にwrite()される
  * 標準エラー出力に対応するストリームstderrは最初からアンバッファードモードになっている(デバッグやエラーメッセージを即座に書き出したいから
* getsは本質的にバッファーオーバーフローの欠陥があり、これを使ったワームがたくさんある
* libcには沢山の問題をはらんでいる関数が存在するので注意
* scanfもgetsと同様の危険があるため非推奨
* straceコマンドで実際のプログラムがどのようなシステムコールを行っているのかわかる
  * 普通に使うとわかりづらいのでopen,read,write,closeだけに絞ることもできる(strace -e trace=open,read,write,close ./cat data > /dev/null)みたいな感じ
* Books?の正規表現は「Book」と「Books」となる
* iconvは文字コードの相互変換ライブラリで例えばEUC_JPとUTF-8の変換ができる、Linuxだとlibcに入っている
* ネットワークファイルシステム（NFS、SAMBA）P183
* シェルのバッククォートの使われ方
  * cat /var/run/xxx.pid
  -> 76
  * kill -HUP `cat /var/run/xxx.pid`
  -> kill -HUP 76と同義
* devfsとudevの違いはカーネルの一部としてのプログラムか、カーネル外のプログラムかの違い(P186)
* https://github.com/todokr/simple-http-server はその他のプログラミング言語での実装する際に役立つかも
