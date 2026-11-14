#pragma once
#include "cydsp/processors/NonuniformConvolver.h"
#include "cydsp/processors/ScheduledConvolver.h"
#include <variant>
namespace cydsp {
enum class ConvolutionMode { hybrid, nonuniform, scheduled };
struct ConvolutionConfig {
    ConvolutionMode mode = ConvolutionMode::hybrid;
    std::size_t earlyPartition = 128;
    // Used by nonuniform and scheduled modes; ignored by hybrid.
    std::size_t latePartition = 1024;
    // MatrixConvolver only. When a process call still holds a whole early
    // partition at a partition boundary, the first earlyPartition taps are
    // convolved in the frequency domain with the early tier's own transforms
    // instead of the direct FIR. Latency stays zero. That output differs from
    // the direct sum by FFT rounding, so it depends on the host block pattern.
    bool spectralHead = false;
    bool operator==(const ConvolutionConfig&) const = default;
};
// Common zero-added-block-latency interface. Engine selection happens during prepare.
// Failed preparation preserves the complete previous plan and signal history.
template <Sample T>
    requires(!std::is_const_v<T>)
class Convolver {
  public:
    Convolver() = default;
    Convolver(const Convolver&) = delete;
    Convolver& operator=(const Convolver&) = delete;
    Convolver(Convolver&& other) noexcept {
        swap(other);
    }
    Convolver& operator=(Convolver&& other) noexcept {
        if (this != &other) {
            Convolver next(std::move(other));
            swap(next);
        }
        return *this;
    }
    [[nodiscard]] Status prepare(const ProcessSpec&, std::span<const T>,
                                 const ConvolutionConfig& = {},
                                 CpuBackend = CpuBackend::automatic) noexcept;
    void reset() noexcept;
    [[nodiscard]] Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
    [[nodiscard]] ConvolutionConfig configuration() const noexcept {
        return config_;
    }
    [[nodiscard]] std::size_t latencyFrames() const noexcept {
        return 0;
    }
    [[nodiscard]] std::size_t tailFrames() const noexcept;
    [[nodiscard]] CpuBackend backend() const noexcept;
    [[nodiscard]] std::size_t partitionFrames() const noexcept {
        return prepared_ ? config_.earlyPartition : 0;
    }

  private:
    void swap(Convolver&) noexcept;
    std::variant<HybridConvolver<T>, NonuniformConvolver<T>, ScheduledConvolver<T>> engine_;
    ConvolutionConfig config_{};
    bool prepared_ = false;
};
extern template class Convolver<float>;
extern template class Convolver<double>;
} // namespace cydsp
