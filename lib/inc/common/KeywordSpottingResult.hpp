///
/// @file KeywordSpottingResult.hpp
///
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

///
/// @brief キーワード識別 (Speech Commands 35 クラス) の推論結果.
///
/// KeywordSpottingInfer が InferResult に格納して通知する. ヒープ確保を避けるため
/// trivially copyable な POD として定義する. ラベル文字列は結果に含めず, top_index を
/// keyword_spotting_label_name() で引く.
///
struct KeywordSpottingResult {
    ///
    /// 分類クラス数 (モデル出力次元).
    ///
    static constexpr std::size_t kClassCount = 35;

    ///
    /// 各クラスの log-softmax 値.
    ///
    std::array<float, kClassCount> logprobs{};

    ///
    /// 最尤クラスの index.
    ///
    std::uint8_t top_index = 0;

    ///
    /// 最尤クラスの確率 (exp(logprobs[top_index])).
    ///
    float top_confidence = 0.0F;

    ///
    /// 推論を実行するたびに +1 されるシーケンス番号 (結果の更新検知用).
    ///
    std::uint64_t sequence = 0;
};

///
/// @brief keyword spotting モデルのクラスラベル一覧.
///
/// model/keyword_spotting/keyword_spotting.onnx の metadata "labels" と一致させること.
///
inline constexpr std::array<std::string_view, KeywordSpottingResult::kClassCount>
        kKeywordSpottingLabels{"backward", "bed",    "bird",    "cat",    "dog", "down",  "eight",
                               "five",     "follow", "forward", "four",   "go",  "happy", "house",
                               "learn",    "left",   "marvin",  "nine",   "no",  "off",   "on",
                               "one",      "right",  "seven",   "sheila", "six", "stop",  "three",
                               "tree",     "two",    "up",      "visual", "wow", "yes",   "zero"};

///
/// @brief クラス index からラベル文字列を取得する. 範囲外は空文字列.
///
[[nodiscard]] constexpr auto keyword_spotting_label_name(std::size_t index) noexcept
        -> std::string_view
{
    if (index >= kKeywordSpottingLabels.size()) {
        return {};
    }
    return kKeywordSpottingLabels.at(index);
}
