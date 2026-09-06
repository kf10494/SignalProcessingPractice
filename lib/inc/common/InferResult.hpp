///
/// @file InferResult.hpp
///
#pragma once

#include <etl/variant.h>

#include "common/KeywordSpottingResult.hpp"

///
/// @brief 推論結果が存在しないことを表す型.
///
/// Null/Bypass Infer Strategy など, 実際の推論を行わない場合の既定値として用いる.
///
struct NoInferResult {};

///
/// @brief InferStrategy が通知しうる推論結果の型集合.
///
/// キーワード識別・声質変換など, 推論ドメインごとの結果型をここに列挙する.
/// ヒープ確保を避けるため, 全ての alternative は trivially copyable であること.
///
using InferResult = etl::variant<NoInferResult, KeywordSpottingResult>;
