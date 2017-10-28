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

## Linuxとユーザ

##### 蛇足
* mountコマンドを使うと、現在使っているシステムでどのようなファイルシステムが使われているのか調べられる
* ext4はLinuxで現在最も一般的なファイルシステム(`$ mount -t ext4`)
