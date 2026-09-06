///
/// @file KeywordSpottingInfer.hpp
///
#pragma once

#include <onnxruntime_cxx_api.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "arm_math.h"
#include "common/FrameSyncProcess.hpp"
#include "common/KeywordSpottingResult.hpp"

///
/// @brief keyword_spotting.onnx (M5 / Speech Commands 35 クラス) による Infer Strategy.
///
/// @par 前提とするパイプライン構成
///      本 Strategy の Exec() は「時間領域フレーム」を必要とする. パイプライン順は
///      overlap -> window -> fft -> infer であるため, 次の構成で使用すること.
///        - Window   : Rectangle (Hann だと窓重みのかかった信号が入力される)
///        - Fft      : Bypass    (これにより Exec が overlap 出力の生波形を受け取る)
///        - Infer    : KeywordSpotting (本 Strategy)
///      不整合な構成では推論結果は更新されない (音声処理も行わず空フレームを返すのみ).
///
/// @par 処理内容
///      Exec() ごとに Overlapper の最新 hop (512 サンプル @ 48 kHz) を取り出し,
///      アンチエイリアス FIR で 1/6 間引き (48 kHz -> 8 kHz) して 8000 サンプル
///      (1.0 s) の窓へ蓄積する. 一定量の新規サンプルが貯まるたびに ONNX Runtime で
///      推論し, 結果を InferResult(KeywordSpottingResult) として返す.
///
/// @note 推論はパイプライン処理スレッド上で同期実行される. 全バッファは静的確保で,
///       セッション生成 (モデルロード) 時を除きヒープ確保を行わない.
///
class KeywordSpottingInfer {
public:
    ///
    /// @name コンパイル時定数.
    /// @{
    ///
    /// モデル入力サンプルレート.
    ///
    static constexpr std::uint32_t kModelSampleRate = 8000;

    ///
    /// モデル入力サンプル数 (1.0 s @ 8 kHz).
    ///
    static constexpr std::size_t kModelInputSamples = 8000;

    ///
    /// 48 kHz -> 8 kHz の間引き比.
    ///
    static constexpr std::size_t kDecimationFactor = 6;

    ///
    /// 間引き用 FIR のタップ数 (kDecimationFactor の倍数).
    ///
    static constexpr std::size_t kDecimationTaps = 120;

    ///
    /// 1 回の間引きで消費する 48 kHz サンプル数 (hop 3 個ぶん, kDecimationFactor の倍数).
    ///
    static constexpr std::size_t kInputBlockSamples = 3 * FrameSyncProcess::audio_hop_length;

    ///
    /// 1 回の間引きで得られる 8 kHz サンプル数.
    ///
    static constexpr std::size_t kOutputBlockSamples = kInputBlockSamples / kDecimationFactor;

    ///
    /// FIR 状態バッファ長 (numTaps + blockSize - 1).
    ///
    static constexpr std::size_t kDecimationStateSize = kDecimationTaps + kInputBlockSamples - 1;

    ///
    /// 無音判定の既定 RMS しきい値.
    ///
    static constexpr float kDefaultSilenceRms = 0.02F;

    ///
    /// 推論結果採用の既定確率しきい値 (0 = 常に採用).
    ///
    static constexpr float kDefaultMinConfidence = 0.0F;
    /// @}

    ///
    /// @brief 構築パラメータ.
    ///
    struct Params {
        ///
        /// keyword_spotting.onnx へのパス.
        ///
        std::string_view model_path;

        ///
        /// 推論間隔 (8 kHz ドメインの新規サンプル数). 小さいほど高頻度だが処理負荷が増す.
        ///
        std::size_t stride_samples = kOutputBlockSamples;

        ///
        /// この RMS 未満の窓は無音とみなし推論をスキップする.
        ///
        float silence_rms = kDefaultSilenceRms;

        ///
        /// この確率未満の推論結果は採用しない (結果を更新しない).
        ///
        float min_confidence = kDefaultMinConfidence;
    };

    explicit KeywordSpottingInfer(const Params& params);

    KeywordSpottingInfer(const KeywordSpottingInfer&) = delete;
    auto operator=(const KeywordSpottingInfer&) -> KeywordSpottingInfer& = delete;
    KeywordSpottingInfer(KeywordSpottingInfer&&) = delete;
    auto operator=(KeywordSpottingInfer&&) -> KeywordSpottingInfer& = delete;
    ~KeywordSpottingInfer() = default;

    ///
    /// @brief Infer Strategy 本体.
    ///
    /// @param frame overlap 出力の時間領域フレーム (Fft=Bypass 前提).
    /// @return 空フレームと, 直近の推論結果 (未確定なら NoInferResult).
    ///
    auto Exec(const FrameSyncProcess::AudioFrame& frame) -> FrameSyncProcess::InferOutput;

    ///
    /// @brief 内部バッファ・FIR 状態・直近結果をクリアする.
    ///
    auto Reset() -> void;

    ///
    /// @brief セッションの初期化に成功しているか.
    ///
    [[nodiscard]] auto ready() const noexcept -> bool
    {
        return ready_;
    }

private:
    ///
    /// @brief 48 kHz 512 サンプルを間引きバッファへ取り込み, 満杯なら間引いて窓へ流す.
    ///
    auto FeedHop(const FrameSyncProcess::AudioFrame& frame) -> void;

    ///
    /// @brief 8 kHz の窓に対して推論を実行し, last_result_ を更新する.
    ///
    auto RunInference() -> void;

    ///
    /// @name 間引き (48 kHz -> 8 kHz).
    /// @{
    arm_fir_decimate_instance_f32 decimator_{};
    std::array<float, kDecimationStateSize> decimator_state_{};
    std::array<float, kInputBlockSamples> input_block_{};
    std::size_t input_fill_ = 0;
    /// @}

    ///
    /// @name 8 kHz の推論窓.
    /// @{
    std::array<float, kModelInputSamples> window_{};
    std::size_t filled_samples_ = 0;
    std::size_t samples_since_inference_ = 0;
    /// @}

    ///
    /// @name ONNX Runtime.
    /// @{
    Ort::Env env_;
    Ort::MemoryInfo memory_info_;
    Ort::Session session_{nullptr};
    std::array<float, KeywordSpottingResult::kClassCount> scores_{};
    /// @}

    ///
    /// @name 結果.
    /// @{
    KeywordSpottingResult last_result_{};
    std::uint64_t sequence_counter_ = 0;
    bool has_result_ = false;
    bool ready_ = false;
    /// @}

    ///
    /// @name パラメータ.
    /// @{
    std::size_t stride_samples_;
    float silence_rms_;
    float min_confidence_;
    /// @}
};
