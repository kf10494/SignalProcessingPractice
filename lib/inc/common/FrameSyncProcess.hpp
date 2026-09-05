///
/// @file FrameSyncProcess.hpp
///
#pragma once

#include <cstddef>

#include <etl/delegate.h>

#include "common/AudioFrame.hpp"
#include "common/InferResult.hpp"
#include "common/StrategySlot.hpp"

///
/// 前方宣言.
///
/// TODO: FrameSyncProcess.hpp と FrameSyncProcessConfig.hpp で相互参照が発生している.
///       現状は FrameSyncProcessConfig の前方宣言によってコンパイルエラーを一時的に
///       回避しているが, 将来的には依存関係の解消を検討する.
///
struct FrameSyncProcessConfig;
struct PipelineResult;

namespace frame_sync_detail {

///
/// CMSIS-DSP (arm_rfft_fast_init_f32) が対応する FFT 長の範囲.
///
inline constexpr std::size_t kMinSupportedFftLength = 32;
inline constexpr std::size_t kMaxSupportedFftLength = 4096;

///
/// @brief CMSIS-DSP (arm_rfft_fast_init_f32) が対応する FFT 長かどうかを判定する.
///
[[nodiscard]] constexpr auto IsSupportedFftLength(std::size_t length) noexcept -> bool
{
    return length >= kMinSupportedFftLength && length <= kMaxSupportedFftLength &&
           (length & (length - 1)) == 0;
}

}  // namespace frame_sync_detail

class FrameSyncProcess {
public:
    ///
    /// @name オーディオフレーム.
    /// {@

    ///
    /// オーディオフレームの 1 フレーム辺りのサンプル数.
    ///
    static constexpr std::size_t audio_frame_length = 1024;
    static constexpr std::size_t audio_hop_length = 512;

    static_assert(frame_sync_detail::IsSupportedFftLength(audio_frame_length),
                  "audio_frame_length は CMSIS-DSP (arm_rfft_fast_init_f32) が対応するサイズ "
                  "(32/64/128/256/512/1024/2048/4096) である必要があります.");
    static_assert(audio_hop_length > 0 && audio_hop_length <= audio_frame_length,
                  "audio_hop_length は 1 以上 audio_frame_length 以下である必要があります.");

    ///
    /// オーディオホップ.
    ///
    using AudioHop = AudioFrameTemplate<audio_hop_length, float>;

    ///
    /// オーディオフレーム.
    ///
    using AudioFrame = AudioFrameTemplate<audio_frame_length, float>;

    /// @}

    ///
    /// @name タグ型.
    /// {@
    template <typename T>
    struct tag_t {
        explicit tag_t() = default;
    };

    struct Acquire {};
    struct PreProcess {};
    struct Overlap {};
    struct Window {};
    struct Fft {};
    struct Infer {};
    struct PostProcess {};
    struct OverlapAdd {};
    struct Output {};

    using AcquireTag = tag_t<Acquire>;
    using PreProcessTag = tag_t<PreProcess>;
    using OverlapTag = tag_t<Overlap>;
    using WindowTag = tag_t<Window>;
    using FftTag = tag_t<Fft>;
    using InferTag = tag_t<Infer>;
    using PostProcessTag = tag_t<PostProcess>;
    using OverlapAddTag = tag_t<OverlapAdd>;
    using OutputTag = tag_t<Output>;
    /// @}

    ///
    /// @name Strategy 型.
    /// {@
    ///
    /// オーディオフレーム獲得.
    ///
    using AudioAcquireStrategy = StrategySlot<AudioHop()>;

    ///
    /// オーディオ前処理.
    ///
    using PreProcessStrategy = StrategySlot<AudioHop(const AudioHop &)>;

    ///
    /// オーバーラッピング.
    ///
    using OverlapStrategy = StrategySlot<AudioFrame(const AudioHop &)>;

    ///
    /// 窓関数の積算.
    ///
    using WindowStrategy = StrategySlot<AudioFrame(const AudioFrame &)>;

    ///
    /// FFT.
    ///
    using FftStrategy = StrategySlot<AudioFrame(const AudioFrame &)>;

    ///
    /// 推論.
    ///
    /// @note Infer Strategy の出力は, 音声フレームと型付き推論結果 (InferResult) の組とする.
    ///       frame は, 何らかの時間軸または周波数軸の推論結果を返すモデルであれば有効なフレームを, そうでなければ空フレームを返す.
    ///       result は, キーワード識別・声質変換など推論ドメインごとの結果を InferResult に格納したものを返す.
    ///
    struct InferOutput {
        AudioFrame frame;
        InferResult result;
    };
    using InferStrategy = StrategySlot<InferOutput(const AudioFrame &)>;

    ///
    /// オーディオ後処理.
    ///
    using PostProcessStrategy = StrategySlot<AudioFrame(const AudioFrame &)>;

    ///
    /// Overlap-Add.
    ///
    using OverlapAddStrategy = StrategySlot<AudioHop(const AudioFrame &)>;

    ///
    /// オーディオ出力.
    ///
    using AudioOutputStrategy = StrategySlot<void(const AudioHop &)>;
    /// @}

    ///
    /// @name Observer 型.
    /// {@
    ///
    /// ProcessFrame() 完了時の通知コールバック.
    ///
    using ProcessCompleteObserver = etl::delegate<void(const PipelineResult &)>;
    /// @}

    ///
    /// @name ctor, dtor.
    /// {@
    ///
    /// 既定 Strategy 構成 (FrameSyncProcessConfig の既定値) で構築する.
    ///
    FrameSyncProcess();
    explicit FrameSyncProcess(const FrameSyncProcessConfig &config);
    ~FrameSyncProcess();

    FrameSyncProcess(FrameSyncProcess &&other) noexcept;
    auto operator=(FrameSyncProcess &&other) noexcept -> FrameSyncProcess &;

    FrameSyncProcess(const FrameSyncProcess &) = delete;
    auto operator=(const FrameSyncProcess &) -> FrameSyncProcess & = delete;
    /// @}

    ///
    /// @name 公開関数.
    /// {@
    ///
    /// Observer 登録.
    ///
    void Attach(ProcessCompleteObserver delegate);

    ///
    /// Observer 解除.
    ///
    void Detach(ProcessCompleteObserver delegate);

    ///
    /// 直近の信号処理結果をスナップショットとして取得する.
    ///
    /// スレッドセーフ（SeqLock による読み取り保護）. ProcessFrame() と並行呼び出し可能.
    /// out には呼び出し側で確保した PipelineResult を渡すこと.
    ///
    void GetResult(PipelineResult *out) const;

    ///
    /// 処理設定.
    ///
    void SetConfig(AcquireTag tag, AudioAcquireStrategy strategy);
    void SetConfig(PreProcessTag tag, PreProcessStrategy strategy);
    void SetConfig(OverlapTag tag, OverlapStrategy strategy);
    void SetConfig(WindowTag tag, WindowStrategy strategy);
    void SetConfig(FftTag tag, FftStrategy strategy);
    void SetConfig(InferTag tag, InferStrategy strategy);
    void SetConfig(PostProcessTag tag, PostProcessStrategy strategy);
    void SetConfig(OverlapAddTag tag, OverlapAddStrategy strategy);
    void SetConfig(OutputTag tag, AudioOutputStrategy strategy);

    ///
    /// 1 フレーム分の処理を実行.
    ///
    void ProcessFrame();
    /// @}

    ///
    /// Impl 前方宣言.
    ///
    struct Impl;

private:
    ///
    /// Impl サイズの上限.
    ///
    static constexpr std::size_t kImplSize = 32768;  // 32KB
    static constexpr std::size_t kImplAlign = alignof(std::max_align_t);

    alignas(kImplAlign) std::array<std::byte, kImplSize> storage_;

    ///
    /// Impl へのキャスト用.
    ///
    [[nodiscard]] auto ImplPtr() -> Impl *;
    [[nodiscard]] auto ImplPtr() const -> const Impl *;

    ///
    /// @brief Impl サイズのコンパイル時検証.
    ///
    static void CheckImpl();
};
