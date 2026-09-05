///
/// @file null_strategies.hpp
///
#pragma once

#include "common/FrameSyncProcess.hpp"

struct NullInput {
    static auto Exec() -> FrameSyncProcess::AudioHop;
    static auto Reset() -> void;
};

struct BypassPreProcess {
    static auto Exec(const FrameSyncProcess::AudioHop& frame) -> FrameSyncProcess::AudioHop;
    static auto Reset() -> void;
};

struct BypassPostProcess {
    static auto Exec(const FrameSyncProcess::AudioFrame& frame) -> FrameSyncProcess::AudioFrame;
    static auto Reset() -> void;
};

struct NullOverlap {
    static auto Exec(const FrameSyncProcess::AudioHop& frame) -> FrameSyncProcess::AudioFrame;
    static auto Reset() -> void;
};

struct NullWindow {
    static auto Exec(const FrameSyncProcess::AudioFrame& frame) -> FrameSyncProcess::AudioFrame;
    static auto Reset() -> void;
};

struct BypassFft {
    static auto Exec(const FrameSyncProcess::AudioFrame& frame) -> FrameSyncProcess::AudioFrame;
    static auto Reset() -> void;
};

struct BypassInfer {
    static auto Exec(const FrameSyncProcess::AudioFrame& frame) -> FrameSyncProcess::InferOutput;
    static auto Reset() -> void;
};

struct NullInfer {
    static auto Exec(const FrameSyncProcess::AudioFrame& frame) -> FrameSyncProcess::InferOutput;
    static auto Reset() -> void;
};

struct NullOverlapAdd {
    static auto Exec(const FrameSyncProcess::AudioFrame& frame) -> FrameSyncProcess::AudioHop;
    static auto Reset() -> void;
};

struct NullOutput {
    static auto Exec(const FrameSyncProcess::AudioHop& frame) -> void;
    static auto Reset() -> void;
};
