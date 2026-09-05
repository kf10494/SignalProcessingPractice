#
# ユニットテスト用の CMake スクリプトをインクルード.
#
# TODO: ユニットテストを, 設定で OFF を切り替えられるようにする.
#
include(${CMAKE_CURRENT_LIST_DIR}/test/TEST.cmake)

#
# カレントディレクトリ配下のファイルをすべて取得 (再帰探索はしない)
#
file(GLOB SRC_FILES 
    ${CMAKE_CURRENT_LIST_DIR}/src/core/frame_sync/*.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/core/frame_sync/*.hpp
    ${CMAKE_CURRENT_LIST_DIR}/src/core/pipeline/*.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/core/pipeline/*.hpp

    ${CMAKE_CURRENT_LIST_DIR}/src/platform/common/acquire/*.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/platform/common/acquire/*.hpp
    ${CMAKE_CURRENT_LIST_DIR}/src/platform/common/fft/*.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/platform/common/fft/*.hpp
    ${CMAKE_CURRENT_LIST_DIR}/src/platform/common/infer/*.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/platform/common/infer/*.hpp
    ${CMAKE_CURRENT_LIST_DIR}/src/platform/common/overlap/*.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/platform/common/overlap/*.hpp
    ${CMAKE_CURRENT_LIST_DIR}/src/platform/common/overlap_add/*.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/platform/common/overlap_add/*.hpp
    ${CMAKE_CURRENT_LIST_DIR}/src/platform/common/output/*.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/platform/common/output/*.hpp
    ${CMAKE_CURRENT_LIST_DIR}/src/platform/common/preprocess/*.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/platform/common/preprocess/*.hpp
    ${CMAKE_CURRENT_LIST_DIR}/src/platform/common/postprocess/*.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/platform/common/postprocess/*.hpp
    ${CMAKE_CURRENT_LIST_DIR}/src/platform/common/shared_logic/*.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/platform/common/shared_logic/*.hpp
    ${CMAKE_CURRENT_LIST_DIR}/src/platform/common/window/*.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/platform/common/window/*.hpp
    )

#
# Qt に依存するプラットフォーム実装は, BUILD_APP=ON (Qt が find_package 済み) の
# ときのみビルド対象に含める. gtest-clang 等の Qt を用いないビルドを壊さないため.
#
if(BUILD_APP)
    file(GLOB QT_SRC_FILES
        ${CMAKE_CURRENT_LIST_DIR}/src/platform/desktop_app/acquire/*.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/platform/desktop_app/acquire/*.hpp
        ${CMAKE_CURRENT_LIST_DIR}/src/platform/desktop_app/output/*.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/platform/desktop_app/output/*.hpp
        # Q_OBJECT を含む公開ヘッダ (inc/desktop_app/) は, AUTOMOC が確実に検出できるよう
        # ターゲットの SOURCES として明示的に含める.
        ${CMAKE_CURRENT_LIST_DIR}/inc/desktop_app/*.hpp
        )
    list(APPEND SRC_FILES ${QT_SRC_FILES})
endif()

include(FetchContent)

#
# ETL のインクルード
#
FetchContent_Declare(
    etl
    GIT_REPOSITORY https://github.com/ETLCPP/etl.git
    GIT_TAG        20.47.1
)

FetchContent_MakeAvailable(etl)

#
# CMSIS-DSP のインクルード
#
FetchContent_Declare(
    cmsis_dsp
    GIT_REPOSITORY https://github.com/ARM-software/CMSIS-DSP.git
    GIT_TAG        v1.17.0
)

FetchContent_MakeAvailable(cmsis_dsp)

#
# CMSIS-DSP のヘッダを SYSTEM 扱いにする.
# (clang-tidy 等の静的解析時間を抑えるため. Qt/ETL/GoogleTest は各々の
#  CMake 設定で SYSTEM 扱いになっているが, CMSISDSP ターゲットはなっていない.)
#
get_target_property(CMSIS_DSP_INCLUDES CMSISDSP INTERFACE_INCLUDE_DIRECTORIES)
set_target_properties(CMSISDSP PROPERTIES
    INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${CMSIS_DSP_INCLUDES}")

#
# Add a library target
#
add_library(SIGNAL_PROCESSING_PRACTICE_LIB
            STATIC 
            ${SRC_FILES})

target_link_libraries(SIGNAL_PROCESSING_PRACTICE_LIB
                      PUBLIC
                      etl::etl
                      CMSISDSP)

#
# Qt に依存するソースを含む場合のみ, Qt をリンクし AUTOMOC (Q_OBJECT の moc 処理) を有効にする.
# app.cmake が先に include() されているため, find_package(Qt6 ...) 済みの
# Qt6::* ターゲットをここで参照できる.
#
if(BUILD_APP)
    target_link_libraries(SIGNAL_PROCESSING_PRACTICE_LIB
                          PUBLIC
                          Qt6::Core
                          Qt6::Multimedia)

    set_target_properties(SIGNAL_PROCESSING_PRACTICE_LIB PROPERTIES AUTOMOC ON)
endif()

#
# Set the PUBLIC include path
#
target_include_directories(
    SIGNAL_PROCESSING_PRACTICE_LIB
    PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/inc/)
