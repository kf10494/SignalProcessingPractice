///
/// @file MainWindow.cpp
///

#include "MainWindow.h"

#include <array>
#include <memory>
#include <utility>

#include <QComboBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QString>
#include <QTimer>
#include <QVBoxLayout>

#include "common/AudioConfig.hpp"
#include "ui_MainWindow.h"
#include "view/PlotWidget.h"

namespace {

///
/// 表示更新周期 (約 30 fps).
///
constexpr int kFrameTickIntervalMs = 33;

///
/// 波形表示の振幅レンジ.
///
constexpr float kWaveformMin = -1.0F;
constexpr float kWaveformMax = 1.0F;

///
/// 波形表示の時間レンジ (1 ホップ分, ミリ秒).
///
constexpr float kMillisecondsPerSecond = 1000.0F;
constexpr float kHopDurationMs = kMillisecondsPerSecond *
                                 static_cast<float>(FrameSyncProcess::audio_hop_length) /
                                 static_cast<float>(kAppSampleRate);

///
/// スペクトラム表示の dB レンジ.
///
constexpr float kSpectrumMinDb = -100.0F;
constexpr float kSpectrumMaxDb = 0.0F;

///
/// スペクトラム表示の周波数レンジ (DC〜ナイキスト, kHz).
///
constexpr float kHzPerKHz = 1000.0F;
constexpr float kNyquistKHz = static_cast<float>(kAppSampleRate) / 2.0F / kHzPerKHz;

///
/// サイン波周波数 SpinBox の設定 (可聴域, 初期値 440 Hz).
///
constexpr int kSineFrequencyMin = 20;
constexpr int kSineFrequencyMax = 20000;
constexpr int kSineFrequencyDefault = 440;

///
/// 確率をパーセント表示にする係数と小数桁.
///
constexpr float kPercentScale = 100.0F;
constexpr int kConfidenceDecimals = 1;

}  // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    SetupPipelineComboBoxes();
    SetupPlotWidgets();
    SetupInferResultWidget();
    SetupFrameTick();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::AttachPipelineObserver(PipelineSelectionObserver observer)
{
    pipeline_observers_.push_back(std::move(observer));
}

void MainWindow::AttachAcquireDeviceObserver(AcquireDeviceObserver observer)
{
    acquire_device_observers_.push_back(std::move(observer));
}

void MainWindow::AttachSineFrequencyObserver(SineFrequencyObserver observer)
{
    sine_frequency_observers_.push_back(std::move(observer));
}

void MainWindow::AttachFileSelectionObserver(FileSelectionObserver observer)
{
    file_selection_observers_.push_back(std::move(observer));
}

void MainWindow::AttachOutputDeviceObserver(OutputDeviceObserver observer)
{
    output_device_observers_.push_back(std::move(observer));
}

void MainWindow::ShowAcquireDeviceSelector(const std::vector<std::string>& device_names)
{
    QComboBox* combo_box = ui->comboBoxAcquireDevice;

    // 選択肢の入れ替え中に不定な index で通知しないようシグナルを止め,
    // 投入完了後に現在の選択を明示的に通知する.
    combo_box->blockSignals(true);
    combo_box->clear();
    for (const auto& name : device_names) {
        combo_box->addItem(QString::fromStdString(name));
    }
    combo_box->blockSignals(false);
    combo_box->show();

    if (combo_box->currentIndex() >= 0) {
        NotifyAcquireDeviceSelection(combo_box->currentIndex());
    }
}

void MainWindow::HideAcquireDeviceSelector()
{
    ui->comboBoxAcquireDevice->hide();
}

void MainWindow::ShowSineFrequencySelector()
{
    ui->spinBoxSineFrequency->show();
    NotifySineFrequency(ui->spinBoxSineFrequency->value());
}

void MainWindow::HideSineFrequencySelector()
{
    ui->spinBoxSineFrequency->hide();
}

void MainWindow::ShowFileSelector()
{
    ui->pushButtonAcquireFile->show();
}

void MainWindow::HideFileSelector()
{
    ui->pushButtonAcquireFile->hide();
}

void MainWindow::ShowOutputDeviceSelector(const std::vector<std::string>& device_names)
{
    QComboBox* combo_box = ui->comboBoxOutputDevice;

    // 選択肢の入れ替え中に不定な index で通知しないようシグナルを止め,
    // 投入完了後に現在の選択を明示的に通知する.
    combo_box->blockSignals(true);
    combo_box->clear();
    for (const auto& name : device_names) {
        combo_box->addItem(QString::fromStdString(name));
    }
    combo_box->blockSignals(false);
    combo_box->show();

    if (combo_box->currentIndex() >= 0) {
        NotifyOutputDeviceSelection(combo_box->currentIndex());
    }
}

void MainWindow::HideOutputDeviceSelector()
{
    ui->comboBoxOutputDevice->hide();
}

void MainWindow::AttachFrameTickObserver(FrameTickObserver observer)
{
    frame_tick_observers_.push_back(std::move(observer));
}

void MainWindow::UpdateWaveform(std::span<const float> samples)
{
    waveform_plot_->SetSamples(samples);
}

void MainWindow::UpdateSpectrum(std::span<const float> values)
{
    spectrum_plot_->SetSamples(values);
}

void MainWindow::UpdateInferResult(std::string_view label, float confidence)
{
    if (label.empty()) {
        infer_result_label_->setText(QStringLiteral("—"));
        return;
    }
    const QString text =
            QStringLiteral("%1  (%2%)")
                    .arg(QString::fromUtf8(label.data(), static_cast<qsizetype>(label.size())))
                    .arg(confidence * kPercentScale, 0, 'f', kConfidenceDecimals);
    infer_result_label_->setText(text);
}

void MainWindow::SetupPipelineComboBoxes()
{
    const std::array<std::pair<PipelineStage, QComboBox*>, kPipelineStageCount> combo_boxes{{
            {PipelineStage::kAcquire, ui->comboBoxAcquire},
            {PipelineStage::kPreProcess, ui->comboBoxPreProcess},
            {PipelineStage::kOverlap, ui->comboBoxOverlap},
            {PipelineStage::kWindow, ui->comboBoxWindow},
            {PipelineStage::kFft, ui->comboBoxFft},
            {PipelineStage::kInfer, ui->comboBoxInfer},
            {PipelineStage::kPostProcess, ui->comboBoxPostProcess},
            {PipelineStage::kOverlapAdd, ui->comboBoxOverlapAdd},
            {PipelineStage::kOutput, ui->comboBoxOutput},
    }};

    for (const auto& [stage, combo_box] : combo_boxes) {
        for (const auto name : GetStrategyNames(stage)) {
            combo_box->addItem(QString::fromUtf8(name.data(), static_cast<qsizetype>(name.size())));
        }
        connect(combo_box, &QComboBox::currentIndexChanged, this, [this, stage](int index) {
            NotifyPipelineSelection(stage, index);
        });
    }

    // 入力デバイス選択 ComboBox は "Device" 選択時のみ表示する.
    ui->comboBoxAcquireDevice->hide();
    connect(ui->comboBoxAcquireDevice, &QComboBox::currentIndexChanged, this, [this](int index) {
        if (index >= 0) {
            NotifyAcquireDeviceSelection(index);
        }
    });

    // サイン波周波数 SpinBox は "Sine" 選択時のみ表示する.
    ui->spinBoxSineFrequency->setRange(kSineFrequencyMin, kSineFrequencyMax);
    ui->spinBoxSineFrequency->setValue(kSineFrequencyDefault);
    ui->spinBoxSineFrequency->setSuffix(QStringLiteral(" Hz"));
    ui->spinBoxSineFrequency->hide();
    connect(ui->spinBoxSineFrequency, &QSpinBox::valueChanged, this, [this](int value) {
        NotifySineFrequency(value);
    });

    // 出力デバイス選択 ComboBox は出力 "Device" 選択時のみ表示する.
    ui->comboBoxOutputDevice->hide();
    connect(ui->comboBoxOutputDevice, &QComboBox::currentIndexChanged, this, [this](int index) {
        if (index >= 0) {
            NotifyOutputDeviceSelection(index);
        }
    });

    // 音声ファイル選択ボタンは "File" 選択時のみ表示する.
    ui->pushButtonAcquireFile->setText(QStringLiteral("選択..."));
    ui->pushButtonAcquireFile->hide();
    connect(ui->pushButtonAcquireFile, &QPushButton::clicked, this, [this] {
        const QString path =
                QFileDialog::getOpenFileName(this, QStringLiteral("音声ファイルを選択"), QString{},
                                             QStringLiteral("WAV (*.wav)"));
        if (path.isEmpty()) {
            return;
        }
        ui->pushButtonAcquireFile->setText(QFileInfo{path}.fileName());
        NotifyFileSelection(path.toStdString());
    });
}

namespace {

///
/// プレースホルダへ PlotWidget を埋め込む.
///
/// 生成したオブジェクトの所有権は Qt の親子機構 (placeholder) へ移譲する.
///
auto EmbedPlotWidget(QWidget* placeholder) -> PlotWidget*
{
    auto plot = std::make_unique<PlotWidget>();
    auto layout = std::make_unique<QVBoxLayout>();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(plot.get());

    auto* plot_ptr = plot.release();
    placeholder->setLayout(layout.release());
    return plot_ptr;
}

}  // namespace

void MainWindow::SetupPlotWidgets()
{
    waveform_plot_ = EmbedPlotWidget(ui->widgetWaveform);
    waveform_plot_->SetAxes(
            {.min_value = 0.0F, .max_value = kHopDurationMs, .unit = QStringLiteral("ms")},
            {.min_value = kWaveformMin, .max_value = kWaveformMax, .unit = QString{}});

    spectrum_plot_ = EmbedPlotWidget(ui->widgetSpectrum);
    spectrum_plot_->SetAxes(
            {.min_value = 0.0F, .max_value = kNyquistKHz, .unit = QStringLiteral("kHz")},
            {.min_value = kSpectrumMinDb,
             .max_value = kSpectrumMaxDb,
             .unit = QStringLiteral("dB")});
}

void MainWindow::SetupInferResultWidget()
{
    // 所有権は Qt の親子機構 (placeholder) へ移譲する.
    auto label = std::make_unique<QLabel>(QStringLiteral("—"));
    auto layout = std::make_unique<QVBoxLayout>();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(label.get());

    infer_result_label_ = label.release();
    ui->widgetInferResult->setLayout(layout.release());
}

void MainWindow::SetupFrameTick()
{
    // 所有権は Qt の親子機構 (this) へ移譲する.
    auto frame_tick_timer = std::make_unique<QTimer>(this);
    connect(frame_tick_timer.get(), &QTimer::timeout, this, [this] {
        NotifyFrameTick();
    });
    frame_tick_timer->start(kFrameTickIntervalMs);
    frame_tick_timer_ = frame_tick_timer.release();
}

void MainWindow::NotifyPipelineSelection(PipelineStage stage, int index)
{
    for (const auto& observer : pipeline_observers_) {
        observer(stage, index);
    }
}

void MainWindow::NotifyAcquireDeviceSelection(int device_index)
{
    for (const auto& observer : acquire_device_observers_) {
        observer(device_index);
    }
}

void MainWindow::NotifySineFrequency(int frequency_hz)
{
    for (const auto& observer : sine_frequency_observers_) {
        observer(frequency_hz);
    }
}

void MainWindow::NotifyFileSelection(const std::string& path)
{
    for (const auto& observer : file_selection_observers_) {
        observer(path);
    }
}

void MainWindow::NotifyOutputDeviceSelection(int device_index)
{
    for (const auto& observer : output_device_observers_) {
        observer(device_index);
    }
}

void MainWindow::NotifyFrameTick()
{
    for (const auto& observer : frame_tick_observers_) {
        observer();
    }
}
