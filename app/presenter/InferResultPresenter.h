///
/// @file InferResultPresenter.h
///
#pragma once

#include <cstdint>
#include <functional>
#include <string_view>

#include "common/PipelineResult.hpp"

class MainModel;

///
/// @brief 推論結果表示の Presenter 層.
///
/// View の Tick 通知ごとに MainModel から結果スナップショットを取得し,
/// InferResult が KeywordSpottingResult を保持していれば, ラベルと確率を
/// View へ描画指示する. 推論結果が更新された (sequence が進んだ) ときのみ描画する.
///
class InferResultPresenter {
public:
    ///
    /// Tick Observer 登録関数の型 (View の AttachFrameTickObserver() を注入する).
    ///
    using TickRegistrar = std::function<void(std::function<void()>)>;

    ///
    /// 描画指示関数の型 (View の UpdateInferResult() を注入する).
    ///
    using RenderFn = std::function<void(std::string_view label, float confidence)>;

    InferResultPresenter(MainModel* model, const TickRegistrar& registrar, RenderFn render);

private:
    ///
    /// Tick ごとの表示更新.
    ///
    void OnTick();

    MainModel* model_;
    RenderFn render_;

    ///
    /// GetResult() のコピー先バッファ.
    ///
    PipelineResult result_;

    ///
    /// 描画済みの推論結果 sequence.
    ///
    std::uint64_t last_sequence_ = 0;
};
