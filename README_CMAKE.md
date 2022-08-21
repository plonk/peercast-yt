# Linuxでのビルド


## 手順

```shell
git clone ttps://github.com/plonk1/peercast-yt
cd peercast-yt

# Build環境の作成
cmake -S . -B build

# build
cmake --build ./build

# テストケース毎に実行(遅い)
cmake --build build --target test

# テストを一括で実行(早い)
# カレントディレクトリに.tmpファイルが作成されるので注意
pushd build && test-all && popd

# clean
cmake --build ./build --target clean
```


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
```shell
sudo apt install ninja-build
# cmake -S . -B build -G "Unix Makefiles"
cmake -S . -B build -G "Ninja"
cmake --build ./build
```

# TUIでCONFIGする場合
```
sudo apt install cmake-curses-gui
cmake -S . -B build
cmake build
```