///
/// @file MainPresenter.cpp
///

#include "presenter/MainPresenter.h"

#include <functional>
#include <string_view>
#include <utility>

#include "view/MainWindow.h"

MainPresenter::MainPresenter(MainWindow* view)
    : view_(view),
      model_(std::make_unique<MainModel>()),
      waveform_presenter_(
              model_.get(),
              [view](std::function<void()> observer) {
                  view->AttachFrameTickObserver(std::move(observer));
              },
              [view](std::span<const float> samples) {
                  view->UpdateWaveform(samples);
              }),
      spectrum_presenter_(
              model_.get(),
              [view](std::function<void()> observer) {
                  view->AttachFrameTickObserver(std::move(observer));
              },
              [view](std::span<const float> values) {
                  view->UpdateSpectrum(values);
              }),
      infer_result_presenter_(
              model_.get(),
              [view](std::function<void()> observer) {
                  view->AttachFrameTickObserver(std::move(observer));
              },
              [view](std::string_view label, float confidence) {
                  view->UpdateInferResult(label, confidence);
              }),
      pipeline_presenter_(
              model_.get(),
              PipelineViewHooks{
                      .attach_selection =
                              [view](PipelineSelectionObserver observer) {
                                  view->AttachPipelineObserver(std::move(observer));
                              },
                      .attach_device =
                              [view](AcquireDeviceObserver observer) {
                                  view->AttachAcquireDeviceObserver(std::move(observer));
                              },
                      .show_device_selector =
                              [view](const std::vector<std::string>& device_names) {
                                  view->ShowAcquireDeviceSelector(device_names);
                              },
                      .hide_device_selector =
                              [view] {
                                  view->HideAcquireDeviceSelector();
                              },
                      .attach_sine_frequency =
                              [view](SineFrequencyObserver observer) {
                                  view->AttachSineFrequencyObserver(std::move(observer));
                              },
                      .show_sine_frequency_selector =
                              [view] {
                                  view->ShowSineFrequencySelector();
                              },
                      .hide_sine_frequency_selector =
                              [view] {
                                  view->HideSineFrequencySelector();
                              },
                      .attach_file_selection =
                              [view](FileSelectionObserver observer) {
                                  view->AttachFileSelectionObserver(std::move(observer));
                              },
                      .show_file_selector =
                              [view] {
                                  view->ShowFileSelector();
                              },
                      .hide_file_selector =
                              [view] {
                                  view->HideFileSelector();
                              },
                      .attach_output_device =
                              [view](OutputDeviceObserver observer) {
                                  view->AttachOutputDeviceObserver(std::move(observer));
                              },
                      .show_output_device_selector =
                              [view](const std::vector<std::string>& device_names) {
                                  view->ShowOutputDeviceSelector(device_names);
                              },
                      .hide_output_device_selector =
                              [view] {
                                  view->HideOutputDeviceSelector();
                              },
              })
{
    // NOTE: 開始/停止 UI の実装までは, Presenter 構築と同時に処理を開始する.
    model_->Start();
}
