#pragma once
#include "cydsp/core/AudioBlock.h"
#include "cydsp/core/Cpu.h"
#include <array>
#include <memory>
namespace cydsp {
struct FdnConfig {
    std::size_t lines = 16;
    double roomScale = 1, maximumPreDelayMs = 250;
};
struct FdnParameters {
    std::array<double, 3> decaySeconds{2, 1.5, 1};
    double preDelayMs = 10, diffusion = .7, wet = 1, dry = 0;
    bool freeze = false;
};
// Orthogonal Hadamard feedback network, 4/8/16/32 lines, 1..min(lines,8)
// channels. Band decays are nominal; conservative feedback bounds keep arbitrary
// low/mid/high profiles passive. Freeze removes injection and damping.
template <Sample T> class FdnReverb {
  public:
    FdnReverb();
    ~FdnReverb();
    FdnReverb(FdnReverb&&) noexcept;
    FdnReverb& operator=(FdnReverb&&) noexcept;
    [[nodiscard]] Status prepare(const ProcessSpec&, const FdnConfig& = {},
                                 CpuBackend = CpuBackend::automatic) noexcept;
    [[nodiscard]] Status setParameters(const FdnParameters&, std::size_t rampFrames = 256) noexcept;
    [[nodiscard]] Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
    void reset() noexcept;
    [[nodiscard]] std::size_t nominalTailFrames() const noexcept;
    [[nodiscard]] CpuBackend backend() const noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
extern template class FdnReverb<float>;
extern template class FdnReverb<double>;
} // namespace cydsp
