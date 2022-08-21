# CMakeのバージョンを設定
cmake_minimum_required(VERSION 3.18)


# プロジェクト名と使用する言語を設定
project(peercast-yt CXX)
set(CMAKE_CXX_STANDARD, 11) # 標準の言語仕様をC++11にする

# CPack用に使われる
# 現在のプラットフォーム用のランタイムライブラリを含めるために必要
if(NOT WIN32)
  include(InstallRequiredSystemLibraries)
endif()

if(NOT WIN32)
  include(GNUInstallDirs)
endif()
# CMAKE_INSTALL_DIR
# CMAKE_INSTALL_DOCDIR
# set(CMAKE_INSTALL_DATADIR)
# CMAKE_INSTALL_DATADIR

################################################################################
# Projectの依存関係
# - TOP(peercast-yt)
#   - libpeercast (/core以下)
#     - linux(cli binary, zip packaging)
#       - AppImage(packaging)
#     - windows(cli/gui binary, zip packaging)
#   - html
#   - rtmp-server
#
################################################################################

################################################################################
# やり残したこと
# FIXME: generate-[html|public]で生成される一時フォルダが/ui/{html|public}のまま
#
################################################################################

################################################################################
# Project: libpeercast
################################################################################
# 実行ファイルをmain.cppから作成
add_library(core STATIC)
# SET_TARGET_PROPERTIES(lib-core PROPERTIES OUTPUT_NAME libpeercast)

# set_property(TARGET libpeercast PROPERTY CXX_STANDARD 11)
target_compile_definitions(core PRIVATE _UNIX)

# コンパイル・リンクするソースコードを変数に格納
if(WIN32)
  file(GLOB_RECURSE MY_LIBPEERCAST_SRCS
    "${PROJECT_SOURCE_DIR}/core/common/*.cpp"
    "${PROJECT_SOURCE_DIR}/core/win32/*.cpp"
  )
else()
  file(GLOB_RECURSE MY_LIBPEERCAST_SRCS
    "${PROJECT_SOURCE_DIR}/core/common/*.cpp"
    "${PROJECT_SOURCE_DIR}/core/unix/*.cpp"
  )
endif()
target_sources(core
  PRIVATE ${MY_LIBPEERCAST_SRCS}
)

# Includeディレクトリを登録
# PUBLICにすることでリンクするターゲットで自動的に読み込まれる
target_include_directories(core
  PUBLIC ${PROJECT_SOURCE_DIR}/core/common
  )
target_include_directories(core
  PUBLIC ${PROJECT_SOURCE_DIR}/core
  )

################################################################################
# Project: peercast(linux)
################################################################################
add_executable(linux-bin ui/linux/main.cpp)
SET_TARGET_PROPERTIES(linux-bin PROPERTIES OUTPUT_NAME peercast)
target_link_libraries(linux-bin core)

# OpenSSLライブラリを探してリンク
set(OPENSSL_USE_STATIC_LIBS TRUE)
find_package(OpenSSL REQUIRED)
# target_link_libraries(peercast-yt-cli PRIVATE OpenSSL::SSL)
target_link_libraries(linux-bin OpenSSL::SSL)

################################################################################
# Project: html
################################################################################
# CATALOGS := $(wildcard catalogs/*.json)
# TEMPLATES := $(wildcard html-master/*.* html-master/images/*.* html-master/js/*.* Templates/*.*)
# PUBLIC_TEMPLATES := $(wildcard public-master/*.* public-master/images/*.* public-master/js/*.* Templates/*.*)
file(GLOB_RECURSE MY_HTML_CATALOGS
    "${PROJECT_SOURCE_DIR}/ui/catalogs/*.json"
)
file(GLOB_RECURSE MY_HTML_TEMPLATES
    "${PROJECT_SOURCE_DIR}/ui/html-master/*.*"
)
file(GLOB_RECURSE MY_PUBLIC_TEMPLATES
    "${PROJECT_SOURCE_DIR}/ui/public-master/*.*"
)

# make相当
# all: html
# html: html.tar
add_custom_target(html ALL
    DEPENDS html.tar)
# ファイル更新を検出するために add_custom_commandを利用する
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/html.tar
  COMMAND ruby --disable-gems "${PROJECT_SOURCE_DIR}/ui/generate-html"
  COMMAND tar cf "${CMAKE_CURRENT_BINARY_DIR}/html.tar" html
  DEPENDS ${MY_HTML_CATALOGS} ${MY_HTML_TEMPLATES}
  WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/ui"
)
# clean時に消されるようにする
set_property(DIRECTORY PROPERTY ADDITIONAL_MAKE_CLEAN_FILES "${PROJECT_SOURCE_DIR}/ui/html")

add_custom_target(public ALL
    DEPENDS public.tar)
add_custom_command(
  # OUTPUT public.tar
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/public.tar
  COMMAND ruby --disable-gems "${PROJECT_SOURCE_DIR}/ui/generate-public"
  COMMAND tar cf "${CMAKE_CURRENT_BINARY_DIR}/public.tar" public
  DEPENDS ${MY_HTML_CATALOGS} ${MY_PUBLIC_TEMPLATES}
  WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/ui"
)
set_property(DIRECTORY PROPERTY ADDITIONAL_MAKE_CLEAN_FILES "${PROJECT_SOURCE_DIR}/ui/public")

# add_custom_target(clean-html ALL
#     COMMAND rm -rf "${PROJECT_SOURCE_DIR}/ui/html"
# )

################################################################################
# Linxu用Install
################################################################################
if(NOT WIN32)
install(TARGETS linux-bin)
install(DIRECTORY
          "${PROJECT_SOURCE_DIR}/ui/assets"
          "${PROJECT_SOURCE_DIR}/ui/cgi-bin"
          "${PROJECT_SOURCE_DIR}/ui/html"
          "${PROJECT_SOURCE_DIR}/ui/public"
       # ${CMAKE_INSTALL_PREFIX}/share/peercast以下に保存される
        DESTINATION share/peercast
)
install(DIRECTORY
          "${PROJECT_SOURCE_DIR}/licenses"
        DESTINATION share/doc/peercast
)
endif()

################################################################################
# Linxu用 Packaging
# peercast-yt[PEERCAST_YT_VERSION]-[CMAKE_CXX_PLATFORM_ID]-[ARCHITECTURE].tar.gz
# https://cmake.org/cmake/help/latest/module/CPack.html#module:CPack
################################################################################
if(NOT WIN32)
  set(CPACK_GENERATOR "TGZ")
  include(CPack) # Install定義からArchiveを自動で作る
endif()


################################################################################
# DEBUG用 cmake -S . -B build で変数が全部出る
################################################################################
# get_cmake_property(_variableNames VARIABLES)
# foreach (_variableName ${_variableNames})
#     message(STATUS "${_variableName}=${${_variableName}}")
# endforeach()




################################################################################
# MEMO
################################################################################
# - add_custom_command / add_custom_target あたりの使い分けについて
#   https://theolizer.com/cpp-school3/cpp-school3-4/
# - LinuxでのTarボール化
#   https://cmake.org/cmake/help/latest/guide/tutorial/Packaging%20an%20Installer.html



# target_link_options(peercast PRIVATE -static-libgcc -static-libstdc++)
# target_link_options(peercast PRIVATE -static-libstdc++)

#add_executable(peercast ui/mingw/main.cpp)
# 面倒だしstatic-linkしちまおうぜ
# set(CMAKE_EXE_LINKER_FLAGS " -static")
# target_link_options(peercast-yt-cli PRIVATE -static-libgcc -static-libstdc++)

# # 使用するライブラリの指定
# if(WIN32)
#   # Static
#   target_link_libraries(peercast-yt-cli winpthread)
#   # Dynamic
#   # target_link_libraries(peercast-yt-cli winpthread) # dynamic
#   target_link_libraries(peercast-yt-cli wsock32 ws2_32)
#   target_link_libraries(peercast-yt-cli Shlwapi)
# endif()

# 絶対パスのほうがいいとかなんとか
# cmake_policy(SET CMP0076 NEW) # CMakeが自動的に相対パスを絶対パスへ変換する
# target_sources(mylib
#   PRIVATE
#     file1.cpp
#     file2.cpp
# )