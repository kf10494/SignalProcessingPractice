///
/// @file KeywordSpottingInfer.cpp
///
#include "common/Strategies/KeywordSpottingInfer.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iterator>
#include <string>

#include "arm_math.h"

namespace {

///
/// @brief 48 kHz -> 8 kHz 間引き用アンチエイリアス FIR (120 tap, Kaiser 窓, fc≈3.7 kHz).
///
/// 線形位相の対称係数のため, CMSIS-DSP が要求する時間反転順と通常順は一致する.
///
constexpr std::array<float, KeywordSpottingInfer::kDecimationTaps> kDecimationCoeffs{
        -4.047749166e-06F, -7.966044389e-07F, +8.698878912e-06F, +2.432975535e-05F,
        +4.248713050e-05F, +5.577663630e-05F, +5.436796740e-05F, +2.911624355e-05F,
        -2.411479903e-05F, -1.002268040e-04F, -1.825199759e-04F, -2.438723704e-04F,
        -2.523938751e-04F, -1.811148947e-04F, -1.961542013e-05F, +2.159158038e-04F,
        +4.782943869e-04F, +6.940445354e-04F, +7.785317150e-04F, +6.597148469e-04F,
        +3.052854272e-04F, -2.546586163e-04F, -9.179912416e-04F, -1.520707298e-03F,
        -1.869053363e-03F, -1.789515880e-03F, -1.185958263e-03F, -8.837553167e-05F,
        +1.323435449e-03F, +2.734350638e-03F, +3.751223639e-03F, +3.996127691e-03F,
        +3.216655141e-03F, +1.385450217e-03F, -1.241552852e-03F, -4.133217709e-03F,
        -6.569015499e-03F, -7.794575577e-03F, -7.218302546e-03F, -4.603881792e-03F,
        -2.063947096e-04F, +5.194512790e-03F, +1.039198032e-02F, +1.397819270e-02F,
        +1.466475893e-02F, +1.163329465e-02F, +4.835221408e-03F, -4.839383414e-03F,
        -1.558342356e-02F, -2.490690065e-02F, -3.005846700e-02F, -2.856332234e-02F,
        -1.877245452e-02F, -3.055540745e-04F, +2.571686502e-02F, +5.671878733e-02F,
        +8.900286738e-02F, +1.183238921e-01F, +1.405988888e-01F, +1.526183413e-01F,
        +1.526183413e-01F, +1.405988888e-01F, +1.183238921e-01F, +8.900286738e-02F,
        +5.671878733e-02F, +2.571686502e-02F, -3.055540745e-04F, -1.877245452e-02F,
        -2.856332234e-02F, -3.005846700e-02F, -2.490690065e-02F, -1.558342356e-02F,
        -4.839383414e-03F, +4.835221408e-03F, +1.163329465e-02F, +1.466475893e-02F,
        +1.397819270e-02F, +1.039198032e-02F, +5.194512790e-03F, -2.063947096e-04F,
        -4.603881792e-03F, -7.218302546e-03F, -7.794575577e-03F, -6.569015499e-03F,
        -4.133217709e-03F, -1.241552852e-03F, +1.385450217e-03F, +3.216655141e-03F,
        +3.996127691e-03F, +3.751223639e-03F, +2.734350638e-03F, +1.323435449e-03F,
        -8.837553167e-05F, -1.185958263e-03F, -1.789515880e-03F, -1.869053363e-03F,
        -1.520707298e-03F, -9.179912416e-04F, -2.546586163e-04F, +3.052854272e-04F,
        +6.597148469e-04F, +7.785317150e-04F, +6.940445354e-04F, +4.782943869e-04F,
        +2.159158038e-04F, -1.961542013e-05F, -1.811148947e-04F, -2.523938751e-04F,
        -2.438723704e-04F, -1.825199759e-04F, -1.002268040e-04F, -2.411479903e-05F,
        +2.911624355e-05F, +5.436796740e-05F, +5.577663630e-05F, +4.248713050e-05F,
        +2.432975535e-05F, +8.698878912e-06F, -7.966044389e-07F, -4.047749166e-06F};

///
/// @brief モデル入出力テンソル名.
///
constexpr std::array<const char*, 1> kInputNames{"waveform_8k"};
constexpr std::array<const char*, 1> kOutputNames{"logprobs"};

///
/// @brief ONNX ロギング用の識別子.
///
constexpr const char* kEnvLogId = "KeywordSpottingInfer";

}  // namespace

KeywordSpottingInfer::KeywordSpottingInfer(const Params& params)
    : env_(ORT_LOGGING_LEVEL_WARNING, kEnvLogId),
      memory_info_(Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU)),
      stride_samples_(params.stride_samples == 0 ? kOutputBlockSamples : params.stride_samples),
      silence_rms_(params.silence_rms),
      min_confidence_(params.min_confidence)
{
    arm_fir_decimate_init_f32(&decimator_, static_cast<std::uint16_t>(kDecimationTaps),
                              static_cast<std::uint8_t>(kDecimationFactor),
                              kDecimationCoeffs.data(), decimator_state_.data(),
                              static_cast<std::uint32_t>(kInputBlockSamples));

    try {
        Ort::SessionOptions options;
        options.SetIntraOpNumThreads(1);
        options.SetInterOpNumThreads(1);
        options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

#ifdef _WIN32
        const std::wstring path(params.model_path.begin(), params.model_path.end());
#else
        const std::string path(params.model_path);
#endif
        session_ = Ort::Session{env_, path.c_str(), options};
        ready_ = true;
    } catch (const Ort::Exception&) {
        ready_ = false;
    }
}

auto KeywordSpottingInfer::Exec(const FrameSyncProcess::AudioFrame& frame)
        -> FrameSyncProcess::InferOutput
{
    if (ready_) {
        FeedHop(frame);
    }

    return FrameSyncProcess::InferOutput{
            FrameSyncProcess::AudioFrame{},
            has_result_ ? InferResult{last_result_} : InferResult{NoInferResult{}}};
}

auto KeywordSpottingInfer::FeedHop(const FrameSyncProcess::AudioFrame& frame) -> void
{
    // Overlapper 出力の最新 hop (末尾 512 サンプル) を間引きバッファへ取り込む.
    std::copy(std::next(frame.begin(), FrameSyncProcess::audio_hop_length), frame.end(),
              std::next(input_block_.begin(), static_cast<std::ptrdiff_t>(input_fill_)));
    input_fill_ += FrameSyncProcess::audio_hop_length;

    if (input_fill_ < kInputBlockSamples) {
        return;
    }
    input_fill_ = 0;

    std::array<float, kOutputBlockSamples> decimated{};
    arm_fir_decimate_f32(&decimator_, input_block_.data(), decimated.data(),
                         static_cast<std::uint32_t>(kInputBlockSamples));

    // 8 kHz 窓を左へずらして新規サンプルを末尾へ追記する.
    std::move(std::next(window_.begin(), kOutputBlockSamples), window_.end(), window_.begin());
    std::copy(decimated.begin(), decimated.end(), std::prev(window_.end(), kOutputBlockSamples));

    filled_samples_ = std::min(filled_samples_ + kOutputBlockSamples, kModelInputSamples);
    samples_since_inference_ += kOutputBlockSamples;

    if (filled_samples_ < kModelInputSamples || samples_since_inference_ < stride_samples_) {
        return;
    }
    samples_since_inference_ = 0;
    RunInference();
}

auto KeywordSpottingInfer::RunInference() -> void
{
    float rms = 0.0F;
    arm_rms_f32(window_.data(), static_cast<std::uint32_t>(window_.size()), &rms);
    if (rms < silence_rms_) {
        return;
    }

    const std::array<std::int64_t, 3> input_shape{1, 1,
                                                  static_cast<std::int64_t>(kModelInputSamples)};
    const std::array<std::int64_t, 2> output_shape{
            1, static_cast<std::int64_t>(KeywordSpottingResult::kClassCount)};

    try {
        auto input_tensor =
                Ort::Value::CreateTensor<float>(memory_info_, window_.data(), window_.size(),
                                                input_shape.data(), input_shape.size());
        auto output_tensor =
                Ort::Value::CreateTensor<float>(memory_info_, scores_.data(), scores_.size(),
                                                output_shape.data(), output_shape.size());

        session_.Run(Ort::RunOptions{nullptr}, kInputNames.data(), &input_tensor, 1,
                     kOutputNames.data(), &output_tensor, 1);
    } catch (const Ort::Exception&) {
        return;
    }

    auto* const best = std::max_element(scores_.begin(), scores_.end());
    const auto top_index = static_cast<std::size_t>(std::distance(scores_.begin(), best));
    const float confidence = std::exp(*best);
    if (confidence < min_confidence_) {
        return;
    }

    ++sequence_counter_;
    std::copy(scores_.begin(), scores_.end(), last_result_.logprobs.begin());
    last_result_.top_index = static_cast<std::uint8_t>(top_index);
    last_result_.top_confidence = confidence;
    last_result_.sequence = sequence_counter_;
    has_result_ = true;
}

auto KeywordSpottingInfer::Reset() -> void
{
    decimator_state_.fill(0.0F);
    input_block_.fill(0.0F);
    input_fill_ = 0;

    window_.fill(0.0F);
    filled_samples_ = 0;
    samples_since_inference_ = 0;

    last_result_ = KeywordSpottingResult{};
    has_result_ = false;
}
