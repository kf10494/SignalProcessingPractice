///
/// @file PipelineResult.hpp
///
#pragma once

#include "common/FrameSyncProcess.hpp"
#include "common/InferResult.hpp"

///
/// @brief 1 フレーム分の信号処理結果.
///
/// 各 Strategy の入出力を保持する. 全フィールドはパイプラインステージの順に並ぶ.
///
struct PipelineResult {
    FrameSyncProcess::AudioHop input_hop;
    FrameSyncProcess::AudioHop pre_processed_hop;
    FrameSyncProcess::AudioFrame overlapped_frame;
    FrameSyncProcess::AudioFrame windowed_frame;
    FrameSyncProcess::AudioFrame fft_frame;
    FrameSyncProcess::AudioFrame inferred_frame;
    InferResult infer_result{NoInferResult{}};
    FrameSyncProcess::AudioFrame post_processed_frame;
    FrameSyncProcess::AudioHop output_hop;
};
