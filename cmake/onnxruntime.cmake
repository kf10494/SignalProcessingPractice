#
# ONNX Runtime (プリビルド配布物) を FetchContent で取得し,
# onnxruntime::onnxruntime という IMPORTED ターゲットを定義する.
#
# ソースからのビルドは非常に重いため, 公式リリースのプリビルドアーカイブを
# ダウンロードして展開し, 手動で IMPORTED ターゲット化する.
# (アーカイブ同梱の onnxruntimeConfig.cmake は install ツリー用のパス前提で
#  壊れているため使用しない.)
#
# WITH_ONNXRUNTIME=OFF のとき (組込みビルド等) は何もしない.
#
# NOTE: 公式プリビルドは MSVC ビルドであり, MinGW (本プロジェクトの Windows
#       ツールチェーン) では SAL アノテーションを含むヘッダを解釈できずビルドに
#       失敗する. そのため MinGW では既定 OFF とする. Windows で keyword spotting を
#       有効化するには MSVC ツールチェーンへの切り替え等が必要.
#

if(MINGW)
    set(_spp_ort_default OFF)
else()
    set(_spp_ort_default ON)
endif()

option(WITH_ONNXRUNTIME "Enable ONNX Runtime based infer strategies" ${_spp_ort_default})
unset(_spp_ort_default)

if(WITH_ONNXRUNTIME AND MINGW)
    message(WARNING
        "WITH_ONNXRUNTIME=ON with a MinGW toolchain: the official ONNX Runtime "
        "prebuilds are MSVC-only and will not compile here. Use an MSVC toolchain "
        "or set -DWITH_ONNXRUNTIME=OFF.")
endif()

if(NOT WITH_ONNXRUNTIME)
    return()
endif()

if(TARGET onnxruntime::onnxruntime)
    return()
endif()

set(ORT_VERSION 1.20.1)

if(WIN32)
    set(ORT_ARCHIVE_NAME "onnxruntime-win-x64-${ORT_VERSION}")
    set(ORT_ARCHIVE_EXT  "zip")
    set(ORT_ARCHIVE_HASH "78d447051e48bd2e1e778bba378bec4ece11191c9e538cf7b2c4a4565e8f5581")
    set(ORT_SHARED_LIB   "lib/onnxruntime.dll")
    set(ORT_IMPORT_LIB   "lib/onnxruntime.lib")
else()
    set(ORT_ARCHIVE_NAME "onnxruntime-linux-x64-${ORT_VERSION}")
    set(ORT_ARCHIVE_EXT  "tgz")
    set(ORT_ARCHIVE_HASH "67db4dc1561f1e3fd42e619575c82c601ef89849afc7ea85a003abbac1a1a105")
    set(ORT_SHARED_LIB   "lib/libonnxruntime.so")
    set(ORT_IMPORT_LIB   "")
endif()

include(FetchContent)
FetchContent_Declare(
    onnxruntime
    URL      "https://github.com/microsoft/onnxruntime/releases/download/v${ORT_VERSION}/${ORT_ARCHIVE_NAME}.${ORT_ARCHIVE_EXT}"
    URL_HASH "SHA256=${ORT_ARCHIVE_HASH}"
)
FetchContent_MakeAvailable(onnxruntime)

add_library(onnxruntime::onnxruntime SHARED IMPORTED GLOBAL)
set_target_properties(onnxruntime::onnxruntime PROPERTIES
    IMPORTED_LOCATION "${onnxruntime_SOURCE_DIR}/${ORT_SHARED_LIB}"
    INTERFACE_INCLUDE_DIRECTORIES        "${onnxruntime_SOURCE_DIR}/include"
    INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${onnxruntime_SOURCE_DIR}/include")

if(WIN32)
    set_target_properties(onnxruntime::onnxruntime PROPERTIES
        IMPORTED_IMPLIB "${onnxruntime_SOURCE_DIR}/${ORT_IMPORT_LIB}")
endif()

#
# 実行時の共有ライブラリ探索用にパスを公開する.
#   - Linux: BUILD_RPATH に ONNXRUNTIME_LIB_DIR を追加すればビルドツリーから実行可能.
#            リリース (AppImage) は linuxdeploy が DT_NEEDED を辿って同梱する.
#   - Windows: onnxruntime.dll を実行ファイルの隣へ POST_BUILD コピーする必要がある.
#
set(ONNXRUNTIME_LIB_DIR "${onnxruntime_SOURCE_DIR}/lib"
    CACHE INTERNAL "Directory containing the ONNX Runtime shared library")
set(ONNXRUNTIME_SHARED_LIB "${onnxruntime_SOURCE_DIR}/${ORT_SHARED_LIB}"
    CACHE INTERNAL "Path to the ONNX Runtime shared library")
