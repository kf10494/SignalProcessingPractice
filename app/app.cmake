#
# Qt6 を検索
#
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Multimedia)

#
# Qt プロジェクトのセットアップ
#
qt_standard_project_setup()

#
# ソースファイルの収集
#
file(GLOB SRC_FILES
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_LIST_DIR}/*.cpp
    ${CMAKE_CURRENT_LIST_DIR}/*.h
    ${CMAKE_CURRENT_LIST_DIR}/*.hpp
    ${CMAKE_CURRENT_LIST_DIR}/common/*.cpp
    ${CMAKE_CURRENT_LIST_DIR}/common/*.h
    ${CMAKE_CURRENT_LIST_DIR}/common/*.hpp
    ${CMAKE_CURRENT_LIST_DIR}/model/*.cpp
    ${CMAKE_CURRENT_LIST_DIR}/model/*.h
    ${CMAKE_CURRENT_LIST_DIR}/model/*.hpp
    ${CMAKE_CURRENT_LIST_DIR}/presenter/*.cpp
    ${CMAKE_CURRENT_LIST_DIR}/presenter/*.h
    ${CMAKE_CURRENT_LIST_DIR}/presenter/*.hpp
    ${CMAKE_CURRENT_LIST_DIR}/view/*.cpp
    ${CMAKE_CURRENT_LIST_DIR}/view/*.h
    ${CMAKE_CURRENT_LIST_DIR}/view/*.hpp
    ${CMAKE_CURRENT_LIST_DIR}/view/*.ui
    ${CMAKE_CURRENT_LIST_DIR}/view/*.qrc
)

#
# 実行ファイルのターゲット定義
#
qt_add_executable(SignalProcessingPracticeApp
    ${SRC_FILES}
)

target_include_directories(SignalProcessingPracticeApp
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(SignalProcessingPracticeApp
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Multimedia
        SIGNAL_PROCESSING_PRACTICE_LIB
)

install(TARGETS SignalProcessingPracticeApp
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

#
# ONNX Runtime の実行時ライブラリ探索設定.
#   - Linux : ビルドツリーの ONNX Runtime を BUILD_RPATH で参照 (dev 実行用).
#             リリース (AppImage) は linuxdeploy が DT_NEEDED を辿って同梱する.
#   - Windows: onnxruntime.dll を実行ファイルの隣へコピーする.
#
if(WITH_ONNXRUNTIME)
    if(WIN32)
        add_custom_command(TARGET SignalProcessingPracticeApp POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${ONNXRUNTIME_SHARED_LIB}"
                    "$<TARGET_FILE_DIR:SignalProcessingPracticeApp>"
            VERBATIM)
    else()
        set_property(TARGET SignalProcessingPracticeApp APPEND PROPERTY
            BUILD_RPATH "${ONNXRUNTIME_LIB_DIR}")
    endif()
endif()

if(WIN32)
    set_target_properties(SignalProcessingPracticeApp PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

if(WIN32)
    # Qt の共有ライブラリ/プラグイン一式を install 先へ同梱する.
    # NOTE: qt_generate_deploy_app_script() は Linux 向けの共有ライブラリ同梱を
    # サポートしていない (実行しても何もせず skip される)。Linux は
    # release ワークフロー側で linuxdeploy を使って別途 AppImage化する。
    qt_generate_deploy_app_script(
        TARGET SignalProcessingPracticeApp
        OUTPUT_SCRIPT deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR
    )

    install(SCRIPT ${deploy_script})
endif()
