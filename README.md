# PeerCast YT

[![Build Status](https://travis-ci.org/plonk/peercast-yt.svg?branch=master)](https://travis-ci.org/plonk/peercast-yt)

VPS等の Linux で動かすのに向いている PeerCast です。

* PeerCastStation と(部分的に)互換の JSON RPC インターフェイス。(epcyp、ginger などで使えます)
* FLVフォーマット、MKVフォーマットに対応。
* HTTP Push ストリームでの配信に対応。ffmpeg と直接接続できます。
* HTML UI をメッセージカタログ化。各国語版で機能に違いがないようにしました。
* YPブラウザ。YP4G 形式の index.txt を取得してチャンネルリストを表示します。
* ネットワーク出力をバッファリングしているので、多少 IO 負荷が軽くなっているはずです。
* Ajax による画面更新。

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
