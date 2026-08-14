///
/// @file FilePlayer.hpp
///
#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "common/FrameSyncProcess.hpp"

///
/// @brief WAV ファイルを読み込み, ホップ単位で供給する Producer 用データソース.
///
/// 対応形式: RIFF WAV の PCM 16bit / IEEE Float 32bit.
/// 多チャンネルはチャンネル平均でモノラル化する. 再生は末尾でループする.
///
class FilePlayer {
public:
    ///
    /// WAV ファイルを読み込む.
    ///
    /// 成功時は読み出し位置を先頭へ戻す.
    /// 失敗時は stderr へ警告を出力して false を返し, 既存データを保持する.
    ///
    auto Load(const std::string& path) -> bool;

    ///
    /// 読み出し位置を先頭へ戻す.
    ///
    /// AudioAcquireStrategy としての Reset() を兼ねる.
    ///
    void Reset();

    ///
    /// 現在位置から 1 ホップ分を返し, 位置を進める (末尾でループ).
    ///
    /// 未ロード時は無音を返す. AudioAcquireStrategy としての Exec() を兼ねる.
    ///
    auto Exec() -> FrameSyncProcess::AudioHop;

private:
    ///
    /// モノラル化済みのサンプル列.
    ///
    std::vector<float> samples_;

    ///
    /// 読み出し位置.
    ///
    std::size_t position_{0};
};
