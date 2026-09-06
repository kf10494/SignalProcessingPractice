#
# keyword spotting モデル (ONNX) をビルドディレクトリへ配置し, パスを公開する.
#
#   - KEYWORD_SPOTTING_MODEL_FILENAME : ファイル名 (実行ファイルの隣へ置く名前)
#   - KEYWORD_SPOTTING_MODEL_BUILD    : ビルドツリー内のコピー先 (dev / test 用の絶対パス)
#
# 実行時は「実行ファイルの隣」を優先し, 見つからなければ KEYWORD_SPOTTING_MODEL_BUILD を
# フォールバックとして使う (app 側 ResolveKeywordSpottingModelPath()).
# パッケージ (AppImage / Windows zip) への同梱は app/app.cmake の install ルールで行う.
#

if(NOT WITH_ONNXRUNTIME)
    return()
endif()

set(KEYWORD_SPOTTING_MODEL_FILENAME "keyword_spotting.onnx"
    CACHE INTERNAL "Keyword spotting model filename")
set(KEYWORD_SPOTTING_MODEL_BUILD "${CMAKE_BINARY_DIR}/${KEYWORD_SPOTTING_MODEL_FILENAME}"
    CACHE INTERNAL "Keyword spotting model path in the build tree")

configure_file(
    "${CMAKE_SOURCE_DIR}/model/keyword_spotting/${KEYWORD_SPOTTING_MODEL_FILENAME}"
    "${KEYWORD_SPOTTING_MODEL_BUILD}"
    COPYONLY)
