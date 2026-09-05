///
/// @file AudioOutputBuffer.cpp
///

#include "desktop_app/AudioOutputBuffer.hpp"

auto AudioOutputBuffer::Push(std::span<const float> samples) -> bool
{
    if (queue_.available() < samples.size()) {
        return false;
    }
    for (const float sample : samples) {
        queue_.push(sample);
    }
    return true;
}

auto AudioOutputBuffer::Pop(std::span<float> out) -> std::size_t
{
    std::size_t popped = 0;
    for (float& sample : out) {
        if (!queue_.pop(sample)) {
            break;
        }
        ++popped;
    }
    return popped;
}

void AudioOutputBuffer::Clear()
{
    queue_.clear();
}
