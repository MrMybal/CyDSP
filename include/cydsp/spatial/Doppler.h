#pragma once
#include "cydsp/processors/Resampler.h"
#include "cydsp/spatial/SpatialMath.h"
#include <cmath>
namespace cydsp {
// Doppler changes source playback rate; callers must retain unconsumed input and
// route only produced frames. Propagation delay is a separate, pitch-preserving
// crossfade (MultiTapDelay): do not also apply a continuously gliding path delay.
template <Sample T> class DopplerResampler {
  public:
    [[nodiscard]] Status prepare(const ResampleSpec& spec, double maximumSemitones = 24,
                                 CpuBackend backend = CpuBackend::automatic) noexcept {
        if (!isFinite(maximumSemitones) || maximumSemitones < 0 || maximumSemitones > 48 ||
            spec.inputRate != spec.outputRate)
            return Status::invalid_spec;
        auto config = spec;
        const double maximum = std::exp2(maximumSemitones / 12);
        config.minimumRatio = 1 / maximum;
        config.maximumRatio = maximum;
        config = resampleSpecForQuality(config, ResampleQuality::high);
        auto status = resampler_.prepare(config, backend);
        if (status == Status::ok)
            limit_ = maximumSemitones;
        return status;
    }
    [[nodiscard]] Status setMotion(const RelativeSource& source, const AcousticMedium& medium,
                                   std::size_t rampFrames = 0, double intensity = 1) noexcept {
        double ratio = 1;
        auto status = dopplerRatio(source, medium, ratio, intensity, limit_);
        if (status != Status::ok)
            return status;
        return resampler_.setRatio(ratio, rampFrames);
    }
    [[nodiscard]] ResampleResult process(AudioBlock<const T> input, AudioBlock<T> output) noexcept {
        return resampler_.process(input, output);
    }
    [[nodiscard]] ResampleResult finish(AudioBlock<T> output) noexcept {
        return resampler_.finish(output);
    }
    void reset() noexcept {
        resampler_.reset();
    }
    [[nodiscard]] double ratio() const noexcept {
        return resampler_.ratio();
    }
    [[nodiscard]] double latencyInputFrames() const noexcept {
        return resampler_.latencyInputFrames();
    }

  private:
    Resampler<T> resampler_;
    double limit_ = 24;
};
} // namespace cydsp
