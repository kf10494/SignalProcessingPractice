///
/// @file PipelineSelection.h
///
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <string_view>

///
/// @brief 音声処理パイプラインの段.
///
enum class PipelineStage : std::uint8_t {
    kAcquire = 0,
    kPreProcess,
    kOverlap,
    kWindow,
    kFft,
    kInfer,
    kPostProcess,
    kOverlapAdd,
    kOutput,
};

///
/// パイプラインの段数.
///
inline constexpr std::size_t kPipelineStageCount = 9;

///
/// @brief Strategy 選択変更の Observer 型.
///
/// View (MainWindow) の ComboBox 変更時に (段, 選択 index) が通知される.
///
using PipelineSelectionObserver = std::function<void(PipelineStage stage, int index)>;

///
/// 入力段の "Sine" 項目の index.
///
inline constexpr int kAcquireSineItemIndex = 1;

///
/// 入力段の "Device" 項目の index.
///
inline constexpr int kAcquireDeviceItemIndex = 2;

///
/// @brief 入力デバイス選択変更の Observer 型.
///
/// View のデバイス選択 ComboBox 変更時に選択 index が通知される.
///
using AcquireDeviceObserver = std::function<void(int device_index)>;

///
/// @brief サイン波周波数変更の Observer 型.
///
/// View の周波数 SpinBox 変更時に周波数 (Hz) が通知される.
///
using SineFrequencyObserver = std::function<void(int frequency_hz)>;

///
/// 入力段の "File" 項目の index.
///
inline constexpr int kAcquireFileItemIndex = 3;

///
/// @brief 音声ファイル選択の Observer 型.
///
/// View のファイル選択ダイアログ確定時にファイルパスが通知される.
///
using FileSelectionObserver = std::function<void(const std::string& path)>;

///
/// 出力段の "Device" 項目の index.
///
inline constexpr int kOutputDeviceItemIndex = 1;

///
/// @brief 出力デバイス選択変更の Observer 型.
///
/// View の出力デバイス選択 ComboBox 変更時に選択 index が通知される.
///
using OutputDeviceObserver = std::function<void(int device_index)>;

namespace pipeline_selection_detail {

inline constexpr std::array<std::string_view, 4> kAcquireNames{"Null", "Sine", "Device", "File"};
inline constexpr std::array<std::string_view, 1> kPreProcessNames{"Bypass"};
inline constexpr std::array<std::string_view, 1> kOverlapNames{"Overlapper"};
inline constexpr std::array<std::string_view, 2> kWindowNames{"Rectangle", "Hann"};
inline constexpr std::array<std::string_view, 2> kFftNames{"FFT", "Bypass"};
#ifdef SPP_WITH_ONNXRUNTIME
inline constexpr std::array<std::string_view, 2> kInferNames{"Bypass", "KeywordSpotting"};
#else
inline constexpr std::array<std::string_view, 1> kInferNames{"Bypass"};
#endif
inline constexpr std::array<std::string_view, 1> kPostProcessNames{"IFFT"};
inline constexpr std::array<std::string_view, 2> kOverlapAddNames{"Rectangle", "Hann"};
inline constexpr std::array<std::string_view, 2> kOutputNames{"Null", "Device"};

}  // namespace pipeline_selection_detail

///
/// @brief 各段で選択可能な Strategy 名を取得する.
///
/// View の ComboBox 項目と Model の index 解釈の単一情報源とする.
/// index 0 が既定の Strategy に対応する.
///
[[nodiscard]] inline auto GetStrategyNames(PipelineStage stage) -> std::span<const std::string_view>
{
    namespace detail = pipeline_selection_detail;
    switch (stage) {
        case PipelineStage::kAcquire:
            return detail::kAcquireNames;
        case PipelineStage::kPreProcess:
            return detail::kPreProcessNames;
        case PipelineStage::kOverlap:
            return detail::kOverlapNames;
        case PipelineStage::kWindow:
            return detail::kWindowNames;
        case PipelineStage::kFft:
            return detail::kFftNames;
        case PipelineStage::kInfer:
            return detail::kInferNames;
        case PipelineStage::kPostProcess:
            return detail::kPostProcessNames;
        case PipelineStage::kOverlapAdd:
            return detail::kOverlapAddNames;
        case PipelineStage::kOutput:
            return detail::kOutputNames;
    }
    return {};
}
