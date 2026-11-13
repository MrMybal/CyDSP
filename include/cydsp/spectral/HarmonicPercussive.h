#pragma once
#include "cydsp/spectral/Stft.h"
namespace cydsp {
struct HpssConfig {
    StftConfig stft{1024, 256, WindowType::hann};
    std::size_t temporalFrames = 17, frequencyBins = 17;
};
// Centered median masks linked across input channels. Harmonic + percussive is
// the delayed input (complex complementary masks), not an independent-source
// guarantee. Lookahead = temporalFrames/2 hops, included in latencyFrames().
// Drain by feeding latencyFrames()+2*N zeros; reset starts a new stream.
template <Sample T> class HarmonicPercussive {
  public:
    HarmonicPercussive();
    ~HarmonicPercussive();
    HarmonicPercussive(HarmonicPercussive&&) noexcept;
    HarmonicPercussive& operator=(HarmonicPercussive&&) noexcept;
    [[nodiscard]] Status prepare(const ProcessSpec&, const HpssConfig& = {},
                                 CpuBackend = CpuBackend::automatic) noexcept;
    [[nodiscard]] Status process(AudioBlock<const T>, AudioBlock<T> harmonic,
                                 AudioBlock<T> percussive) noexcept;
    void reset() noexcept;
    [[nodiscard]] std::size_t latencyFrames() const noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
extern template class HarmonicPercussive<float>;
extern template class HarmonicPercussive<double>;
} // namespace cydsp
