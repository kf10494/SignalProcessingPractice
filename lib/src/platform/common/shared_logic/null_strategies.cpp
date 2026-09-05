///
/// @file null_strategies.cpp
///
#include "common/Strategies/null_strategies.hpp"

#include "common/FrameSyncProcess.hpp"

auto NullInput::Exec() -> FrameSyncProcess::AudioHop
{
    return FrameSyncProcess::AudioHop{};
}
auto NullInput::Reset() -> void
{
}

auto BypassPreProcess::Exec(const FrameSyncProcess::AudioHop& frame) -> FrameSyncProcess::AudioHop
{
    return frame;
}
auto BypassPreProcess::Reset() -> void
{
}

auto BypassPostProcess::Exec(const FrameSyncProcess::AudioFrame& frame)
        -> FrameSyncProcess::AudioFrame
{
    return frame;
}
auto BypassPostProcess::Reset() -> void
{
}

auto NullOverlap::Exec([[maybe_unused]] const FrameSyncProcess::AudioHop& frame)
        -> FrameSyncProcess::AudioFrame
{
    return FrameSyncProcess::AudioFrame{};
}
auto NullOverlap::Reset() -> void
{
}

auto NullWindow::Exec(const FrameSyncProcess::AudioFrame& frame) -> FrameSyncProcess::AudioFrame
{
    return frame;
}
auto NullWindow::Reset() -> void
{
}

auto BypassFft::Exec(const FrameSyncProcess::AudioFrame& frame) -> FrameSyncProcess::AudioFrame
{
    return frame;
}
auto BypassFft::Reset() -> void
{
}

auto BypassInfer::Exec(const FrameSyncProcess::AudioFrame& frame) -> FrameSyncProcess::InferOutput
{
    return FrameSyncProcess::InferOutput{frame, InferResult{NoInferResult{}}};
}
auto BypassInfer::Reset() -> void
{
}

auto NullInfer::Exec([[maybe_unused]] const FrameSyncProcess::AudioFrame& frame)
        -> FrameSyncProcess::InferOutput
{
    return FrameSyncProcess::InferOutput{FrameSyncProcess::AudioFrame{},
                                         InferResult{NoInferResult{}}};
}
auto NullInfer::Reset() -> void
{
}

auto NullOverlapAdd::Exec([[maybe_unused]] const FrameSyncProcess::AudioFrame& frame)
        -> FrameSyncProcess::AudioHop
{
    return FrameSyncProcess::AudioHop{};
}
auto NullOverlapAdd::Reset() -> void
{
}

auto NullOutput::Exec([[maybe_unused]] const FrameSyncProcess::AudioHop& frame) -> void
{
}
auto NullOutput::Reset() -> void
{
}
