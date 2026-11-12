#pragma once
#include "cydsp/core/Types.h"
#include <cassert>
#include <span>
namespace cydsp {
// Non-owning planar view. Pointer table and samples must outlive the view.
template <Sample T> class AudioBlock {
  public:
    constexpr AudioBlock() noexcept = default;
    constexpr AudioBlock(T* const* data, std::size_t channels, std::size_t frames) noexcept
        : data_(data), channels_(channels), frames_(frames) {}
    template <Sample U>
        requires(std::is_const_v<T> && std::same_as<std::remove_const_t<T>, U>)
    constexpr AudioBlock(const AudioBlock<U>& other) noexcept
        : data_(other.channelData()), channels_(other.channels()), frames_(other.frames()),
          offset_(other.frameOffset()) {}
    [[nodiscard]] constexpr std::size_t channels() const noexcept {
        return channels_;
    }
    [[nodiscard]] constexpr std::size_t frames() const noexcept {
        return frames_;
    }
    [[nodiscard]] constexpr std::size_t frameOffset() const noexcept {
        return offset_;
    }
    [[nodiscard]] constexpr AudioBlock subBlock(std::size_t offset,
                                                std::size_t frames) const noexcept {
        if (offset > frames_ || frames > frames_ - offset)
            return {};
        auto result = *this;
        result.offset_ += offset;
        result.frames_ = frames;
        return result;
    }
    // Original pointer table; channel() also applies frameOffset().
    [[nodiscard]] constexpr T* const* channelData() const noexcept {
        return data_;
    }
    [[nodiscard]] constexpr bool isValid() const noexcept {
        if (channels_ && data_ == nullptr)
            return false;
        for (std::size_t ch = 0; ch < channels_; ++ch)
            if (frames_ && data_[ch] == nullptr)
                return false;
        return true;
    }
    [[nodiscard]] constexpr std::span<T> channel(std::size_t index) const noexcept {
        assert(index < channels_ && data_ != nullptr);
        return {frames_ == 0 ? nullptr : data_[index] + offset_, frames_};
    }

  private:
    T* const* data_ = nullptr;
    std::size_t channels_ = 0, frames_ = 0, offset_ = 0;
};
template <Sample T>
    requires(!std::is_const_v<T>)
[[nodiscard]] Status validateProcessBlocks(const ProcessSpec&, AudioBlock<const T>,
                                           AudioBlock<T>) noexcept;
// Independently sized blocks: reject every source/destination overlap before any write.
template <Sample T>
[[nodiscard]] Status validateDisjointBlocks(AudioBlock<const T>, AudioBlock<T>) noexcept;
extern template Status validateDisjointBlocks<float>(AudioBlock<const float>,
                                                     AudioBlock<float>) noexcept;
extern template Status validateDisjointBlocks<double>(AudioBlock<const double>,
                                                      AudioBlock<double>) noexcept;
extern template Status validateProcessBlocks<float>(const ProcessSpec&, AudioBlock<const float>,
                                                    AudioBlock<float>) noexcept;
extern template Status validateProcessBlocks<double>(const ProcessSpec&, AudioBlock<const double>,
                                                     AudioBlock<double>) noexcept;
} // namespace cydsp
