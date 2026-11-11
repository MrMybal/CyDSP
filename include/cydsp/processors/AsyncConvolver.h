#pragma once
#include "cydsp/processors/Convolver.h"
#include <array>
#include <atomic>
namespace cydsp {
// One producer calls stageImpulse on a worker; one consumer calls process on audio.
// Three retained slots provide bounded publication without locks or audio-thread frees.
template <Sample T>
    requires(!std::is_const_v<T>)
class AsyncConvolver {
  public:
    AsyncConvolver() = default;
    AsyncConvolver(const AsyncConvolver&) = delete;
    AsyncConvolver& operator=(const AsyncConvolver&) = delete;
    AsyncConvolver(AsyncConvolver&&) = delete;
    AsyncConvolver& operator=(AsyncConvolver&&) = delete;
    // prepare/reset/destruction require BOTH producer and consumer to be quiescent.
    [[nodiscard]] Status prepare(const ProcessSpec&, std::span<const T>,
                                 std::size_t partition = 128,
                                 CpuBackend = CpuBackend::automatic) noexcept;
    // Non-real-time producer. May allocate/free; can run concurrently with process.
    // If all slots are occupied, returns update_queue_full without changing pending IR.
    [[nodiscard]] Status stageImpulse(std::span<const T>, std::size_t transitionFrames = 256,
                                      bool prewarm = true) noexcept;
    // Quiescent preparation selects the engine for every worker-prepared IR.
    [[nodiscard]] Status prepare(const ProcessSpec&, std::span<const T>, const ConvolutionConfig&,
                                 CpuBackend = CpuBackend::automatic) noexcept;
    [[nodiscard]] ConvolutionConfig configuration() const noexcept {
        return config_;
    }
    void reset() noexcept;
    [[nodiscard]] Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
    [[nodiscard]] bool hasPendingImpulse() const noexcept {
        return pending_.load(std::memory_order_acquire) != none;
    }
    // These consumer-side observers must be serialized with process.
    [[nodiscard]] bool isTransitioning() const noexcept {
        return warming_ != 0 || remaining_ != 0;
    }
    [[nodiscard]] std::size_t warmupRemainingFrames() const noexcept {
        return warming_;
    }
    [[nodiscard]] std::size_t tailFrames() const noexcept;
    [[nodiscard]] std::size_t latencyFrames() const noexcept {
        return 0;
    }

  private:
    static constexpr unsigned none = 3;
    static_assert(std::atomic<unsigned>::is_always_lock_free);
    void adopt(unsigned) noexcept;
    ProcessSpec spec_{};
    ConvolutionConfig config_{};
    std::array<Convolver<T>, 3> banks_;
    std::array<AudioBuffer<T>, 2> scratch_;
    std::array<std::atomic<unsigned>, 3> occupied_{};
    std::array<std::size_t, 3> durations_{}, warmups_{};
    std::atomic<unsigned> pending_{none};
    unsigned active_ = 0, destination_ = none;
    std::size_t tile_ = 0, remaining_ = 0, warming_ = 0;
    CpuBackend backend_ = CpuBackend::scalar;
    double blend_ = 0;
};
extern template class AsyncConvolver<float>;
extern template class AsyncConvolver<double>;
} // namespace cydsp
