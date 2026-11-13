#pragma once
#include "cydsp/core/AudioBlock.h"
#include <cstdint>
#include <memory>
#include <string_view>
namespace cydsp {
enum class ParameterUnit {
    none,
    hertz,
    seconds,
    milliseconds,
    decibels,
    linearGain,
    semitones,
    metres,
    degrees
};
enum class ParameterScale { linear, logarithmic };
struct ParameterDescriptor {
    std::uint32_t id = 0;
    std::string_view name;
    ParameterUnit unit = ParameterUnit::none;
    double minimum = 0, maximum = 1, initial = 0;
    ParameterScale scale = ParameterScale::linear;
    bool modulatable = true;
};
struct ParameterEvent {
    std::uint32_t id = 0;
    std::size_t frameOffset = 0;
    double value = 0;
    std::size_t rampFrames = 0;
};
[[nodiscard]] double parameterToNormalized(const ParameterDescriptor&, double value) noexcept;
[[nodiscard]] double parameterFromNormalized(const ParameterDescriptor&,
                                             double normalized) noexcept;
struct DspTiming {
    std::size_t latencyFrames = 0, tailFrames = 0;
};
// Descriptor strings are copied during preparation. Output channel order matches
// descriptors; events must be sorted by offset. A ramp starts on its event's sample.
class ParameterBank {
  public:
    ParameterBank();
    ~ParameterBank();
    ParameterBank(ParameterBank&&) noexcept;
    ParameterBank& operator=(ParameterBank&&) noexcept;
    [[nodiscard]] Status prepare(std::span<const ParameterDescriptor>,
                                 std::size_t maximumBlockFrames) noexcept;
    [[nodiscard]] Status render(std::span<const ParameterEvent>, AudioBlock<double>) noexcept;
    void reset() noexcept;
    [[nodiscard]] double value(std::uint32_t id) const noexcept;
    [[nodiscard]] std::size_t size() const noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
} // namespace cydsp
