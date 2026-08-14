///
/// @file AudioInputBuffer.cpp
///

#include "desktop_app/AudioInputBuffer.hpp"

#include <utility>

#include "common/AudioConfig.hpp"

auto AudioInputBuffer::Push(std::span<const float> samples) -> bool
{
    if (queue_.available() < samples.size()) {
        return false;
    }
    for (const float sample : samples) {
        queue_.push(sample);
    }

    // push とデータ量判定の順序を保証するため, 空のクリティカルセクションを挟んで通知する.
    {
        const std::lock_guard<std::mutex> guard{mutex_};
    }
    hop_available_.notify_one();
    return true;
}

auto AudioInputBuffer::WaitForHop(std::stop_token stop_token) -> bool
{
    std::unique_lock<std::mutex> lock{mutex_};
    return hop_available_.wait(lock, std::move(stop_token), [this] {
        return queue_.size() >= FrameSyncProcess::audio_hop_length;
    });
}

void AudioInputBuffer::PopHop(AudioHop* out)
{
    out->set_sample_rate(kAppSampleRate);
    for (std::size_t i = 0; i < FrameSyncProcess::audio_hop_length; ++i) {
        float sample = 0.0F;
        if (!queue_.pop(sample)) {
            sample = 0.0F;
        }
        (*out)[i] = sample;
    }
}

void AudioInputBuffer::Clear()
{
    queue_.clear();
}
