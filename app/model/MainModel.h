///
/// @file MainModel.h
///
#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <stop_token>
#include <string>
#include <thread>
#include <vector>

#include "common/FrameSyncProcess.hpp"
#include "common/PipelineSelection.h"
#include "common/Strategies/HannOverlapAdder.hpp"
#include "common/Strategies/HannWindow.hpp"
#include "common/Strategies/RingBufferAcquire.hpp"
#include "common/Strategies/RingBufferOutput.hpp"
#include "common/Strategies/SineGenerator.hpp"
#ifdef SPP_WITH_ONNXRUNTIME
#include "common/Strategies/KeywordSpottingInfer.hpp"
#endif
#include "desktop_app/AudioInputBuffer.hpp"
#include "desktop_app/AudioOutputBuffer.hpp"
#include "desktop_app/Strategies/FilePlayer.hpp"

struct PipelineResult;
class DeviceInput;
class DeviceOutput;

///
/// @brief MVP の Model 層.
///
/// FrameSyncProcess と入力系 (AudioAcquireStrategy の実体 / Device 用リングバッファ) を
/// 所有し, 処理スレッドを管理する. Null / Sine / File は一定周期で, Device は
/// リングバッファへのデータ到着を待って ProcessFrame() を実行する.
///
class MainModel {
public:
    MainModel();
    ~MainModel();

    MainModel(const MainModel&) = delete;
    auto operator=(const MainModel&) -> MainModel& = delete;
    MainModel(MainModel&&) = delete;
    auto operator=(MainModel&&) -> MainModel& = delete;

    ///
    /// 処理スレッドを起動する.
    ///
    void Start();

    ///
    /// 処理スレッドを停止する (join 完了まで待機).
    ///
    void Stop();

    ///
    /// ComboBox の選択 (stage, index) に対応する Strategy を FrameSyncProcess へ設定する.
    ///
    /// index は GetStrategyNames(stage) の並びに対応する.
    /// kAcquire も他の段と同様に AudioAcquireStrategy そのものを切り替える.
    /// "Device" 選択時はキャプチャを開始せず, ApplyDeviceSelection() を待つ.
    ///
    void ApplyStrategySelection(PipelineStage stage, int index);

    ///
    /// 利用可能な入力デバイス名の一覧を取得する.
    ///
    [[nodiscard]] static auto GetAudioInputDeviceNames() -> std::vector<std::string>;

    ///
    /// 指定 index の入力デバイスでキャプチャを開始する (入力が "Device" のときのみ有効).
    ///
    /// device_index は GetAudioInputDeviceNames() の並びに対応する.
    ///
    void ApplyDeviceSelection(int device_index);

    ///
    /// サイン波生成の周波数を設定する.
    ///
    /// 処理スレッドの Exec() 呼び出しと排他して SineGenerator を更新する (スレッドセーフ).
    ///
    void ApplySineFrequency(int frequency_hz);

    ///
    /// 音声ファイル (WAV) を読み込む.
    ///
    /// 処理スレッドの Exec() 呼び出しと排他して FilePlayer を更新する (スレッドセーフ).
    /// 読み込み失敗時は既存データ (未ロードなら無音) を維持する.
    ///
    void ApplyFileSelection(const std::string& path);

    ///
    /// 利用可能な出力デバイス名の一覧を取得する.
    ///
    [[nodiscard]] static auto GetAudioOutputDeviceNames() -> std::vector<std::string>;

    ///
    /// 指定 index の出力デバイスで再生を開始する (出力が "Device" のときのみ有効).
    ///
    /// device_index は GetAudioOutputDeviceNames() の並びに対応する.
    ///
    void ApplyOutputDeviceSelection(int device_index);

    ///
    /// FrameSyncProcess への参照を取得する.
    ///
    [[nodiscard]] auto Process() -> FrameSyncProcess&;

    ///
    /// 処理済みフレーム数を取得する (スレッドセーフ).
    ///
    [[nodiscard]] auto GetProcessedFrameCount() const -> std::uint64_t;

private:
    ///
    /// 処理スレッドのループ本体.
    ///
    void RunProcessing(const std::stop_token& stop_token);

    ///
    /// ProcessFrame() 完了通知の Observer.
    ///
    void OnFrameProcessed(const PipelineResult& result);

    FrameSyncProcess process_;

    ///
    /// @name 入力系 (Device 選択時のみ使用する非同期経路).
    ///
    /// Producer (DeviceInput) から Push() できるのは RingBufferAcquire の
    /// 公開関数のみであり, AudioInputBuffer を直接参照するのは
    /// RingBufferAcquire の内部実装だけである.
    /// {@
    AudioInputBuffer input_buffer_;
    RingBufferAcquire ring_buffer_acquire_{&input_buffer_};
    std::unique_ptr<DeviceInput> device_input_;
    /// @}

    ///
    /// @name 代替 Strategy の実体.
    ///
    /// StrategySlot / RingBufferAcquire は非所有ポインタを bind するため,
    /// 実体は MainModel が所有する. Null / Sine / File / Device のいずれも,
    /// ApplyStrategySelection() で対応する AudioAcquireStrategy を
    /// FrameSyncProcess へ直接 bind することで切り替える.
    /// 既定 (index 0 = Null) の Strategy は FrameSyncProcessConfig の
    /// static 実体を再利用する.
    /// {@
    SineGenerator sine_generator_;
    HannWindow hann_window_;
    HannOverlapAdder hann_overlap_adder_;

#ifdef SPP_WITH_ONNXRUNTIME
    ///
    /// keyword spotting 推論 Strategy (Infer 段で "KeywordSpotting" 選択時に bind).
    ///
    /// モデルパスは ctor で解決する (実行ファイル相対 → ビルド時パスの順).
    ///
    KeywordSpottingInfer keyword_infer_;
#endif

    ///
    /// 音声ファイル入力のデータソース (AudioAcquireStrategy として直接 bind する).
    ///
    FilePlayer file_player_;
    /// @}

    ///
    /// @name 出力系 (Output Strategy → リングバッファ → 出力デバイス).
    /// {@
    AudioOutputBuffer output_buffer_;
    RingBufferOutput ring_buffer_output_{&output_buffer_};
    std::unique_ptr<DeviceOutput> device_output_;
    /// @}

    std::jthread processing_thread_;

    std::atomic<std::uint64_t> processed_frame_count_{0};

    ///
    /// @name 入力切り替え / 排他制御.
    ///
    /// acquire_mutex_ は, ProcessFrame() (SineGenerator / FilePlayer の Exec()) と
    /// ApplySineFrequency() / ApplyFileSelection() による状態変更, および
    /// device_mode_ / device_wait_stop_source_ への読み書きを排他する.
    /// {@
    std::mutex acquire_mutex_;

    ///
    /// 入力が "Device" 選択中かどうか (ApplyDeviceSelection() の有効判定と
    /// RunProcessing() の待機方式の切り替えに使用).
    ///
    bool device_mode_{false};

    ///
    /// Device モード中の RingBufferAcquire::WaitForHop() を, 入力切り替え時に
    /// 即座に解除するための stop_source.
    ///
    /// Device モードへ切り替えるたびに新しい実体へ差し替える
    /// (stop_source は一度 request_stop() すると再利用できないため).
    ///
    std::stop_source device_wait_stop_source_;
    /// @}

    ///
    /// 出力が "Device" 選択中かどうか (ApplyOutputDeviceSelection() の有効判定に使用).
    ///
    bool output_device_mode_{false};
};
