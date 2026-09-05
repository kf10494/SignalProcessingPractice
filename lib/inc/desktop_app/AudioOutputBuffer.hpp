///
/// @file AudioOutputBuffer.hpp
///
#pragma once

#include <cstddef>
#include <span>

#include <etl/queue_spsc_atomic.h>

#include "common/FrameSyncProcess.hpp"

///
/// @brief 出力サンプルを受け渡す SPSC リングバッファ.
///
/// Producer (処理スレッドの Output Strategy) と Consumer (QAudioSink の読み出し) の
/// 1:1 接続を前提とする. 入力側と異なり Consumer はデバイス駆動のため待機機構を持たない.
///
class AudioOutputBuffer {
public:
    ///
    /// バッファ容量 (32 ホップ分, 約 372 ms @ 44.1 kHz).
    ///
    /// Null / Sine / File 入力時は, 処理スレッドが std::this_thread::sleep_until()
    /// で自走してこのバッファへ書き込む (MainModel::RunProcessing() 参照). 実時間
    /// スケジューリングの保証がない通常優先度スレッドのため, OS のスケジューリング
    /// 遅延をここで吸収できるだけの余裕を持たせる. 小さすぎると
    /// AudioPullDevice::readData() が枯渇して無音で埋められ, 音切れ (ポップノイズ)
    /// の原因になる.
    ///
    static constexpr std::size_t kCapacity = FrameSyncProcess::audio_hop_length * 32;

    ///
    /// サンプル列を書き込む.
    ///
    /// 空き容量が不足する場合は部分書き込みせず全体を破棄し false を返す.
    ///
    auto Push(std::span<const float> samples) -> bool;

    ///
    /// 最大 out.size() 個のサンプルを取り出す.
    ///
    /// @return 実際に取り出したサンプル数 (不足時は out の先頭のみ埋まる).
    ///
    auto Pop(std::span<float> out) -> std::size_t;

    ///
    /// バッファをクリアする (Consumer 側から呼び出すこと).
    ///
    void Clear();

private:
    etl::queue_spsc_atomic<float, kCapacity> queue_;
};
