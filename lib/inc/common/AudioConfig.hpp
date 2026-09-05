///
/// @file AudioConfig.hpp
///
#pragma once

#include <chrono>
#include <cstdint>

#include "common/FrameSyncProcess.hpp"

///
/// アプリケーションのサンプルレート.
///
inline constexpr std::uint32_t kAppSampleRate = 48000U;

///
/// 1 ホップ分の周期 (512 / 48000 Hz ≒ 10.67 ms).
///
inline constexpr auto kHopPeriod = std::chrono::nanoseconds{
        std::chrono::nanoseconds{std::chrono::seconds{1}}.count() *
        static_cast<std::int64_t>(FrameSyncProcess::audio_hop_length) / kAppSampleRate};
