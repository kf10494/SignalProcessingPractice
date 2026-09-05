///
/// @file MainModel.cpp
///

#include "model/MainModel.h"

#include <chrono>
#include <mutex>
#include <utility>

#include "common/AudioConfig.hpp"
#include "common/FrameSyncProcessConfig.hpp"
#include "common/PipelineResult.hpp"
#include "desktop_app/DeviceInput.hpp"
#include "desktop_app/DeviceOutput.hpp"

MainModel::MainModel()
    : device_input_(std::make_unique<DeviceInput>(&ring_buffer_acquire_)),
      sine_generator_({.frequency = SineGenerator::kDefaultFrequency,
                       .amplitude = SineGenerator::kDefaultAmplitude}),
      device_output_(std::make_unique<DeviceOutput>(&output_buffer_))
{
    process_.SetConfig(FrameSyncProcess::AcquireTag{}, get_default_null_input_strategy());
    process_.Attach(
            FrameSyncProcess::ProcessCompleteObserver::create<MainModel,
                                                              &MainModel::OnFrameProcessed>(*this));
}

MainModel::~MainModel()
{
    Stop();
}

void MainModel::Start()
{
    if (processing_thread_.joinable()) {
        return;
    }
    processing_thread_ = std::jthread{[this](const std::stop_token& stop_token) {
        RunProcessing(stop_token);
    }};
}

void MainModel::Stop()
{
    device_output_->Stop();
    device_input_->Stop();
    {
        const std::lock_guard<std::mutex> guard{acquire_mutex_};
        device_wait_stop_source_.request_stop();
    }
    if (processing_thread_.joinable()) {
        processing_thread_.request_stop();
        processing_thread_.join();
    }
}

void MainModel::ApplyStrategySelection(PipelineStage stage, int index)
{
    switch (stage) {
        case PipelineStage::kAcquire:
            if (index == kAcquireDeviceItemIndex) {
                {
                    const std::lock_guard<std::mutex> guard{acquire_mutex_};
                    device_wait_stop_source_ = std::stop_source{};
                    device_mode_ = true;
                }
                process_.SetConfig(FrameSyncProcess::AcquireTag{},
                                   FrameSyncProcess::AudioAcquireStrategy{&ring_buffer_acquire_});
                // キャプチャの開始はデバイス選択 (ApplyDeviceSelection) を待つ.
            } else {
                {
                    const std::lock_guard<std::mutex> guard{acquire_mutex_};
                    device_mode_ = false;
                    device_wait_stop_source_.request_stop();
                }
                device_input_->Stop();
                if (index == kAcquireSineItemIndex) {
                    process_.SetConfig(FrameSyncProcess::AcquireTag{},
                                       FrameSyncProcess::AudioAcquireStrategy{&sine_generator_});
                } else if (index == kAcquireFileItemIndex) {
                    // 未ロード時は FilePlayer が無音を返す. ファイル選択は ApplyFileSelection().
                    process_.SetConfig(FrameSyncProcess::AcquireTag{},
                                       FrameSyncProcess::AudioAcquireStrategy{&file_player_});
                } else {
                    process_.SetConfig(FrameSyncProcess::AcquireTag{},
                                       get_default_null_input_strategy());
                }
            }
            break;
        case PipelineStage::kPreProcess:
            process_.SetConfig(FrameSyncProcess::PreProcessTag{},
                               get_default_bypass_preprocess_strategy());
            break;
        case PipelineStage::kOverlap:
            process_.SetConfig(FrameSyncProcess::OverlapTag{}, get_default_overlapper_strategy());
            break;
        case PipelineStage::kWindow:
            process_.SetConfig(FrameSyncProcess::WindowTag{},
                               index == 1 ? FrameSyncProcess::WindowStrategy{&hann_window_}
                                          : get_default_rectangle_window_strategy());
            break;
        case PipelineStage::kFft:
            process_.SetConfig(
                    FrameSyncProcess::FftTag{},
                    index == 1 ? get_default_bypass_fft_strategy() : get_default_fft_strategy());
            break;
        case PipelineStage::kInfer:
            process_.SetConfig(FrameSyncProcess::InferTag{}, get_default_bypass_infer_strategy());
            break;
        case PipelineStage::kPostProcess:
            process_.SetConfig(FrameSyncProcess::PostProcessTag{},
                               get_default_ifft_postprocess_strategy());
            break;
        case PipelineStage::kOverlapAdd:
            process_.SetConfig(FrameSyncProcess::OverlapAddTag{},
                               index == 1
                                       ? FrameSyncProcess::OverlapAddStrategy{&hann_overlap_adder_}
                                       : get_default_rectangle_overlap_adder_strategy());
            break;
        case PipelineStage::kOutput:
            if (index == kOutputDeviceItemIndex) {
                process_.SetConfig(FrameSyncProcess::OutputTag{},
                                   FrameSyncProcess::AudioOutputStrategy{&ring_buffer_output_});
                // 再生の開始はデバイス選択 (ApplyOutputDeviceSelection) を待つ.
                output_device_mode_ = true;
            } else {
                output_device_mode_ = false;
                device_output_->Stop();
                process_.SetConfig(FrameSyncProcess::OutputTag{},
                                   get_default_null_output_strategy());
            }
            break;
    }
}

auto MainModel::GetAudioInputDeviceNames() -> std::vector<std::string>
{
    return DeviceInput::GetDeviceNames();
}

void MainModel::ApplyDeviceSelection(int device_index)
{
    if (!device_mode_) {
        return;
    }
    device_input_->Stop();
    device_input_->Start(device_index);
}

void MainModel::ApplySineFrequency(int frequency_hz)
{
    // 処理スレッドの Exec() 呼び出しと排他して周波数を更新する.
    const std::lock_guard<std::mutex> guard{acquire_mutex_};
    sine_generator_.SetFrequency(static_cast<double>(frequency_hz));
}

void MainModel::ApplyFileSelection(const std::string& path)
{
    // 処理スレッドの Exec() 呼び出しと排他してロードする.
    const std::lock_guard<std::mutex> guard{acquire_mutex_};
    file_player_.Load(path);
}

auto MainModel::GetAudioOutputDeviceNames() -> std::vector<std::string>
{
    return DeviceOutput::GetDeviceNames();
}

void MainModel::ApplyOutputDeviceSelection(int device_index)
{
    if (!output_device_mode_) {
        return;
    }
    device_output_->Stop();
    device_output_->Start(device_index);
}

auto MainModel::Process() -> FrameSyncProcess&
{
    return process_;
}

auto MainModel::GetProcessedFrameCount() const -> std::uint64_t
{
    return processed_frame_count_.load(std::memory_order_relaxed);
}

void MainModel::RunProcessing(const std::stop_token& stop_token)
{
    auto next_deadline = std::chrono::steady_clock::now();
    while (!stop_token.stop_requested()) {
        bool device_mode = false;
        std::stop_token device_wait_token;
        {
            const std::lock_guard<std::mutex> guard{acquire_mutex_};
            device_mode = device_mode_;
            device_wait_token = device_wait_stop_source_.get_token();
        }

        if (device_mode) {
            // Device: リングバッファへのデータ到着 (または入力切り替え/停止) を待つ.
            if (!ring_buffer_acquire_.WaitForHop(std::move(device_wait_token))) {
                continue;
            }
            const std::lock_guard<std::mutex> guard{acquire_mutex_};
            process_.ProcessFrame();
            next_deadline = std::chrono::steady_clock::now();
        } else {
            // Null / Sine / File: 実デバイスのクロックが存在しないため, ホップ周期で自走する.
            next_deadline += kHopPeriod;
            {
                const std::lock_guard<std::mutex> guard{acquire_mutex_};
                process_.ProcessFrame();
            }
            std::this_thread::sleep_until(next_deadline);
        }
    }
}

void MainModel::OnFrameProcessed([[maybe_unused]] const PipelineResult& result)
{
    processed_frame_count_.fetch_add(1, std::memory_order_relaxed);
}
