///
/// @file TEST_KeywordSpottingInfer.cpp
///
#include <cmath>
#include <memory>
#include <numbers>
#include <string_view>

#include <etl/variant.h>
#include <gtest/gtest.h>

#include "common/FrameSyncProcess.hpp"
#include "common/InferResult.hpp"
#include "common/KeywordSpottingResult.hpp"
#include "common/Strategies/KeywordSpottingInfer.hpp"

namespace {

constexpr std::string_view kModelPath = KEYWORD_SPOTTING_MODEL_PATH;

///
/// 推論 1 回を確実にトリガするために投入するフレーム数.
///
/// 窓 8000 サンプル / (1 ブロック 256 サンプル / 3 フレーム) = 約 94 フレームで初回推論.
///
constexpr int kFramesForInference = 150;

constexpr double kToneFrequencyHz = 1000.0;
constexpr float kToneAmplitude = 0.3F;
constexpr double kTwoPi = 2.0 * std::numbers::pi;

///
/// @brief 連続正弦波を模した時間領域フレームを生成する (全域を同一波形で埋める).
///
auto MakeToneFrame(double& phase) -> FrameSyncProcess::AudioFrame
{
    FrameSyncProcess::AudioFrame frame;
    const double increment = kTwoPi * kToneFrequencyHz / static_cast<double>(frame.sample_rate());

    for (auto& sample : frame) {
        sample = kToneAmplitude * static_cast<float>(std::sin(phase));
        phase += increment;
        if (phase >= kTwoPi) {
            phase -= kTwoPi;
        }
    }
    return frame;
}

auto MakeInfer() -> std::unique_ptr<KeywordSpottingInfer>
{
    return std::make_unique<KeywordSpottingInfer>(
            KeywordSpottingInfer::Params{.model_path = kModelPath});
}

///
/// @brief トーンを十分な回数流し, 最後の Exec 出力を返す.
///
auto RunToneInference(KeywordSpottingInfer& infer) -> FrameSyncProcess::InferOutput
{
    double phase = 0.0;
    FrameSyncProcess::InferOutput output;
    for (int i = 0; i < kFramesForInference; ++i) {
        output = infer.Exec(MakeToneFrame(phase));
    }
    return output;
}

}  // namespace

TEST(KeywordSpottingInfer, ReadyWithValidModel)
{
    const auto infer = MakeInfer();
    EXPECT_TRUE(infer->ready());
}

TEST(KeywordSpottingInfer, NotReadyWithInvalidModel)
{
    const auto infer = std::make_unique<KeywordSpottingInfer>(
            KeywordSpottingInfer::Params{.model_path = "/nonexistent/keyword_spotting.onnx"});
    EXPECT_FALSE(infer->ready());
}

TEST(KeywordSpottingInfer, SilenceProducesNoResult)
{
    const auto infer = MakeInfer();
    const FrameSyncProcess::AudioFrame silence;

    FrameSyncProcess::InferOutput output;
    for (int i = 0; i < kFramesForInference; ++i) {
        output = infer->Exec(silence);
    }

    EXPECT_TRUE(etl::holds_alternative<NoInferResult>(output.result));
}

TEST(KeywordSpottingInfer, ToneProducesKeywordResult)
{
    const auto infer = MakeInfer();
    const auto output = RunToneInference(*infer);

    const auto* result = etl::get_if<KeywordSpottingResult>(&output.result);
    ASSERT_NE(result, nullptr);
    EXPECT_LT(result->top_index, KeywordSpottingResult::kClassCount);
    EXPECT_GE(result->sequence, 1U);
    EXPECT_GT(result->top_confidence, 0.0F);
    EXPECT_LE(result->top_confidence, 1.0F);
}

TEST(KeywordSpottingInfer, InferredFrameIsEmpty)
{
    const auto infer = MakeInfer();
    const auto output = RunToneInference(*infer);

    // Infer 段は音声処理に関与しないため, 返すフレームは空 (全ゼロ).
    EXPECT_EQ(output.frame, FrameSyncProcess::AudioFrame{});
}

TEST(KeywordSpottingInfer, ResetClearsResult)
{
    const auto infer = MakeInfer();
    RunToneInference(*infer);

    infer->Reset();

    const FrameSyncProcess::AudioFrame silence;
    const auto output = infer->Exec(silence);
    EXPECT_TRUE(etl::holds_alternative<NoInferResult>(output.result));
}
