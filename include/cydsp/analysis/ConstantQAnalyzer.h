#pragma once
#include "cydsp/core/AudioBlock.h"
#include "cydsp/core/Cpu.h"
#include <cstdint>
#include <memory>
namespace cydsp {
struct ConstantQConfig {
    double minimumHz = 55;
    unsigned binsPerOctave = 12;
    std::size_t bins = 96, hopFrames = 512, maximumKernelFrames = 131072;
};
// Direct constant-Q analysis: variable-length Hann-windowed complex projections.
// All kernels are prepared; vector dots use double precision. Frequency bins share
// an end timestamp, with different window lengths and therefore different delays.
template <Sample T> class ConstantQAnalyzer {
  public:
    ConstantQAnalyzer();
    ~ConstantQAnalyzer();
    ConstantQAnalyzer(ConstantQAnalyzer&&) noexcept;
    ConstantQAnalyzer& operator=(ConstantQAnalyzer&&) noexcept;
    [[nodiscard]] Status prepare(const ProcessSpec&, const ConstantQConfig& = {},
                                 CpuBackend = CpuBackend::automatic) noexcept;
    [[nodiscard]] Status process(AudioBlock<const T>) noexcept;
    void reset() noexcept;
    [[nodiscard]] bool hasFrame() const noexcept;
    [[nodiscard]] std::uint64_t frameCount() const noexcept;
    [[nodiscard]] std::span<const double> centerFrequencies() const noexcept;
    [[nodiscard]] std::span<const double> magnitudes(std::size_t channel) const noexcept;
    [[nodiscard]] std::size_t windowFrames(std::size_t bin) const noexcept;
    [[nodiscard]] CpuBackend backend() const noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
extern template class ConstantQAnalyzer<float>;
extern template class ConstantQAnalyzer<double>;
} // namespace cydsp
