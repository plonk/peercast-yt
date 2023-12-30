# PeerCast YT

PeerCast のフォークです。

## ブラウザインターフェイス

ブラウザインターフェイスは、YPブラウザ、動画プレーヤー、したらば掲示板
ビューワを実装しており、ユーザーはウェブブラウザさえあれば ローカル/リ
モートを問わず PeerCast が視聴できます。

## 多種のエンコーダーに対応

* RTMP に対応しており、OBS などのエンコーダーで配信できます。
  →[RTMPプロトコル対応エンコーダーでの配信のやり方](https://github.com/plonk/peercast-yt/wiki/RTMP%E3%83%97%E3%83%AD%E3%83%88%E3%82%B3%E3%83%AB%E5%AF%BE%E5%BF%9C%E3%82%A8%E3%83%B3%E3%82%B3%E3%83%BC%E3%83%80%E3%83%BC%E3%81%A7%E3%81%AE%E9%85%8D%E4%BF%A1%E3%81%AE%E3%82%84%E3%82%8A%E6%96%B9)
* HTTP Push に対応しており、ffmpeg で配信できます。
  →[HTTP Push 配信のやり方](https://github.com/plonk/peercast-yt/wiki/HTTP-Push-%E9%85%8D%E4%BF%A1%E3%81%AE%E3%82%84%E3%82%8A%E6%96%B9)
* Windows Media HTTP Push 配信プロトコルに対応しており、WME、
  Expression Encoder、KotoEncoder からプッシュ配信できます。(エンコー
  ダーを動かす PC はポートが開いている必要がありません。)
  →[Windows Media HTTP Push 配信のやり方](https://github.com/plonk/peercast-yt/wiki/Windows-Media-HTTP-Push-%E9%85%8D%E4%BF%A1%E3%81%AE%E3%82%84%E3%82%8A%E6%96%B9)

## 多種の動画フォーマットに対応

* 従来のフォーマットに加え、FLV、MKV、WebM の配信に対応しています。

## その他

* [継続パケット機能](docs/continuation-packets.md)により、キーフレーム
  からの再生ができます。
* PeerCastStation 互換の JSON RPC インターフェイス。
  →[JSON RPC API](https://github.com/plonk/peercast-yt/wiki/JSON-RPC-API)
  ([epcyp](https://github.com/mrhorin/epcyp)、
  [ginger](https://github.com/plonk/ginger/) などで使えます)
* <del>公開ディレクトリ機能。チャンネルリストやストリームをWebに公開できます。</del>
* HTML UI をメッセージカタログ化。各国語版で機能に違いがないようにしました。
* Ajax による画面更新。

# バイナリのダウンロード

https://github.com/plonk/peercast-yt/releases/ に各プラットフォーム用
のバイナリがあります。

また、Docker HubにDockerイメージがあります。

<dl>
    <dt>AMD64版</dt>
    <dd>https://hub.docker.com/r/plonk/peercast-yt</dd>
    <dt>ARM版 (Raspbian用)</dt>
    <dd>https://hub.docker.com/r/plonk/peercast-yt-arm</dd>
</dl>

# Linuxでのビルド

## 必要なもの

コンパイラは、GCC 4.9 以降あるいは Clang 3.4 以降などの C++11 に準拠し
たものを使ってください。

また、ビルド時に HTML ファイルの生成のために Ruby を必要とします。また、
実行時(CGIスクリプト)に Python3 が必要です。

## 手順

`ui/linux` ディレクトリに入って `make` したあと、`sudo make install` で
/usr/local/ 以下にインストールできます。

また、`ui/linux/tests` ディレクトリで `make` すると一連の単体テストを実行する
`test-all` コマンドが作成されます。

# 実行

peercast コマンドを起動したあと、ウェブブラウザで `http://localhost:7144/`
を開くと操作できます。なお、設定ファイル `peercast.ini` は `~/.config/peercast/`
ディレクトリに作られます。

# MSYS2でのビルド

MSYS2 を使って Windows (MinGW)版をビルドできます。`ui/mingui` ディレク
トリで`make` してください。

また、`ui/mingui/tests` ディレクトリで `make` すると一連の単体テストを
実行するファイル `test-all.exe` が作成されます。

掲示板ビューワなどの機能を動かすには `peercast.exe` が存在するディレク
トリから見て `python\python.exe` に Tiny Python が配置されている必要
があります。

## ヒント

MSYS2 には MSYS、MinGW 32ビット、MinGW 64ビットの3つの開発環境があり、
それぞれの環境で異なるコンパイラが使用されます。

32ビット版の`peercast.exe`を作成したい場合は `MinGW 32-bit` のターミナ
ルから、64ビット版の場合は `MinGW 64-bit` のターミナルから作業します。
`MSYS` ターミナルからは正常に動く peercast バイナリが作成できません。

開発ツールチェイン、Ruby、Google Testなどの必要なソフトウェアは
`pacman`経由でインストールします。

パッケージ名は、64ビット版のパッケージには `mingw-w64-x86_64-` のプレ
フィックスが、32ビット版は `mingw-w64-i686-` のプレフィックスが付いて
います。

なお、パッケージのダウンロードに10秒以上かかるとエラーになる不具合に遭
遇した場合は `pacman` に `--disable-download-timeout` 引数を追加します。

# RTMP fetchサポート

RTMP をサポートするストリーミングサーバーからストリームを取得して配信
チャンネルを作成したい場合、PeerCast YT が RTMP fetch サポート付きでビ
ルドされている必要があります。

サポートをオンにするには `Makefile` の先頭で `WITH_RTMP` 変数の値を
`yes` にしてビルドします。`librtmp` をリンクする必要があるので、インス
トールしておいてください。

MinGW の場合は rtmpdump-git パッケージを以下のように（64ビットの場合)インストール
してください。

```
pacman -S mingw-w64-x86_64-rtmpdump-git
```
