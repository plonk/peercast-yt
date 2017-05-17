# PeerCast YT

[![Build Status](https://travis-ci.org/plonk/peercast-yt.svg?branch=master)](https://travis-ci.org/plonk/peercast-yt)

VPS等の Linux で動かすのに向いている PeerCast です。

* Ajax による画面更新。
* PeerCastStation 互換の JSON RPC インターフェイス。(epcyp、ginger などで使えます)
* FLV、MKV、WebMフォーマットの配信に対応。
* HTTP Push での配信に対応。ffmpeg と直接接続できます。
* WME、Expression Encoder、KotoEncoder からプッシュ配信できます。
* HTML UI をメッセージカタログ化。各国語版で機能に違いがないようにしました。
* YPブラウザ内蔵。YP4G 形式の index.txt を取得してチャンネルリストを表示します。
* 公開ディレクトリ機能。チャンネルリストやストリームをWebに公開できます。
* [継続パケット機能](docs/continuation-packets.md)。キーフレームからの再生ができます。

# 使用ライブラリ

* [JSON for Modern C++](https://github.com/nlohmann/json)

# Linuxでのビルド

## 必要なもの

GCC 4.9 以降あるいは Clang 3.4 以降などの C++11 に準拠したコンパイラを
使ってください。

また、HTML の生成に ruby を使っているので、ruby が必要です。

## 手順

`ui/linux` ディレクトリに入って `make` してください。すると
`peercast-yt-linux-x86_64.tar.gz` (CPUアーキテクチャによって名前が変わ
ります) ができます。

※ make した後、`ui/linux` ディレクトリの `peercast` を実行することも
できます。

# 実行

`peercast-yt-linux-x86_64.tar.gz` を適当な場所に展開して、ディレクトリ
内の `peercast` を実行してください。

設定ファイル `peercast.ini` は `peercast` と同じディレクトリに作られま
す。
