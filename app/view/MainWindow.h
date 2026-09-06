///
/// @file MainWindow.h
///
#pragma once

#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <QMainWindow>

#include "common/PipelineSelection.h"

namespace Ui {
class MainWindow;
}

class PlotWidget;
class QLabel;
class QTimer;

///
/// @brief 表示更新周期 (約 30 fps) の Tick Observer 型.
///
using FrameTickObserver = std::function<void()>;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    MainWindow(const MainWindow&) = delete;
    auto operator=(const MainWindow&) -> MainWindow& = delete;
    MainWindow(MainWindow&&) = delete;
    auto operator=(MainWindow&&) -> MainWindow& = delete;

    ///
    /// @brief パイプライン Strategy 選択変更の Observer を登録する.
    ///
    /// 登録した Observer は, 音声処理パイプラインの ComboBox 変更時に
    /// (段, 選択 index) で呼び出される.
    ///
    void AttachPipelineObserver(PipelineSelectionObserver observer);

    ///
    /// @brief 入力デバイス選択変更の Observer を登録する.
    ///
    void AttachAcquireDeviceObserver(AcquireDeviceObserver observer);

    ///
    /// @brief サイン波周波数変更の Observer を登録する.
    ///
    void AttachSineFrequencyObserver(SineFrequencyObserver observer);

    ///
    /// @brief 音声ファイル選択の Observer を登録する.
    ///
    void AttachFileSelectionObserver(FileSelectionObserver observer);

    ///
    /// @brief 出力デバイス選択変更の Observer を登録する.
    ///
    void AttachOutputDeviceObserver(OutputDeviceObserver observer);

    ///
    /// @name 入力デバイス選択 ComboBox の表示制御.
    /// {@
    ///
    /// デバイス名一覧を投入して表示し, 現在の選択を Observer へ通知する.
    ///
    void ShowAcquireDeviceSelector(const std::vector<std::string>& device_names);

    ///
    /// 非表示にする.
    ///
    void HideAcquireDeviceSelector();
    /// @}

    ///
    /// @name サイン波周波数 SpinBox の表示制御.
    /// {@
    ///
    /// 表示し, 現在の設定値を Observer へ通知する.
    ///
    void ShowSineFrequencySelector();

    ///
    /// 非表示にする.
    ///
    void HideSineFrequencySelector();
    /// @}

    ///
    /// @name 音声ファイル選択ボタンの表示制御.
    /// {@
    void ShowFileSelector();
    void HideFileSelector();
    /// @}

    ///
    /// @name 出力デバイス選択 ComboBox の表示制御.
    /// {@
    ///
    /// デバイス名一覧を投入して表示し, 現在の選択を Observer へ通知する.
    ///
    void ShowOutputDeviceSelector(const std::vector<std::string>& device_names);

    ///
    /// 非表示にする.
    ///
    void HideOutputDeviceSelector();
    /// @}

    ///
    /// @brief 表示更新周期の Tick Observer を登録する.
    ///
    /// 登録した Observer は QTimer により約 30 fps で呼び出される.
    ///
    void AttachFrameTickObserver(FrameTickObserver observer);

    ///
    /// @name Presenter からの描画指示.
    /// {@
    ///
    /// 時間軸波形を描画する (振幅レンジ ±1.0).
    ///
    void UpdateWaveform(std::span<const float> samples);

    ///
    /// 振幅スペクトラムを描画する (レンジ -100〜0 dB).
    ///
    void UpdateSpectrum(std::span<const float> values);

    ///
    /// 推論結果 (キーワードと確率) を表示する.
    ///
    void UpdateInferResult(std::string_view label, float confidence);
    /// @}

private:
    ///
    /// 各 ComboBox への選択肢投入とシグナル接続.
    ///
    void SetupPipelineComboBoxes();

    ///
    /// プレースホルダへの PlotWidget の埋め込み.
    ///
    void SetupPlotWidgets();

    ///
    /// 推論結果表示ラベルの埋め込み.
    ///
    void SetupInferResultWidget();

    ///
    /// 表示更新タイマーの起動.
    ///
    void SetupFrameTick();

    ///
    /// 登録済み Observer への通知.
    ///
    void NotifyPipelineSelection(PipelineStage stage, int index);
    void NotifyAcquireDeviceSelection(int device_index);
    void NotifySineFrequency(int frequency_hz);
    void NotifyFileSelection(const std::string& path);
    void NotifyOutputDeviceSelection(int device_index);
    void NotifyFrameTick();

    Ui::MainWindow* ui;

    PlotWidget* waveform_plot_{nullptr};
    PlotWidget* spectrum_plot_{nullptr};
    QLabel* infer_result_label_{nullptr};
    QTimer* frame_tick_timer_{nullptr};

    ///
    /// @name Observer リスト.
    /// {@
    std::vector<PipelineSelectionObserver> pipeline_observers_;
    std::vector<AcquireDeviceObserver> acquire_device_observers_;
    std::vector<SineFrequencyObserver> sine_frequency_observers_;
    std::vector<FileSelectionObserver> file_selection_observers_;
    std::vector<OutputDeviceObserver> output_device_observers_;
    std::vector<FrameTickObserver> frame_tick_observers_;
    /// @}
};
