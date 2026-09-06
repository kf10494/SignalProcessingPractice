///
/// @file InferResultPresenter.cpp
///

#include "presenter/InferResultPresenter.h"

#include <utility>

#include <etl/variant.h>

#include "common/InferResult.hpp"
#include "common/KeywordSpottingResult.hpp"
#include "model/MainModel.h"

InferResultPresenter::InferResultPresenter(MainModel* model, const TickRegistrar& registrar,
                                           RenderFn render)
    : model_(model),
      render_(std::move(render))
{
    registrar([this] {
        OnTick();
    });
}

void InferResultPresenter::OnTick()
{
    model_->Process().GetResult(&result_);

    const auto* keyword = etl::get_if<KeywordSpottingResult>(&result_.infer_result);
    if (keyword == nullptr || keyword->sequence == last_sequence_) {
        return;
    }
    last_sequence_ = keyword->sequence;
    render_(keyword_spotting_label_name(keyword->top_index), keyword->top_confidence);
}
