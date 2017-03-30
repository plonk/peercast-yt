# PeerCast YT

以下の機能を追加した PeerCast v0.1218。VP の機能も一部取り込んでいます。

* PeerCastStation 互換の JSON RPC インターフェイス。
* FLV配信 (HTTPソースのプル)
* HTML UI をメッセージカタログ化。各国語版で機能に違いがないようにした。
* YPブラウザ。YP4G の index.txt を取得してチャンネルリストを表示します。

# 使用ライブラリ

* [JSON for Modern C++](https://github.com/nlohmann/json)

# Linuxでのビルド

## 通常の C++ 開発環境の他に必要なもの

HTML の生成に ruby を使っているので、ruby が必要です。

## 手順

`ui/linux` ディレクトリで make して、成功すると `peercast-linux.tar`
ができます。適当なディレクトリに展開して、そのディレクトリで
`./peercast` を実行してください。設定ファイル `peercast.ini` はカレン
トディレクトリに作られます。

make した後、`ui/linux` ディレクトリで `./peercast` を実行することもで
きます。
