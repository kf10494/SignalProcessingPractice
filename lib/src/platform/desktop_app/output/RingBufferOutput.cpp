///
/// @file RingBufferOutput.cpp
///

#include "common/Strategies/RingBufferOutput.hpp"

#include <span>

#include "desktop_app/AudioOutputBuffer.hpp"

RingBufferOutput::RingBufferOutput(AudioOutputBuffer* buffer)
    : buffer_(buffer)
{
}

auto RingBufferOutput::Exec(const FrameSyncProcess::AudioHop& hop) -> void
{
    // 満杯時は破棄する方針 (Push が false を返すのみ).
    buffer_->Push(std::span<const float>{hop.data(), hop.size()});
}

auto RingBufferOutput::Reset() -> void
{
    buffer_->Clear();
}
