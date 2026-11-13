// Compile-only examples; the application supplies geometry, timing and audio I/O.
#include <array>
#include <cydsp/CyDSP.h>
namespace cydsp::examples {
template <Sample T> class SpatialVoice {
  public:
    Status prepare(const ProcessSpec& mono) noexcept {
        if (mono.channels != 1)
            return Status::channel_mismatch;
        SpatialVoice next;
        auto status = next.paths_.prepare(mono, 4, 1);
        if (status != Status::ok)
            return status;
        auto earsSpec = mono;
        earsSpec.channels = 4;
        status = next.ears_.prepare(earsSpec);
        if (status != Status::ok)
            return status;
        status = next.pathsAudio_.resize(4, mono.maxBlockFrames);
        if (status != Status::ok)
            return status;
        *this = std::move(next);
        return Status::ok;
    }
    Status setPaths(std::span<const AcousticPath> paths) noexcept {
        auto status = paths_.setPaths(paths, 256);
        if (status != Status::ok)
            return status;
        std::array<BinauralEmitter, 4> emitters{};
        for (std::size_t slot = 0; slot < 4; ++slot) {
            const auto path = paths_.path(slot);
            emitters[slot].position = path.id ? path.arrivalDirection : Vec3{1, 0, 0};
        }
        return ears_.setEmitters(emitters, 256);
    }
    Status process(AudioBlock<const T> mono, AudioBlock<T> stereo) noexcept {
        auto paths = pathsAudio_.block(mono.frames());
        auto status = paths_.process(mono, paths);
        return status == Status::ok ? ears_.process(paths, stereo) : status;
    }
    void reset() noexcept {
        paths_.reset();
        ears_.reset();
    }

  private:
    AcousticPathRenderer<T> paths_;
    AnalyticBinaural<T> ears_;
    AudioBuffer<T> pathsAudio_;
};
template <Sample T> class DawBus {
  public:
    Status prepare(const ProcessSpec& spec) noexcept {
        DawBus next;
        const std::array<double, 2> crossovers{400, 3000};
        auto status = next.multiband_.prepare(spec, crossovers, 129);
        if (status != Status::ok)
            return status;
        DynamicsParameters d;
        d.lookAheadMs = 0;
        d.ratio = 2;
        d.thresholdDb = -18;
        for (std::size_t band = 0; band < 3; ++band) {
            status = next.multiband_.setBand(band, MultibandMode::compressor, d);
            if (status != Status::ok)
                return status;
        }
        status = next.limiter_.prepare(spec, 4, 5);
        if (status != Status::ok)
            return status;
        status = next.scratch_.resize(spec.channels, spec.maxBlockFrames);
        if (status != Status::ok)
            return status;
        *this = std::move(next);
        return Status::ok;
    }
    Status process(AudioBlock<const T> in, AudioBlock<T> out) noexcept {
        auto scratch = scratch_.block(in.frames());
        auto status = multiband_.process(in, scratch);
        return status == Status::ok ? limiter_.process(scratch, out) : status;
    }
    double latencyFrames() const noexcept {
        return double(multiband_.latencyFrames()) + limiter_.latencyFrames();
    }
    void reset() noexcept {
        multiband_.reset();
        limiter_.reset();
    }

  private:
    MultibandDynamics<T> multiband_;
    TruePeakLimiter<T> limiter_;
    AudioBuffer<T> scratch_;
};
template class SpatialVoice<float>;
template class SpatialVoice<double>;
template class DawBus<float>;
template class DawBus<double>;
} // namespace cydsp::examples
