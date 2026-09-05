///
/// @file DeviceOutput.hpp
///
#pragma once

#include <memory>
#include <string>
#include <vector>

#include <QAudioFormat>
#include <QAudioSink>
#include <QIODevice>

class AudioOutputBuffer;

///
/// @brief QAudioSink (push モード) の読み出し元として AudioOutputBuffer から転送する QIODevice.
///
/// readData() でリングバッファから取り出し, 不足分は無音で埋めてストリームを維持する.
/// Float / Int16 (float からの変換) に対応する.
///
class AudioPullDevice : public QIODevice {
    Q_OBJECT

public:
    explicit AudioPullDevice(AudioOutputBuffer* buffer);

    ///
    /// 出力データのサンプルフォーマットを設定する (Float / Int16 のみ対応).
    ///
    void SetSampleFormat(QAudioFormat::SampleFormat format);

protected:
    auto readData(char* data, qint64 max_size) -> qint64 override;
    auto writeData(const char* data, qint64 size) -> qint64 override;
    [[nodiscard]] auto bytesAvailable() const -> qint64 override;

private:
    AudioOutputBuffer* buffer_;
    QAudioFormat::SampleFormat sample_format_{QAudioFormat::Float};

    ///
    /// Int16 変換用の作業バッファ.
    ///
    std::vector<float> pull_buffer_;
};

///
/// @brief 出力デバイスへの音声再生 (Consumer).
///
/// Qt Multimedia (QAudioSink) を使用する. Qt イベントループ上で動作するため,
/// AudioOutputBuffer からの読み出しはメインスレッドで行われる.
///
class DeviceOutput {
public:
    explicit DeviceOutput(AudioOutputBuffer* buffer);
    ~DeviceOutput();

    DeviceOutput(const DeviceOutput&) = delete;
    auto operator=(const DeviceOutput&) -> DeviceOutput& = delete;
    DeviceOutput(DeviceOutput&&) = delete;
    auto operator=(DeviceOutput&&) -> DeviceOutput& = delete;

    ///
    /// 利用可能な出力デバイス名の一覧を取得する.
    ///
    /// 一覧の並びは Start() の device_index に対応する.
    ///
    [[nodiscard]] static auto GetDeviceNames() -> std::vector<std::string>;

    ///
    /// 指定 index の出力デバイスで再生を開始する.
    ///
    /// @param device_index GetDeviceNames() の並びに対応する index.
    ///                     範囲外の場合はデフォルト出力デバイスを使用する.
    /// @return 開始できれば true. デバイスなし・フォーマット非対応・開始失敗時は
    ///         警告を stderr へ出力して false.
    ///
    auto Start(int device_index) -> bool;

    ///
    /// 再生を停止する.
    ///
    void Stop();

private:
    AudioPullDevice pull_device_;
    std::unique_ptr<QAudioSink> audio_sink_;
};
