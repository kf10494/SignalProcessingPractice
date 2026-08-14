///
/// @file PipelineContext.cpp
///

#include "PipelineContext.hpp"

#include <utility>

#include "common/FrameSyncProcess.hpp"
#include "common/FrameSyncProcessConfig.hpp"
#include "common/PipelineResult.hpp"

PipelineContext::PipelineContext(const FrameSyncProcessConfig& config)
    : audio_acquire_strategy_(config.audio_acquire_strategy),
      pre_process_strategy_(config.pre_process_strategy),
      overlap_strategy_(config.overlap_strategy),
      window_strategy_(config.window_strategy),
      fft_strategy_(config.fft_strategy),
      infer_strategy_(config.infer_strategy),
      post_process_strategy_(config.post_process_strategy),
      overlap_add_strategy_(config.overlap_add_strategy),
      audio_output_strategy_(config.audio_output_strategy)
{
    /* do nothing. */
}

auto PipelineContext::exec() -> void
{
    exec(nullptr);
}

auto PipelineContext::exec(PipelineResult* result) -> void
{
    auto input_hop = this->audio_acquire_strategy_();
    auto pre_processed = this->pre_process_strategy_(input_hop);
    auto overlapped = this->overlap_strategy_(pre_processed);
    auto win_applied = this->window_strategy_(overlapped);
    auto fft_result = this->fft_strategy_(win_applied);
    auto infer_output = this->infer_strategy_(fft_result);
    auto post_processed = this->post_process_strategy_(infer_output.frame);
    auto synthesized = this->overlap_add_strategy_(post_processed);
    this->audio_output_strategy_(synthesized);

    if (result != nullptr) {
        result->input_hop = input_hop;
        result->pre_processed_hop = pre_processed;
        result->overlapped_frame = overlapped;
        result->windowed_frame = win_applied;
        result->fft_frame = fft_result;
        result->inferred_frame = infer_output.frame;
        result->infer_result = infer_output.result;
        result->post_processed_frame = post_processed;
        result->output_hop = synthesized;
    }
}

auto PipelineContext::reset() -> void
{
    audio_acquire_strategy_.reset();
    pre_process_strategy_.reset();
    overlap_strategy_.reset();
    window_strategy_.reset();
    fft_strategy_.reset();
    infer_strategy_.reset();
    post_process_strategy_.reset();
    overlap_add_strategy_.reset();
    audio_output_strategy_.reset();
}

auto PipelineContext::SetAcquireStrategy(FrameSyncProcess::AudioAcquireStrategy strategy) -> void
{
    audio_acquire_strategy_ = std::move(strategy);
}

auto PipelineContext::SetPreProcessStrategy(FrameSyncProcess::PreProcessStrategy strategy) -> void
{
    pre_process_strategy_ = std::move(strategy);
}

auto PipelineContext::SetOverlapStrategy(FrameSyncProcess::OverlapStrategy strategy) -> void
{
    overlap_strategy_ = std::move(strategy);
}

auto PipelineContext::SetWindowStrategy(FrameSyncProcess::WindowStrategy strategy) -> void
{
    window_strategy_ = std::move(strategy);
}

auto PipelineContext::SetFftStrategy(FrameSyncProcess::FftStrategy strategy) -> void
{
    fft_strategy_ = std::move(strategy);
}

auto PipelineContext::SetInferStrategy(FrameSyncProcess::InferStrategy strategy) -> void
{
    infer_strategy_ = std::move(strategy);
}

auto PipelineContext::SetPostProcessStrategy(FrameSyncProcess::PostProcessStrategy strategy) -> void
{
    post_process_strategy_ = std::move(strategy);
}

auto PipelineContext::SetOverlapAddStrategy(FrameSyncProcess::OverlapAddStrategy strategy) -> void
{
    overlap_add_strategy_ = std::move(strategy);
}

auto PipelineContext::SetOutputStrategy(FrameSyncProcess::AudioOutputStrategy strategy) -> void
{
    audio_output_strategy_ = std::move(strategy);
}
