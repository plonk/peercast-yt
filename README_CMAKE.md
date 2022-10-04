# Linuxでのビルド


## 手順

```shell
git clone ttps://github.com/plonk1/peercast-yt
cd peercast-yt

# Build環境の作成(明示しない場合は不定)
cmake -S . -B build   -DCMAKE_BUILD_TYPE=Release
# cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
# cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
# cmake -S . -B build -DCMAKE_BUILD_TYPE=MinSizeRel

# build
cmake --build ./build

# clean
cmake --build ./build --target clean
```


# テストの実行
```shell
cd peercast-yt

# テストケース毎に実行(遅い)
cmake --build build --target test

# テストを一括で実行(早い)
# カレントディレクトリに.tmpファイルが作成されるので注意
pushd build && ./test-all && popd

# CTestコマンドで実行(cmake ～ --target testと同じハズ)
pushd build && ctest && popd
```

# 成果物の作成
```shell
# tar.gzボールにまとめる
cd build

cpack

ls -la peercast*.tar.gz
```


# Ubuntu(22.04)でのビルドに必要なライブラリなど
```shell
sudo apt install build-essential \
                  cmake \
                  pkg-config \
                  libgtest-dev \
                  librtmp-dev
```

# Windows(MinGW-w64)でのビルドに必要なライブラリなど
```shell
pacman -S base-devel \
          mingw-w64-x86_64-toolchain \
          mingw-w64-x86_64-cmake \
          mingw-w64-x86_64-pkg-config \
          git \
          mingw-w64-x86_64-gtest \
          mingw-w64-x86_64-openssl \
          ruby
```

# Ninja で高速ビルド on Ubuntu(22.04)
FIXME: 現在、Windows(MinGW-w64環境）においてNinjaによるビルドはできない
       ようなできるようなよくわからない・・・

```shell
sudo apt install ninja-build

# Ninjaを使う場合
cmake -S . -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release

# 明示的にmakeを使う場合
cmake -S . -B build -G "Unix Makefiles"-DCMAKE_BUILD_TYPE=Release

cmake --build ./build
```

# TUIでCONFIGする場合
```
sudo apt install cmake-curses-gui

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

# TUIでオプションを変更できる
ccmake build

cmake build
```