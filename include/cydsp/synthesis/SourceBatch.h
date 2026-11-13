#pragma once
#include "cydsp/spatial/SpatialMath.h"
#include "cydsp/synthesis/SampleData.h"
#include "cydsp/synthesis/Wavetable.h"
#include <array>
namespace cydsp {
enum class BatchSourceKind { sine, wavetable, grain };
struct BatchSource {
    BatchSourceKind kind = BatchSourceKind::sine;
    std::size_t asset = 0;
    double frequencyHz = 440, gain = 1, phase = 0;
    double samplePosition = 0, playbackRatio = 1;
    std::size_t durationFrames = 4800, startOffsetFrames = 0;
    std::array<double, 64> channelGains{1};
};
// Fixed voice pool. Assets are borrowed immutable prepared banks, shared without
// copies; the host retains them and serializes their destruction/preparation.
// Sine recurrences use SoA vector kernels across voices; grains/tables use their
// existing antialiased banks. Routing/mixing is SIMD across time. No voice stealing.
template <Sample T> class SourceBatch {
  public:
    SourceBatch();
    ~SourceBatch();
    SourceBatch(SourceBatch&&) noexcept;
    SourceBatch& operator=(SourceBatch&&) noexcept;
    [[nodiscard]] Status prepare(const ProcessSpec& output, std::size_t maximumVoices,
                                 std::span<const Wavetable<T>* const> tables = {},
                                 std::span<const SampleData<T>* const> samples = {},
                                 CpuBackend = CpuBackend::automatic) noexcept;
    [[nodiscard]] Status start(std::size_t slot, const BatchSource&) noexcept;
    [[nodiscard]] Status stop(std::size_t slot) noexcept;
    // Stereo equal-power pan + inverse distance, or ACN/SN3D if order 0..7 and
    // output channels == (order+1)^2. Position is listener-local, +X front.
    [[nodiscard]] Status setPosition(std::size_t slot, Vec3, int ambisonicOrder = -1) noexcept;
    [[nodiscard]] Status process(AudioBlock<T>) noexcept;
    void reset() noexcept;
    [[nodiscard]] std::size_t activeVoices() const noexcept;
    [[nodiscard]] CpuBackend backend() const noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
extern template class SourceBatch<float>;
extern template class SourceBatch<double>;
} // namespace cydsp
