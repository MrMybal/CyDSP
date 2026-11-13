#include <array>
#include <cydsp/CyDSP.h>

// Compile-checked integration sketch, no executable or audio device is launched.
// The host serializes this object's methods and publishes UI snapshots itself.
struct GranularTrack {
    cydsp::SampleData<float> sample;
    cydsp::Granulator<float> engine;
    cydsp::LevelMeter<float> level;
    cydsp::TruePeakMeter<float> truePeak;
    cydsp::SpectrumAnalyzer<float> spectrum;
    cydsp::SpectralBands mel;
    cydsp::LoudnessMeter<float> loudness;
    std::array<double, 40> melValues{};

    GranularTrack() = default;
    GranularTrack(const GranularTrack&) = delete;
    GranularTrack& operator=(const GranularTrack&) = delete;
    // engine borrows sample by address; keep this owner at a stable address.
    GranularTrack(GranularTrack&&) = delete;
    GranularTrack& operator=(GranularTrack&&) = delete;

    // Call while stopped. Check the result; a failed multi-object preparation
    // should keep the track stopped until preparation succeeds.
    cydsp::Status prepare(cydsp::AudioBlock<const float> source, double sourceRate) noexcept {
        using enum cydsp::Status;
        const cydsp::ProcessSpec spec{48000, 512, 2};
        auto status = sample.prepare(source, sourceRate);
        if (status != ok)
            return status;
        status = engine.prepare(spec, sample, 128);
        if (status != ok)
            return status;
        status = level.prepare(spec, 2048);
        if (status != ok)
            return status;
        status = truePeak.prepare(spec, 4, 64);
        if (status != ok)
            return status;
        status = spectrum.prepare(spec, {2048, 512, cydsp::WindowType::hann});
        if (status != ok)
            return status;
        status = mel.prepare(spec.sampleRate, 2048);
        if (status != ok)
            return status;
        status = loudness.prepare(spec); // Default exact gating history: about eight hours.
        if (status != ok)
            return status;
        cydsp::GrainCloudParameters cloud;
        cloud.grainsPerSecond = 30;
        cloud.grain.durationMs = 100;
        cloud.grain.gain = .2;
        cloud.positionSpreadSeconds = .05;
        cloud.pitchSpreadSemitones = 3;
        cloud.panSpread = 1;
        return engine.setCloud(cloud);
    }

    // Serialized callback; no allocation, device/file I/O or UI work.
    cydsp::Status process(cydsp::AudioBlock<float> output) noexcept {
        using enum cydsp::Status;
        auto status = engine.process(output);
        if (status != ok)
            return status;
        status = level.process(output);
        if (status != ok)
            return status;
        status = truePeak.process(output);
        if (status != ok)
            return status;
        const auto previous = spectrum.frameCount();
        status = spectrum.process(output);
        if (status != ok)
            return status;
        if (spectrum.frameCount() != previous) {
            status = mel.process(spectrum.powerDensity(0), melValues);
            if (status != ok)
                return status;
        }
        // would_block means this meter consumed no part of this block. The host
        // must stop/reset/reprepare the meter according to its session policy.
        return loudness.process(output);
    }

    // Off the critical callback, while processing is stopped/owner-serialized.
    double integratedLufs() const noexcept {
        return loudness.integratedLufs();
    }
    double loudnessRangeLu() noexcept {
        return loudness.loudnessRangeLu();
    }
    void finish() noexcept {
        truePeak.flush();
    }
};
