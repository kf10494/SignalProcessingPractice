///
/// @file DeviceInput.hpp
///
#pragma once

#include <memory>
#include <string>
#include <vector>

#include <QAudioFormat>
#include <QAudioSource>
#include <QIODevice>

#include "common/Strategies/RingBufferAcquire.hpp"

///
/// @brief QAudioSource (push モード) の書き込み先として RingBufferAcquire へ転送する QIODevice.
///
/// writeData() で受信したサンプルを float へ変換し, RingBufferAcquire::Push() する.
/// push モードを使用することで, シグナル (readyRead) の購読なしにデータを受け取る.
///
class AudioPushDevice : public QIODevice {
    Q_OBJECT

public:
    explicit AudioPushDevice(RingBufferAcquire* ring_buffer_acquire);

    ///
    /// 受信データのサンプルフォーマットを設定する (Float / Int16 のみ対応).
    ///
    void SetSampleFormat(QAudioFormat::SampleFormat format);

protected:
    auto readData(char* data, qint64 max_size) -> qint64 override;
    auto writeData(const char* data, qint64 size) -> qint64 override;

private:
    RingBufferAcquire* ring_buffer_acquire_;
    QAudioFormat::SampleFormat sample_format_{QAudioFormat::Float};

    ///
    /// Int16 → float 変換用の作業バッファ.
    ///
    std::vector<float> convert_buffer_;
};

///
/// @brief デフォルト入力デバイスからの音声キャプチャ (Producer).
///
/// Qt Multimedia (QAudioSource) を使用する. Qt イベントループ上で動作するため,
/// RingBufferAcquire::Push() はメインスレッドから行われる.
/// RingBufferAcquire::Push() を呼ぶ Producer は常に DeviceInput のみとすること (SPSC).
///
class DeviceInput {
public:
    explicit DeviceInput(RingBufferAcquire* ring_buffer_acquire);
    ~DeviceInput();

    DeviceInput(const DeviceInput&) = delete;
    auto operator=(const DeviceInput&) -> DeviceInput& = delete;
    DeviceInput(DeviceInput&&) = delete;
    auto operator=(DeviceInput&&) -> DeviceInput& = delete;

    ///
    /// 利用可能な入力デバイス名の一覧を取得する.
    ///
    /// 一覧の並びは Start() の device_index に対応する.
    ///
    [[nodiscard]] static auto GetDeviceNames() -> std::vector<std::string>;

    ///
    /// 指定 index の入力デバイスでキャプチャを開始する.
    ///
    /// @param device_index GetDeviceNames() の並びに対応する index.
    ///                     範囲外の場合はデフォルト入力デバイスを使用する.
    /// @return 開始できれば true. デバイスなし・フォーマット非対応・開始失敗時は
    ///         警告を stderr へ出力して false.
    ///
    auto Start(int device_index) -> bool;

    ///
    /// キャプチャを停止する.
    ///
    void Stop();

private:
    AudioPushDevice push_device_;
    std::unique_ptr<QAudioSource> audio_source_;
};
