///
/// @file DeviceOutput.cpp
///

#include "qt/DeviceOutput.hpp"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <span>

#include <QAudioDevice>
#include <QMediaDevices>

#include "common/AudioConfig.hpp"
#include "qt/AudioOutputBuffer.hpp"

AudioPullDevice::AudioPullDevice(AudioOutputBuffer* buffer)
    : buffer_(buffer)
{
}

void AudioPullDevice::SetSampleFormat(QAudioFormat::SampleFormat format)
{
    sample_format_ = format;
}

auto AudioPullDevice::readData(char* data, qint64 max_size) -> qint64
{
    if (sample_format_ == QAudioFormat::Float) {
        const auto count = static_cast<std::size_t>(max_size) / sizeof(float);
        const std::span<float> out{std::bit_cast<float*>(data), count};
        const std::size_t popped = buffer_->Pop(out);
        // 不足分は無音で埋めてストリームを維持する.
        std::fill(out.subspan(popped).begin(), out.subspan(popped).end(), 0.0F);
        return static_cast<qint64>(count) * static_cast<qint64>(sizeof(float));
    }

    // Int16: float から変換して転送する.
    const auto count = static_cast<std::size_t>(max_size) / sizeof(std::int16_t);
    pull_buffer_.resize(count);
    const std::size_t popped = buffer_->Pop(std::span<float>{pull_buffer_});
    std::fill(pull_buffer_.begin() + static_cast<std::ptrdiff_t>(popped), pull_buffer_.end(), 0.0F);

    const std::span<std::int16_t> out{std::bit_cast<std::int16_t*>(data), count};
    constexpr auto kInt16Max = static_cast<float>(std::numeric_limits<std::int16_t>::max());
    for (std::size_t i = 0; i < count; ++i) {
        const float clamped = std::clamp(pull_buffer_[i], -1.0F, 1.0F);
        out[i] = static_cast<std::int16_t>(clamped * kInt16Max);
    }
    return static_cast<qint64>(count) * static_cast<qint64>(sizeof(std::int16_t));
}

auto AudioPullDevice::writeData([[maybe_unused]] const char* data,
                                [[maybe_unused]] qint64 size) -> qint64
{
    // 読み出し専用デバイスのため書き込みは非対応.
    return -1;
}

auto AudioPullDevice::bytesAvailable() const -> qint64
{
    // 不足分は無音で埋めて常に読み出し可能とするため, 固定の見かけ上のバイト数を返す.
    // (既定の QIODevice::bytesAvailable() は 0 を返し, atEnd() が true になる結果
    //  QAudioSink が readData() を呼び出さなくなり, 無音のまま再生されない.)
    constexpr qint64 kApparentAvailableBytes = 1 << 20;
    return kApparentAvailableBytes + QIODevice::bytesAvailable();
}

DeviceOutput::DeviceOutput(AudioOutputBuffer* buffer)
    : pull_device_(buffer)
{
}

DeviceOutput::~DeviceOutput()
{
    Stop();
}

auto DeviceOutput::GetDeviceNames() -> std::vector<std::string>
{
    const auto devices = QMediaDevices::audioOutputs();
    std::vector<std::string> names;
    names.reserve(static_cast<std::size_t>(devices.size()));
    for (const auto& device : devices) {
        names.push_back(device.description().toStdString());
    }
    return names;
}

auto DeviceOutput::Start(int device_index) -> bool
{
    if (audio_sink_ != nullptr) {
        return true;
    }

    const auto devices = QMediaDevices::audioOutputs();
    const QAudioDevice device = (device_index >= 0 && device_index < devices.size())
                                        ? devices.at(device_index)
                                        : QMediaDevices::defaultAudioOutput();
    if (device.isNull()) {
        std::cerr << "DeviceOutput: no audio output device found\n";
        return false;
    }

    QAudioFormat format;
    format.setSampleRate(static_cast<int>(kAppSampleRate));
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Float);
    if (!device.isFormatSupported(format)) {
        format.setSampleFormat(QAudioFormat::Int16);
        if (!device.isFormatSupported(format)) {
            std::cerr << "DeviceOutput: " << kAppSampleRate
                      << " Hz mono playback is not supported by device '"
                      << device.description().toStdString() << "'\n";
            return false;
        }
    }

    pull_device_.SetSampleFormat(format.sampleFormat());
    if (!pull_device_.isOpen()) {
        pull_device_.open(QIODevice::ReadOnly);
    }

    audio_sink_ = std::make_unique<QAudioSink>(device, format);
    audio_sink_->start(&pull_device_);
    if (audio_sink_->error() != QAudio::NoError) {
        std::cerr << "DeviceOutput: failed to start playback on device '"
                  << device.description().toStdString()
                  << "' (error=" << static_cast<int>(audio_sink_->error()) << ")\n";
        audio_sink_.reset();
        return false;
    }

    std::cout << "DeviceOutput: playing to '" << device.description().toStdString() << "' ("
              << (format.sampleFormat() == QAudioFormat::Float ? "Float" : "Int16") << ")\n";
    return true;
}

void DeviceOutput::Stop()
{
    if (audio_sink_ != nullptr) {
        audio_sink_->stop();
        audio_sink_.reset();
    }
}
