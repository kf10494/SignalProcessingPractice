///
/// @file AudioInputBuffer.hpp
///
#pragma once

#include <condition_variable>
#include <mutex>
#include <span>
#include <stop_token>

#include <etl/queue_spsc_atomic.h>

#include "common/FrameSyncProcess.hpp"

///
/// @brief 入力サンプルを受け渡す SPSC リングバッファ.
///
/// Producer (DeviceInput のコールバック) と Consumer (処理スレッド) の
/// 1:1 接続を前提とする. RingBufferAcquire を介してのみアクセスされる.
///
class AudioInputBuffer {
public:
    using AudioHop = FrameSyncProcess::AudioHop;

    ///
    /// バッファ容量 (4 ホップ分).
    ///
    static constexpr std::size_t kCapacity = FrameSyncProcess::audio_hop_length * 4;

    ///
    /// サンプル列を書き込む.
    ///
    /// 空き容量が不足する場合は部分書き込みせず全体を破棄し false を返す.
    ///
    auto Push(std::span<const float> samples) -> bool;

    ///
    /// 1 ホップ分 (512 サンプル) 溜まるまで待機する.
    ///
    /// @return データが揃えば true. 停止要求時は false.
    ///
    auto WaitForHop(std::stop_token stop_token) -> bool;

    ///
    /// 1 ホップ分を取り出す. 不足分は無音 (0.0F) で埋める.
    ///
    void PopHop(AudioHop* out);

    ///
    /// バッファをクリアする (Consumer 側から呼び出すこと).
    ///
    void Clear();

private:
    etl::queue_spsc_atomic<float, kCapacity> queue_;

    ///
    /// 通知専用の同期プリミティブ (キュー本体はロックフリー).
    ///
    std::mutex mutex_;
    std::condition_variable_any hop_available_;
};
