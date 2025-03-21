################################################################################
# Projectの依存関係
# - TOP(peercast-yt)
#   - core (/core以下)
#     - test-all
#     - linux-bin
#       - install
#     - windows(cli/gui binary, zip packaging)
#   - html, public
#   - rtmp-server
#
# やり残しなど
# FIXME: generate-[html|public]で生成される一時フォルダがsrc/ui/{html|public}のまま
#
################################################################################

# CMakeのバージョンを設定
cmake_minimum_required(VERSION 3.18)

# プロジェクト名と使用する言語を設定
project(peercast-yt CXX)
set(CMAKE_CXX_STANDARD, 11) # 標準の言語仕様をC++11にする

# package化する際のバージョンを設定
set(CPACK_PACKAGE_VERSION_MAJOR "50")
set(CPACK_PACKAGE_VERSION_MINOR "")
set(CPACK_PACKAGE_VERSION_PATCH "")

# Debianパッケージを作成する際に利用される
set(CPACK_PACKAGE_CONTACT "yoteichi@example.com")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Yoteichi")

if(NOT WIN32)
  # install()で利用する
  include(GNUInstallDirs)
endif()

find_package(PkgConfig REQUIRED)

# CPack用に使われる
# 現在のプラットフォーム用のランタイムライブラリを含めるために必要
include(InstallRequiredSystemLibraries)

# CTestの有効化
enable_testing()
# GoogleTestを有効化
find_package(GTest REQUIRED)
include(GoogleTest)

# RTMPの確認とデフォルトのリンク設定
pkg_check_modules(LIBRTMP QUIET librtmp)
if(LIBRTMP_FOUND)
  option(USE_RTMP "USE_RTMP" ON)
else()
  option(USE_RTMP "USE_RTMP" OFF)
endif()

################################################################################
# Project: libpeercast
################################################################################
# 実行ファイルをmain.cppから作成
add_library(core STATIC)
# SET_TARGET_PROPERTIES(lib-core PROPERTIES OUTPUT_NAME libpeercast)
# set_property(TARGET libpeercast PROPERTY CXX_STANDARD 11)

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
  target_compile_definitions(core PUBLIC _UNIX)
endif()
target_sources(core
  PRIVATE ${MY_LIBPEERCAST_SRCS}
)

# Includeディレクトリを登録
# PUBLICにすることでリンクするターゲットで自動的に読み込まれる
target_include_directories(core
  PUBLIC ${PROJECT_SOURCE_DIR}/core/common
  )
if(WIN32)
  target_include_directories(core
    PUBLIC ${PROJECT_SOURCE_DIR}/core/win32
    )
else()
  target_include_directories(core
    PUBLIC ${PROJECT_SOURCE_DIR}/core/unix
    )
endif()

# OpenSSLライブラリを探してリンク
set(OPENSSL_USE_STATIC_LIBS TRUE)
find_package(OpenSSL REQUIRED)
# NOTE: INTERFACE指定することでcoreをリンクするターゲットが自動でOpenSSLのライブラリをリンクする
target_link_libraries(core INTERFACE OpenSSL::SSL)

if(USE_RTMP)
  target_compile_definitions(core INTERFACE WITH_RTMP)
  target_link_libraries(core INTERFACE ${LIBRTMP_LIBRARIES})
endif()

if(WIN32)
  set(CMAKE_EXE_LINKER_FLAGS " -static")
  target_link_options(core PRIVATE -static-libgcc -static-libstdc++)
  target_link_libraries(core INTERFACE winpthread)
  target_link_libraries(core INTERFACE Shlwapi)
endif()

################################################################################
# tests-all
################################################################################
file(GLOB_RECURSE MY_TESTS_SRCS
  "${PROJECT_SOURCE_DIR}/tests/*.cpp"
)
# NOTE: template.cppにテスト名を追加してエラーを取り除いた
# 下はtemplate.cppをリストから取り除く場合の解決方法
# list(REMOVE_ITEM MY_TESTS_SRCS
#   "${PROJECT_SOURCE_DIR}/tests/template.cpp"
# )
add_executable(test-all
  ${MY_TESTS_SRCS}
)
target_link_libraries(test-all core GTest::GTest GTest::Main)

# NOTE: CTest Runnerで走らせる?テストケース毎に走らせるので実行が遅くなる
gtest_discover_tests(test-all)

# リンクされたバイナリをビルドが終わったタイミングで実行する
add_custom_command(
     TARGET test-all
     COMMENT "Run tests"
     POST_BUILD
     WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
     COMMAND test-all
     # ctestでテストケース毎に実行するので遅い
     #  COMMAND ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION> -R "^${UNIT_TEST}$" --output-on-failures
)

################################################################################
# rtmp_server
################################################################################
add_executable(rtmp-server rtmp-server/rtmp-server.cpp)
target_link_libraries(rtmp-server core)

################################################################################
# Project: peercast(linux)
################################################################################
if(NOT WIN32)
  add_executable(linux-bin ui/linux/main.cpp)
  SET_TARGET_PROPERTIES(linux-bin PROPERTIES OUTPUT_NAME peercast)
  target_link_libraries(linux-bin core)

################################################################################
# Linxu用Install
################################################################################
  install(TARGETS linux-bin rtmp-server)
  install(DIRECTORY
            "${PROJECT_SOURCE_DIR}/ui/assets"
            "${PROJECT_SOURCE_DIR}/ui/html"
            "${PROJECT_SOURCE_DIR}/ui/public"
         # NOTE: ${CMAKE_INSTALL_PREFIX}/share/peercast以下に保存される
          DESTINATION share/peercast
  )
  install(DIRECTORY
            "${PROJECT_SOURCE_DIR}/ui/cgi-bin"
          DESTINATION share/peercast
          PATTERN "*.cgi"
          PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
                      GROUP_EXECUTE             GROUP_READ
                                                WORLD_READ
  )
  # REVIEW: cgi-bin/bbs_reader.pyは実行属性は必要ないか？
  install(DIRECTORY
            "${PROJECT_SOURCE_DIR}/licenses"
          DESTINATION share/doc/peercast
  )

################################################################################
# Linxu用 Packaging
# peercast-yt[PEERCAST_YT_VERSION]-[CMAKE_CXX_PLATFORM_ID]-[ARCHITECTURE].tar.gz
# https://cmake.org/cmake/help/latest/module/CPack.html#module:CPack
################################################################################
  set(CPACK_GENERATOR "TGZ")
  include(CPack) # Install定義からArchiveを自動で作る
endif() # if(NOT WIN32)

################################################################################
# Project: peercast(win32)
################################################################################
if(WIN32)
  add_executable(win-bin WIN32 ui/mingui/Simple.cpp ui/mingui/gui.cpp)
  SET_TARGET_PROPERTIES(win-bin PROPERTIES OUTPUT_NAME peercast)
  target_link_libraries(win-bin core)

  target_sources(win-bin PRIVATE
    ui/mingui/Simple.rc
  )

################################################################################
# Windows(MSYS64)用 Install
################################################################################
  install(TARGETS win-bin rtmp-server
           DESTINATION /
  )
  install(DIRECTORY
            "${PROJECT_SOURCE_DIR}/ui/assets"
            "${PROJECT_SOURCE_DIR}/ui/html"
            "${PROJECT_SOURCE_DIR}/ui/public"
         # NOTE: ${CMAKE_INSTALL_PREFIX}/share/peercast以下に保存される
          DESTINATION /
  )
  install(DIRECTORY
            "${PROJECT_SOURCE_DIR}/ui/cgi-bin"
          DESTINATION /
          PATTERN "*.cgi"
          PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
                      GROUP_EXECUTE             GROUP_READ
                                                WORLD_READ
  )
  # REVIEW: cgi-bin/bbs_reader.pyは実行属性は必要ないか？
  install(DIRECTORY
            "${PROJECT_SOURCE_DIR}/licenses"
          DESTINATION /
  )

################################################################################
# Windows(MSYS64)用 Packaging
# peercast-yt[PEERCAST_YT_VERSION]-[CMAKE_CXX_PLATFORM_ID]-win64.zip
################################################################################
  set(CPACK_GENERATOR "ZIP")
  include(CPack) # Install定義からArchiveを自動で作る
endif() # if(NOT WIN32)


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

# FIXME:どうしよう
#  https://cmake.org/cmake/help/latest/policy/CMP0058.html
# make相当
# all: html
# html: html.tar
add_custom_target(html ALL
    DEPENDS html.touch.txt)
add_custom_command(
  OUTPUT html.touch.txt
  COMMAND ruby --disable-gems "${PROJECT_SOURCE_DIR}/ui/generate-html"
  COMMAND ${CMAKE_COMMAND} -E touch "${CMAKE_CURRENT_BINARY_DIR}/html.touch.txt"
  DEPENDS ${MY_HTML_CATALOGS} ${MY_HTML_TEMPLATES}
  WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/ui"
)
# NOTE: clean時に消されるようにする
set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_CLEAN_FILES
  "${PROJECT_SOURCE_DIR}/ui/html"
)

add_custom_target(public ALL
    DEPENDS public.touch.txt)
add_custom_command(
  OUTPUT public.touch.txt
  COMMAND ruby --disable-gems "${PROJECT_SOURCE_DIR}/ui/generate-public"
  COMMAND ${CMAKE_COMMAND} -E touch "${CMAKE_CURRENT_BINARY_DIR}/public.touch.txt"
  DEPENDS ${MY_HTML_CATALOGS} ${MY_PUBLIC_TEMPLATES}
  WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/ui"
)

set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_CLEAN_FILES
  "${PROJECT_SOURCE_DIR}/ui/public"
)


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
