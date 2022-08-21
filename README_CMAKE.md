# Linuxでのビルド

## 必要なもの

コンパイラは、GCC 4.9 以降あるいは Clang 3.4 以降などの C++11 に準拠し
たものを使ってください。

また、ビルド時に HTML ファイルの生成のために Ruby を必要とします。また、
実行時(CGIスクリプト)に Python3 が必要です。

## 手順

```shell
git clone ttps://github.com/plonk1/peercast-yt
cd peercast-yt

# フォルダ内で作業
# cmake -S . -B build # 分かってる人はこちらで
cmake --build ./build


# フォルダ外で作業
cd ../
cmake -S peercast-yt -B build
cmake --build ./build

```

<!-- cmake --install <ビルドツリー> --prefix <インストールprefix>

困ったらこれTUI(ccmake)もいいぞ
sudo apt install cmake-curses-gui -->

# テストの実行
```shell
cd peercast-yt
cmake -S . -B build
cmake --build build

cd build
ctest
#~~~~ 注意
```


# Ubuntu(22.04)でのビルドに必要なライブラリなど
```shell
sudo apt install build-essential \
                  cmake \
                  libgtest-dev
```

# Mingw-w64でのビルドに必要なライブラリなど
```shell
pacman -S base-devel \
          mingw-w64-x86_64-toolchain \
          mingw-w64-x86_64-cmake \
          git \
          mingw-w64-x86_64-gtest \
          mingw-w64-x86_64-openssl \
          ruby
```

# Ninja で高速ビルド on Ubuntu(22.04)
```shell
sudo apt install ninja-build
# cmake -S . -B build -G "Unix Makefiles"
cmake -S . -B build -G "Ninja"
cmake --build ./build
```

# 実行

peercast コマンドを起動したあと、ウェブブラウザで `http://localhost:7144/`
を開くと操作できます。なお、設定ファイル `peercast.ini` は `~/.config/peercast/`
ディレクトリに作られます。


|      | GNUInstallDirs           | CMake組み込みデフォルト |
| ---- | ------------------------ | ----------------------- |
| BIN  | ${CMAKE_INSTALL_BINDIR}  | bin                     |
| DATA | ${CMAKE_INSTALL_DATADIR} | <DATAROOT dir>          |
| DOC  | ${CMAKE_INSTALL_DOCDIR}  | <DATAROOT dir>/doc      |

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
