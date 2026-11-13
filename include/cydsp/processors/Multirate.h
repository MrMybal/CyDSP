#pragma once
#include "cydsp/core/AudioBlock.h"
#include "cydsp/core/Cpu.h"
#include <memory>
#include <utility>
#include <vector>
namespace cydsp {
enum class RateConversion { decimate, interpolate };
// Fixed integer FIR conversion with a polyphase interpolator and skipped
// decimator output dots. Output length must equal outputFrames(input.frames()).
template <Sample T> class FirRateConverter {
  public:
    FirRateConverter();
    ~FirRateConverter();
    FirRateConverter(FirRateConverter&&) noexcept;
    FirRateConverter& operator=(FirRateConverter&&) noexcept;
    [[nodiscard]] Status prepare(const ProcessSpec& input, unsigned factor, RateConversion,
                                 std::size_t tapsPerPhase = 32,
                                 CpuBackend = CpuBackend::automatic) noexcept;
    [[nodiscard]] std::size_t outputFrames(std::size_t inputFrames) const noexcept;
    [[nodiscard]] Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
    void reset() noexcept;
    [[nodiscard]] double outputSampleRate() const noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
// Critically sampled orthogonal Haar packet bank (2/4/8/16/32 channels per
// source channel). Fast Walsh-Hadamard packet transform, ordered by sequency.
// Exact inverse, coarse frequency separation; use FIR conversion for selective
// anti-alias filtering. Band channel index = band*sourceChannels+sourceChannel.
// Analysis retains an incomplete packet; synthesis emits complete packets.
template <Sample T> class DyadicFilterBank {
  public:
    DyadicFilterBank() = default;
    DyadicFilterBank(const DyadicFilterBank&) = delete;
    DyadicFilterBank& operator=(const DyadicFilterBank&) = delete;
    DyadicFilterBank(DyadicFilterBank&& o) noexcept {
        swap(o);
    }
    DyadicFilterBank& operator=(DyadicFilterBank&& o) noexcept {
        if (this != &o) {
            DyadicFilterBank next(std::move(o));
            swap(next);
        }
        return *this;
    }
    [[nodiscard]] Status prepare(const ProcessSpec&, std::size_t bands = 8) noexcept;
    [[nodiscard]] std::size_t analysisFrames(std::size_t inputFrames) const noexcept;
    [[nodiscard]] Status analyze(AudioBlock<const T>, AudioBlock<T> bands) noexcept;
    [[nodiscard]] Status synthesize(AudioBlock<const T> bands, AudioBlock<T>) const noexcept;
    void reset() noexcept;

  private:
    void swap(DyadicFilterBank& o) noexcept {
        std::swap(spec_, o.spec_);
        packet_.swap(o.packet_);
        std::swap(bands_, o.bands_);
        std::swap(pending_, o.pending_);
    }
    ProcessSpec spec_{};
    std::vector<T> packet_;
    std::size_t bands_ = 0, pending_ = 0;
};
extern template class FirRateConverter<float>;
extern template class FirRateConverter<double>;
extern template class DyadicFilterBank<float>;
extern template class DyadicFilterBank<double>;
} // namespace cydsp
