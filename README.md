# PeerCast YT

[![Build Status](https://travis-ci.org/plonk/peercast-yt.svg?branch=master)](https://travis-ci.org/plonk/peercast-yt)

VPS等の Linux で動かすのに向いている PeerCast です。PeerCast Gateway
の縁の下で動いています。

* PeerCastStation と部分的に互換の JSON RPC インターフェイス。
* FLV配信 (HTTPソースのプル)
* HTML UI をメッセージカタログ化。各国語版で機能に違いがないようにしました。
* YPブラウザ。YP4G の index.txt を取得してチャンネルリストを表示します。
* ネットワーク出力をバッファリングしているので、多少 IO 負荷が軽くなっているはずです。

# 使用ライブラリ

* [JSON for Modern C++](https://github.com/nlohmann/json)

# Linuxでのビルド

## コンパイラ

JSON ライブラリが新しめのコンパイラを必要とします。GCC 4.9 以降あるい
は Clang 3.4 以降あたり。

## その他

HTML の生成に ruby を使っているので、ruby が必要です。

## 手順

`make -C ui/linux` とすると `ui/linux/peercast-linux.tar` ができます。
適当なディレクトリに展開して、ディレクトリ内の `peercast` を実行してく
ださい。設定ファイル `peercast.ini` は `peercast` と同じディレクトリに
作られます。

make した後、`ui/linux` ディレクトリの `peercast` を実行することもでき
ます。
