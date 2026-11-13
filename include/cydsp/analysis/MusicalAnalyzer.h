#pragma once
#include "cydsp/core/AudioBlock.h"
#include "cydsp/core/Cpu.h"
#include <array>
#include <cstdint>
#include <memory>
namespace cydsp {
struct MusicalAnalysisConfig {
    std::size_t fftFrames = 2048, hopFrames = 256, melBands = 40, mfccCount = 13;
    double minimumPitchHz = 60, maximumPitchHz = 2000, onsetThreshold = 1.5, minimumBpm = 50,
           maximumBpm = 220;
};
struct MusicalFeatures {
    double fundamentalHz = 0, pitchConfidence = 0, onsetStrength = 0, tempoBpm = 0,
           beatConfidence = 0, beatPhase = 0, keyConfidence = 0;
    bool onset = false, keyMinor = false;
    unsigned keyPitchClass = 0; // 0=C, ... 11=B. Heuristic, not polyphonic transcription.
    std::array<double, 12> chroma{};
    std::array<double, 32> mfcc{};
    std::uint64_t frameEnd = 0;
};
// FFT-accelerated YIN difference with a fixed N/2 comparison window. Pitch is
// monophonic. Analysis starts after N captured frames; onset/beat and key are
// heuristic estimates. Configure N large enough for the requested low pitch.
template <Sample T> class MusicalAnalyzer {
  public:
    using Callback = void (*)(void*, const MusicalFeatures&) noexcept;
    MusicalAnalyzer();
    ~MusicalAnalyzer();
    MusicalAnalyzer(MusicalAnalyzer&&) noexcept;
    MusicalAnalyzer& operator=(MusicalAnalyzer&&) noexcept;
    [[nodiscard]] Status prepare(const ProcessSpec&, const MusicalAnalysisConfig& = {},
                                 CpuBackend = CpuBackend::automatic) noexcept;
    void setCallback(Callback, void* context = nullptr) noexcept;
    [[nodiscard]] Status process(AudioBlock<const T>) noexcept;
    [[nodiscard]] MusicalFeatures features() const noexcept;
    void reset() noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
extern template class MusicalAnalyzer<float>;
extern template class MusicalAnalyzer<double>;
} // namespace cydsp
