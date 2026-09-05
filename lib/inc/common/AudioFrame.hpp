///
/// @file AudioFrame.hpp
///
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

///
/// @brief 音声サンプルを保持する値型コンテナ.
///
/// サンプル数を非型テンプレート引数で固定し, 実体は std::array としてスタック上に確保する.
/// メンバが std::array と組み込み型のみのため, 特殊メンバ関数はすべてコンパイラ生成の
/// デフォルトで機能する (Rule of Zero).
///
/// @tparam NumSamples 1 フレーム当たりのサンプル数.
/// @tparam SampleType サンプルの型 (既定は float).
///
template <std::size_t NumSamples, typename SampleType = float>
class AudioFrameTemplate {
public:
    ///
    /// 既定のサンプルレート.
    ///
    static constexpr std::uint32_t kDefaultSampleRate = 44100U;

    ///
    /// @name ctor, dtor.
    /// @{
    constexpr explicit AudioFrameTemplate(std::uint32_t sample_rate = kDefaultSampleRate) noexcept
        : sample_rate_{sample_rate}
    {
    }

    AudioFrameTemplate(const AudioFrameTemplate&) = default;
    auto operator=(const AudioFrameTemplate&) -> AudioFrameTemplate& = default;
    AudioFrameTemplate(AudioFrameTemplate&&) = default;
    auto operator=(AudioFrameTemplate&&) -> AudioFrameTemplate& = default;
    ~AudioFrameTemplate() = default;
    /// @}

    ///
    /// @name プロパティ.
    /// @{
    [[nodiscard]] constexpr auto sample_rate() const noexcept -> std::uint32_t
    {
        return sample_rate_;
    }
    constexpr auto set_sample_rate(std::uint32_t rate) noexcept -> void
    {
        sample_rate_ = rate;
    }
    [[nodiscard]] constexpr auto size() const noexcept -> std::size_t
    {
        return NumSamples;
    }
    /// @}

    ///
    /// @name 要素アクセス.
    /// @{
    constexpr auto operator[](std::size_t index) -> SampleType&
    {
        return data_.at(index);
    }
    constexpr auto operator[](std::size_t index) const -> const SampleType&
    {
        return data_.at(index);
    }
    [[nodiscard]] constexpr auto data() noexcept -> SampleType*
    {
        return data_.data();
    }
    [[nodiscard]] constexpr auto data() const noexcept -> const SampleType*
    {
        return data_.data();
    }
    /// @}

    ///
    /// @name イテレータ (標準アルゴリズムとの連携用).
    /// @{
    [[nodiscard]] constexpr auto begin() noexcept
    {
        return data_.begin();
    }
    [[nodiscard]] constexpr auto end() noexcept
    {
        return data_.end();
    }
    [[nodiscard]] constexpr auto begin() const noexcept
    {
        return data_.begin();
    }
    [[nodiscard]] constexpr auto end() const noexcept
    {
        return data_.end();
    }
    /// @}

    ///
    /// @name 比較演算子.
    ///
    /// サンプルレートと全サンプルが一致するかで等価性を判定する.
    /// @{
    friend constexpr auto operator==(const AudioFrameTemplate& lhs,
                                     const AudioFrameTemplate& rhs) noexcept -> bool
    {
        return lhs.sample_rate_ == rhs.sample_rate_ && lhs.data_ == rhs.data_;
    }

    friend constexpr auto operator!=(const AudioFrameTemplate& lhs,
                                     const AudioFrameTemplate& rhs) noexcept -> bool
    {
        return !(lhs == rhs);
    }
    /// @}

private:
    std::uint32_t sample_rate_{kDefaultSampleRate};
    std::array<SampleType, NumSamples> data_{};
};
