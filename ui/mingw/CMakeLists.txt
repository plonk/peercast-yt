# CMakeのバージョンを設定
cmake_minimum_required(VERSION 3.18)

# プロジェクト名と使用する言語を設定
project(peercast-yt-cli CXX)

# コンパイル・リンクするソースコードを変数に格納
file(GLOB_RECURSE MYPECA_SRCS
  "${PROJECT_SOURCE_DIR}/../../core/common/*.cpp"
  "${PROJECT_SOURCE_DIR}/../../core/win32/*.cpp"
)

# 絶対パスのほうがいいとかなんとか
# cmake_policy(SET CMP0076 NEW) # CMakeが自動的に相対パスを絶対パスへ変換する
# target_sources(mylib
#   PRIVATE
#     file1.cpp
#     file2.cpp
# )

# 実行ファイルをmain.cppから作成
add_executable(peercast-yt-cli main.cpp)
target_sources(peercast-yt-cli
  PRIVATE ${MYPECA_SRCS}
)
  # ${PROJECT_SOURCE_DIR}/../../core/win32/*.cpp


# Includeディレクトリを登録
target_include_directories(peercast-yt-cli
  PUBLIC ${PROJECT_SOURCE_DIR}/../../core/common
  )
target_include_directories(peercast-yt-cli
  PUBLIC ${PROJECT_SOURCE_DIR}/../../core
  )

# 面倒だしstatic-linkしちまおうぜ
set(CMAKE_EXE_LINKER_FLAGS " -static")
target_link_options(peercast-yt-cli PRIVATE -static-libgcc -static-libstdc++)

# 使用するライブラリの指定
if(WIN32)
  # Static
  target_link_libraries(peercast-yt-cli winpthread)
  # Dynamic
  # target_link_libraries(peercast-yt-cli winpthread) # dynamic
  target_link_libraries(peercast-yt-cli wsock32 ws2_32)
  target_link_libraries(peercast-yt-cli Shlwapi)
endif()

# OpenSSLライブラリを探してリンク
set(OPENSSL_USE_STATIC_LIBS TRUE)
find_package(OpenSSL REQUIRED)
# target_link_libraries(peercast-yt-cli PRIVATE OpenSSL::SSL)
target_link_libraries(peercast-yt-cli OpenSSL::SSL)
