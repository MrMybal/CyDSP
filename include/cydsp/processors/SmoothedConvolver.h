#pragma once
#include "cydsp/processors/Convolver.h"
#include <array>
namespace cydsp {
// Owner-serialized IR preparation and output crossfades. stageImpulse is NON real-time.
// Plans are retained after fading; process/reset/startTransition never release them.
template <Sample T>
    requires(!std::is_const_v<T>)
class SmoothedConvolver {
  public:
    SmoothedConvolver() = default;
    SmoothedConvolver(const SmoothedConvolver&) = delete;
    SmoothedConvolver& operator=(const SmoothedConvolver&) = delete;
    SmoothedConvolver(SmoothedConvolver&& other) noexcept {
        swap(other);
    }
    SmoothedConvolver& operator=(SmoothedConvolver&& other) noexcept {
        if (this != &other) {
            SmoothedConvolver next(std::move(other));
            swap(next);
        }
        return *this;
    }
    [[nodiscard]] Status prepare(const ProcessSpec&, std::span<const T> impulse,
                                 std::size_t partitionFrames = 128,
                                 CpuBackend = CpuBackend::automatic) noexcept;
    // May allocate/free. Cannot run concurrently with processing or during a fade.
    [[nodiscard]] Status stageImpulse(std::span<const T>) noexcept;
    // No allocation; default prewarms the destination on live input for its IR tail, then fades.
    // prewarm=false retains the cold-history behavior; zero duration switches after prewarming.
    [[nodiscard]] Status startTransition(std::size_t frames = 256, bool prewarm = true) noexcept;
    void discardStagedImpulse() noexcept {
        staged_ = false;
    }
    // Select the engine once for all staged IRs; legacy prepare selects hybrid.
    [[nodiscard]] Status prepare(const ProcessSpec&, std::span<const T>, const ConvolutionConfig&,
                                 CpuBackend = CpuBackend::automatic) noexcept;
    [[nodiscard]] ConvolutionConfig configuration() const noexcept {
        return config_;
    }
    // A running fade snaps to its destination. An unstarted staged plan stays staged.
    void reset() noexcept;
    [[nodiscard]] Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
    [[nodiscard]] bool hasStagedImpulse() const noexcept {
        return staged_;
    }
    [[nodiscard]] bool isTransitioning() const noexcept {
        return warming_ != 0 || remaining_ != 0;
    }
    [[nodiscard]] std::size_t warmupRemainingFrames() const noexcept {
        return warming_;
    }
    [[nodiscard]] std::size_t transitionRemainingFrames() const noexcept {
        return remaining_;
    }
    [[nodiscard]] std::size_t latencyFrames() const noexcept {
        return 0;
    }
    [[nodiscard]] std::size_t tailFrames() const noexcept;
    [[nodiscard]] std::size_t partitionFrames() const noexcept {
        return banks_[active_].partitionFrames();
    }
    [[nodiscard]] CpuBackend backend() const noexcept {
        return banks_[active_].backend();
    }

  private:
    void swap(SmoothedConvolver&) noexcept;
    ProcessSpec spec_{};
    ConvolutionConfig config_{};
    std::array<Convolver<T>, 2> banks_;
    std::array<AudioBuffer<T>, 2> scratch_;
    std::size_t active_ = 0, tileFrames_ = 0, remaining_ = 0, warming_ = 0;
    double blend_ = 0;
    bool staged_ = false;
};
extern template class SmoothedConvolver<float>;
extern template class SmoothedConvolver<double>;
} // namespace cydsp
