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
