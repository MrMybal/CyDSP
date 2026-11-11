#pragma once
#include "cydsp/core/AudioBlock.h"
#include <algorithm>
#include <memory>
#include <new>
#include <stdexcept>
#include <utility>
#include <vector>
namespace cydsp {
// Owning 64-byte-aligned planar storage. resize is non-real-time.
template <Sample T>
    requires(!std::is_const_v<T>)
class AudioBuffer {
    struct AlignedDelete {
        void operator()(T* p) const noexcept {
            ::operator delete[](p, std::align_val_t{64});
        }
    };

  public:
    AudioBuffer() = default;
    AudioBuffer(const AudioBuffer&) = delete;
    AudioBuffer& operator=(const AudioBuffer&) = delete;
    AudioBuffer(AudioBuffer&& other) noexcept {
        swap(other);
    }
    AudioBuffer& operator=(AudioBuffer&& other) noexcept {
        if (this != &other) {
            AudioBuffer next(std::move(other));
            swap(next);
        }
        return *this;
    }
    [[nodiscard]] Status resize(std::size_t channels, std::size_t frames) noexcept {
        if (!channels || !frames)
            return Status::invalid_argument;
        constexpr auto lane = std::size_t{64} / sizeof(T);
        constexpr auto limit = std::numeric_limits<std::size_t>::max();
        if (frames > limit - (lane - 1))
            return Status::invalid_argument;
        const auto stride = ((frames + lane - 1) / lane) * lane;
        if (stride > limit / sizeof(T) / channels)
            return Status::invalid_argument;
        try {
            AudioBuffer next;
            next.data_.reset(static_cast<T*>(
                ::operator new[](stride * channels * sizeof(T), std::align_val_t{64})));
            next.pointers_.resize(channels);
            std::fill_n(next.data_.get(), stride * channels, T{});
            for (std::size_t ch = 0; ch < channels; ++ch)
                next.pointers_[ch] = next.data_.get() + ch * stride;
            next.frames_ = frames;
            swap(next);
            return Status::ok;
        } catch (const std::bad_alloc&) {
            return Status::allocation_failed;
        } catch (const std::length_error&) {
            return Status::invalid_argument;
        }
    }
    void swap(AudioBuffer& other) noexcept {
        data_.swap(other.data_);
        pointers_.swap(other.pointers_);
        std::swap(frames_, other.frames_);
    }
    [[nodiscard]] AudioBlock<T> block() noexcept {
        return {pointers_.data(), pointers_.size(), frames_};
    }
    [[nodiscard]] AudioBlock<const T> block() const noexcept {
        return {pointers_.data(), pointers_.size(), frames_};
    }
    [[nodiscard]] AudioBlock<T> block(std::size_t frames) noexcept {
        if (frames > frames_)
            return {};
        return {pointers_.data(), pointers_.size(), frames};
    }

  private:
    std::unique_ptr<T[], AlignedDelete> data_;
    std::vector<T*> pointers_;
    std::size_t frames_ = 0;
};
} // namespace cydsp
