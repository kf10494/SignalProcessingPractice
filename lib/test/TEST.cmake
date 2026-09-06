#
# XXX: gtest は pthread に依存するため, 予め find_package() しておく.
#
find_package(Threads REQUIRED)

#
# Include the Google Test framework
#
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
    EXCLUDE_FROM_ALL
)

#
# For Windows: Prevent overriding the parent project's compiler/linker settings
#
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

#
# Add subdirectories
#
include(${CMAKE_CURRENT_LIST_DIR}/core/frame_sync/TEST_FrameSyncProcess.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/slot/TEST_StrategySlot.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/strategies/TEST_Reset.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/pipeline/TEST_PipelineContext.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/observer/TEST_Observer.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/acquire/TEST_RingBufferAcquire.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/infer/TEST_KeywordSpottingInfer.cmake)

enable_testing()
