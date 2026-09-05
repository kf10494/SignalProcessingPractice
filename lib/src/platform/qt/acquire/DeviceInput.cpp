///
/// @file DeviceInput.cpp
///

#include "qt/DeviceInput.hpp"

#include <bit>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <span>

#include <QAudioDevice>
#include <QMediaDevices>

#include "common/AudioConfig.hpp"

AudioPushDevice::AudioPushDevice(RingBufferAcquire* ring_buffer_acquire)
    : ring_buffer_acquire_(ring_buffer_acquire)
{
}

void AudioPushDevice::SetSampleFormat(QAudioFormat::SampleFormat format)
{
    sample_format_ = format;
}

auto AudioPushDevice::readData([[maybe_unused]] char* data,
                               [[maybe_unused]] qint64 max_size) -> qint64
{
    // 書き込み専用デバイスのため読み出しは非対応.
    return -1;
}

auto AudioPushDevice::writeData(const char* data, qint64 size) -> qint64
{
    if (sample_format_ == QAudioFormat::Float) {
        const auto count = static_cast<std::size_t>(size) / sizeof(float);
        ring_buffer_acquire_->Push(
                std::span<const float>{std::bit_cast<const float*>(data), count});
    } else {
        // Int16 → float へ変換して転送する.
        const auto count = static_cast<std::size_t>(size) / sizeof(std::int16_t);
        const std::span<const std::int16_t> source{std::bit_cast<const std::int16_t*>(data), count};
        convert_buffer_.resize(count);
        constexpr float kInt16Scale =
                1.0F / (static_cast<float>(std::numeric_limits<std::int16_t>::max()) + 1.0F);
        for (std::size_t i = 0; i < count; ++i) {
            convert_buffer_[i] = static_cast<float>(source[i]) * kInt16Scale;
        }
        ring_buffer_acquire_->Push(
                std::span<const float>{convert_buffer_.data(), convert_buffer_.size()});
    }

    // バッファ満杯時は破棄する方針のため, 常に全量を消費したものとして扱う.
    return size;
}

DeviceInput::DeviceInput(RingBufferAcquire* ring_buffer_acquire)
    : push_device_(ring_buffer_acquire)
{
}

DeviceInput::~DeviceInput()
{
    Stop();
}

auto DeviceInput::GetDeviceNames() -> std::vector<std::string>
{
    const auto devices = QMediaDevices::audioInputs();
    std::vector<std::string> names;
    names.reserve(static_cast<std::size_t>(devices.size()));
    for (const auto& device : devices) {
        names.push_back(device.description().toStdString());
    }
    return names;
}

auto DeviceInput::Start(int device_index) -> bool
{
    if (audio_source_ != nullptr) {
        return true;
    }

    const auto devices = QMediaDevices::audioInputs();
    const QAudioDevice device = (device_index >= 0 && device_index < devices.size())
                                        ? devices.at(device_index)
                                        : QMediaDevices::defaultAudioInput();
    if (device.isNull()) {
        std::cerr << "DeviceInput: no audio input device found\n";
        return false;
    }

    QAudioFormat format;
    format.setSampleRate(static_cast<int>(kAppSampleRate));
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Float);
    if (!device.isFormatSupported(format)) {
        format.setSampleFormat(QAudioFormat::Int16);
        if (!device.isFormatSupported(format)) {
            std::cerr << "DeviceInput: " << kAppSampleRate
                      << " Hz mono capture is not supported by device '"
                      << device.description().toStdString() << "'\n";
            return false;
        }
    }

    push_device_.SetSampleFormat(format.sampleFormat());
    if (!push_device_.isOpen()) {
        push_device_.open(QIODevice::WriteOnly);
    }

    audio_source_ = std::make_unique<QAudioSource>(device, format);
    audio_source_->start(&push_device_);
    if (audio_source_->error() != QAudio::NoError) {
        std::cerr << "DeviceInput: failed to start capture on device '"
                  << device.description().toStdString()
                  << "' (error=" << static_cast<int>(audio_source_->error()) << ")\n";
        audio_source_.reset();
        return false;
    }

    std::cout << "DeviceInput: capturing from '" << device.description().toStdString() << "' ("
              << (format.sampleFormat() == QAudioFormat::Float ? "Float" : "Int16") << ")\n";
    return true;
}

void DeviceInput::Stop()
{
    if (audio_source_ != nullptr) {
        audio_source_->stop();
        audio_source_.reset();
    }
}
