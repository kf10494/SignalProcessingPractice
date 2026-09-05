///
/// @file FilePlayer.cpp
///

#include "desktop_app/Strategies/FilePlayer.hpp"

#include <array>
#include <bit>
#include <cstdint>
#include <fstream>
#include <iostream>

#include "common/AudioConfig.hpp"

namespace {

///
/// WAVEFORMAT の音声フォーマット識別子.
///
constexpr std::uint16_t kFormatPcm = 1;
constexpr std::uint16_t kFormatIeeeFloat = 3;

constexpr std::uint16_t kBitsPerSamplePcm16 = 16;
constexpr unsigned int kBitsPerByte = 8U;
constexpr std::uint16_t kBitsPerSampleFloat32 = 32;

///
/// リトルエンディアンのバイト列から整数を組み立てる.
///
auto ReadU16(std::ifstream& stream) -> std::uint16_t
{
    std::array<unsigned char, 2> bytes{};
    stream.read(std::bit_cast<char*>(bytes.data()), bytes.size());
    return static_cast<std::uint16_t>(bytes[0] |
                                      (static_cast<std::uint16_t>(bytes[1]) << kBitsPerByte));
}

auto ReadU32(std::ifstream& stream) -> std::uint32_t
{
    std::array<unsigned char, 4> bytes{};
    stream.read(std::bit_cast<char*>(bytes.data()), bytes.size());
    return static_cast<std::uint32_t>(bytes[0]) |
           (static_cast<std::uint32_t>(bytes[1]) << kBitsPerByte) |
           (static_cast<std::uint32_t>(bytes[2]) << (2U * kBitsPerByte)) |
           (static_cast<std::uint32_t>(bytes[3]) << (3U * kBitsPerByte));
}

auto ReadChunkId(std::ifstream& stream) -> std::string
{
    std::array<char, 4> bytes{};
    stream.read(bytes.data(), bytes.size());
    return std::string{bytes.data(), bytes.size()};
}

///
/// フォーマット情報.
///
struct WavFormat {
    std::uint16_t audio_format{0};
    std::uint16_t channels{0};
    std::uint32_t sample_rate{0};
    std::uint16_t bits_per_sample{0};
};

///
/// data チャンクのバイト列をモノラル float 列へ変換する.
///
auto ConvertToMono(const std::vector<unsigned char>& raw,
                   const WavFormat& format) -> std::vector<float>
{
    const std::size_t bytes_per_sample = format.bits_per_sample / kBitsPerByte;
    const std::size_t frame_size = bytes_per_sample * format.channels;
    const std::size_t frame_count = raw.size() / frame_size;

    std::vector<float> mono;
    mono.reserve(frame_count);

    constexpr float kInt16Scale = 1.0F / 32768.0F;
    for (std::size_t frame = 0; frame < frame_count; ++frame) {
        float sum = 0.0F;
        for (std::size_t channel = 0; channel < format.channels; ++channel) {
            const std::size_t offset = (frame * frame_size) + (channel * bytes_per_sample);
            if (format.audio_format == kFormatPcm) {
                const auto low = static_cast<std::uint16_t>(raw[offset]);
                const auto high = static_cast<std::uint16_t>(raw[offset + 1]);
                const auto value = static_cast<std::int16_t>(
                        low | static_cast<std::uint16_t>(high << kBitsPerByte));
                sum += static_cast<float>(value) * kInt16Scale;
            } else {
                const std::uint32_t bits =
                        static_cast<std::uint32_t>(raw[offset]) |
                        (static_cast<std::uint32_t>(raw[offset + 1]) << kBitsPerByte) |
                        (static_cast<std::uint32_t>(raw[offset + 2]) << (2U * kBitsPerByte)) |
                        (static_cast<std::uint32_t>(raw[offset + 3]) << (3U * kBitsPerByte));
                sum += std::bit_cast<float>(bits);
            }
        }
        mono.push_back(sum / static_cast<float>(format.channels));
    }
    return mono;
}

}  // namespace

auto FilePlayer::Load(const std::string& path) -> bool
{
    std::ifstream stream{path, std::ios::binary};
    if (!stream) {
        std::cerr << "FilePlayer: failed to open '" << path << "'\n";
        return false;
    }

    // RIFF ヘッダ.
    if (ReadChunkId(stream) != "RIFF") {
        std::cerr << "FilePlayer: not a RIFF file: '" << path << "'\n";
        return false;
    }
    ReadU32(stream);  // RIFF チャンクサイズ (未使用).
    if (ReadChunkId(stream) != "WAVE") {
        std::cerr << "FilePlayer: not a WAVE file: '" << path << "'\n";
        return false;
    }

    // fmt / data チャンクを走査する (他のチャンクはスキップ).
    WavFormat format;
    std::vector<unsigned char> data;
    while (stream && (format.audio_format == 0 || data.empty())) {
        const std::string chunk_id = ReadChunkId(stream);
        const std::uint32_t chunk_size = ReadU32(stream);
        if (!stream) {
            break;
        }

        if (chunk_id == "fmt ") {
            format.audio_format = ReadU16(stream);
            format.channels = ReadU16(stream);
            format.sample_rate = ReadU32(stream);
            ReadU32(stream);  // byte rate (未使用).
            ReadU16(stream);  // block align (未使用).
            format.bits_per_sample = ReadU16(stream);
            constexpr std::uint32_t kFmtChunkBaseSize = 16;
            if (chunk_size > kFmtChunkBaseSize) {
                stream.seekg(chunk_size - kFmtChunkBaseSize, std::ios::cur);
            }
        } else if (chunk_id == "data") {
            data.resize(chunk_size);
            stream.read(std::bit_cast<char*>(data.data()),
                        static_cast<std::streamsize>(chunk_size));
        } else {
            // チャンクサイズが奇数の場合は 1 バイトのパディングを含めてスキップする.
            stream.seekg(chunk_size + (chunk_size % 2U), std::ios::cur);
        }
    }

    const bool pcm16 =
            format.audio_format == kFormatPcm && format.bits_per_sample == kBitsPerSamplePcm16;
    const bool float32 = format.audio_format == kFormatIeeeFloat &&
                         format.bits_per_sample == kBitsPerSampleFloat32;
    if ((!pcm16 && !float32) || format.channels == 0 || data.empty()) {
        std::cerr << "FilePlayer: unsupported WAV format in '" << path
                  << "' (format=" << format.audio_format << ", bits=" << format.bits_per_sample
                  << ", channels=" << format.channels << ", data=" << data.size() << " bytes)\n";
        return false;
    }

    if (format.sample_rate != kAppSampleRate) {
        std::cerr << "FilePlayer: sample rate mismatch in '" << path
                  << "' (file=" << format.sample_rate << " Hz, app=" << kAppSampleRate
                  << " Hz). Playback pitch will be shifted.\n";
    }

    samples_ = ConvertToMono(data, format);
    position_ = 0;

    std::cout << "FilePlayer: loaded '" << path << "' (" << samples_.size() << " samples, "
              << format.channels << " ch, " << format.sample_rate << " Hz, "
              << (pcm16 ? "PCM16" : "Float32") << ")\n";
    return true;
}

void FilePlayer::Reset()
{
    position_ = 0;
}

auto FilePlayer::Exec() -> FrameSyncProcess::AudioHop
{
    FrameSyncProcess::AudioHop hop{kAppSampleRate};
    if (samples_.empty()) {
        return hop;
    }
    for (std::size_t i = 0; i < FrameSyncProcess::audio_hop_length; ++i) {
        hop[i] = samples_[position_];
        position_ = (position_ + 1) % samples_.size();
    }
    return hop;
}
