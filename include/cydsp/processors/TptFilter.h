#pragma once
#include "cydsp/core/AudioBlock.h"
#include "cydsp/processors/Biquad.h"
#include <memory>
#include <span>
namespace cydsp {
enum class TptFilterMode {
    lowPass,
    bandPass,
    highPass,
    notch,
    allPass,
    firstOrderLowPass,
    firstOrderHighPass,
    firstOrderAllPass
};
template <Sample T> class TptFilter {
  public:
    TptFilter();
    ~TptFilter();
    TptFilter(TptFilter&&) noexcept;
    TptFilter& operator=(TptFilter&&) noexcept;
    [[nodiscard]] Status prepare(const ProcessSpec&) noexcept;
    [[nodiscard]] Status setParameters(double frequency, double q, TptFilterMode) noexcept;
    [[nodiscard]] Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
    void reset() noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
extern template class TptFilter<float>;
extern template class TptFilter<double>;
template <Sample T> class LinkwitzRiley {
  public:
    LinkwitzRiley();
    ~LinkwitzRiley();
    LinkwitzRiley(LinkwitzRiley&&) noexcept;
    LinkwitzRiley& operator=(LinkwitzRiley&&) noexcept;
    [[nodiscard]] Status prepare(const ProcessSpec&) noexcept;
    [[nodiscard]] Status setFrequency(double frequency) noexcept;
    [[nodiscard]] Status process(AudioBlock<const T>, AudioBlock<T> low,
                                 AudioBlock<T> high) noexcept;
    void reset() noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
extern template class LinkwitzRiley<float>;
extern template class LinkwitzRiley<double>;
template <Sample T> class LadderFilter {
  public:
    LadderFilter();
    ~LadderFilter();
    LadderFilter(LadderFilter&&) noexcept;
    LadderFilter& operator=(LadderFilter&&) noexcept;
    [[nodiscard]] Status prepare(const ProcessSpec&) noexcept;
    [[nodiscard]] Status setParameters(double frequency, double resonance,
                                       double drive = 1) noexcept;
    [[nodiscard]] Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
    void reset() noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
extern template class LadderFilter<float>;
extern template class LadderFilter<double>;
// Non-real-time coefficient design; output is committed only for valid inputs.
[[nodiscard]] Status designButterworth(BiquadType, unsigned order, double sampleRate, double cutoff,
                                       std::span<BiquadCoefficients> sections,
                                       std::size_t& count) noexcept;
[[nodiscard]] Status designWindowedFir(std::span<double> taps, double normalizedCutoff,
                                       bool highPass = false, double attenuationDb = 100) noexcept;
} // namespace cydsp
