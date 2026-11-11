#include <cydsp/CyDSP.h>
// Compile-only integration example. Call initialization outside the audio callback.
struct AudioChain {
    cydsp::Gain<float> gain;
    cydsp::Biquad<float> filter;
    cydsp::Status prepare() {
        const cydsp::ProcessSpec spec{48000.0, 512, 2};
        auto status = gain.prepare(spec);
        if (status != cydsp::Status::ok)
            return status;
        status = filter.prepare(spec);
        if (status != cydsp::Status::ok)
            return status;
        cydsp::BiquadCoefficients coefficients;
        status = cydsp::designBiquad(cydsp::BiquadType::lowPass, spec.sampleRate, 8000.0, 0.707,
                                     coefficients);
        if (status != cydsp::Status::ok)
            return status;
        return filter.setCoefficients(coefficients);
    }
    cydsp::Status process(float* const* channels, std::size_t frames) noexcept {
        cydsp::AudioBlock<float> audio{channels, 2, frames};
        auto status = gain.process(audio, audio);
        return status == cydsp::Status::ok ? filter.process(audio, audio) : status;
    }
};

// Fixed-latency convolution: feed zeros after the source ends for tailFrames().
struct ConvolutionChain {
    cydsp::PartitionedConvolver<float> convolution;
    cydsp::DelayLine<float> dryDelay;
    cydsp::Status prepare(std::span<const float> impulse) {
        const cydsp::ProcessSpec spec{48000, 512, 2};
        auto status = convolution.prepare(spec, impulse, 128);
        if (status != cydsp::Status::ok)
            return status;
        status = dryDelay.prepare(spec, convolution.latencyFrames());
        if (status != cydsp::Status::ok)
            return status;
        return dryDelay.setDelay(static_cast<double>(convolution.latencyFrames()));
    }
};

// The host retains input beyond inputConsumed; outputProduced is the valid prefix.
// Output capacity need not match input frames. A zero-input call can drain pending
// upsampled output; to flush the FIR tail explicitly supply additional zero input.
struct RateConverter {
    cydsp::Resampler<float> resampler;
    cydsp::Status prepare() {
        return resampler.prepare({48000, 44100, 2, 512, 512, 64, 1024});
    }
    cydsp::ResampleResult process(const float* const* input, std::size_t inputFrames,
                                  float* const* output, std::size_t outputCapacity) noexcept {
        return resampler.process({input, 2, inputFrames}, {output, 2, outputCapacity});
    }
};

// This variant emits h[0]*x[n] immediately. It reports no added block latency;
// the IR itself can still contain leading zeros or have frequency-dependent delay.
struct ImmediateConvolution {
    cydsp::HybridConvolver<float> convolution;
    cydsp::Status prepare(std::span<const float> impulse) {
        return convolution.prepare({48000, 512, 2}, impulse, 128);
    }
    cydsp::Status process(float* const* channels, std::size_t frames) noexcept {
        const cydsp::AudioBlock<float> block{channels, 2, frames};
        return convolution.process(block, block);
    }
};

// One independent filter per voice. Parameter updates are serialized with processing.
struct VoiceFilters {
    cydsp::BiquadBank<float> filters;
    cydsp::Status prepare() {
        auto status = filters.prepare({48000, 256, 16});
        if (status != cydsp::Status::ok)
            return status;
        for (std::size_t voice = 0; voice < 16; ++voice) {
            cydsp::BiquadCoefficients c;
            status = cydsp::designPeakingBiquad(48000, 200 + 250 * static_cast<double>(voice),
                                                0.707, 3, c);
            if (status != cydsp::Status::ok)
                return status;
            status = filters.setCoefficients(voice, c);
            if (status != cydsp::Status::ok)
                return status;
        }
        return cydsp::Status::ok;
    }
    cydsp::Status process(float* const* channels, std::size_t frames) noexcept {
        const cydsp::AudioBlock<float> audio{channels, 16, frames};
        return filters.process(audio, audio);
    }
};
struct SmoothedLowPass {
    cydsp::SmoothedBiquad<float> filter;
    cydsp::Status prepare() {
        return filter.prepare({48000, 512, 2});
    }
    cydsp::Status setCutoff(double hz) noexcept {
        cydsp::BiquadCoefficients c;
        const auto status = cydsp::designBiquad(cydsp::BiquadType::lowPass, 48000, hz, 0.707, c);
        return status == cydsp::Status::ok ? filter.setCoefficients(c, 256) : status;
    }
};

// Coefficients are output-major: L = .75*L + .25*R, R = .25*L + .75*R.
struct StereoMixer {
    cydsp::ChannelMatrix<float> matrix;
    cydsp::Status prepare() {
        constexpr std::array<double, 4> coefficients{0.75, 0.25, 0.25, 0.75};
        return matrix.prepare({48000, 512, 2}, 2, coefficients);
    }
    cydsp::Status setMono(std::size_t rampFrames = 256) noexcept {
        constexpr std::array<double, 4> coefficients{0.5, 0.5, 0.5, 0.5};
        return matrix.setMatrix(coefficients, rampFrames);
    }
    cydsp::Status process(float* const* channels, std::size_t frames) noexcept {
        const cydsp::AudioBlock<float> block{channels, 2, frames};
        return matrix.process(block, block);
    }
};
struct ChangingImpulse {
    cydsp::SmoothedConvolver<float> convolution;
    cydsp::Status prepare(std::span<const float> impulse) {
        return convolution.prepare({48000, 512, 2}, impulse, 128);
    }
    // Non-real-time. The owner must serialize this with process, even if another
    // thread loads the impulse file. This call only stages; it does not start fading.
    cydsp::Status stage(std::span<const float> impulse) noexcept {
        return convolution.stageImpulse(impulse);
    }
    // Serialized audio-thread command; no planning, allocation or buffer destruction.
    cydsp::Status activate() noexcept {
        return convolution.startTransition(2048);
    }
    cydsp::Status process(float* const* channels, std::size_t frames) noexcept {
        const cydsp::AudioBlock<float> block{channels, 2, frames};
        return convolution.process(block, block);
    }
};

// Called from a host worker and audio thread respectively. Stop/join the worker
// before reset, re-preparation or destruction; the library creates no thread.
struct WorkerUpdatedImpulse {
    cydsp::AsyncConvolver<float> dsp;
    cydsp::Status prepare(std::span<const float> impulse) {
        return dsp.prepare({48000, 512, 2}, impulse, 128);
    }
    cydsp::Status prepareOnWorker(std::span<const float> impulse) noexcept {
        // update_queue_full means retry from the worker after audio makes progress.
        return dsp.stageImpulse(impulse, 2048);
    }
    cydsp::Status processOnAudio(float* const* channels, std::size_t frames) noexcept {
        const cydsp::AudioBlock<float> block{channels, 2, frames};
        return dsp.process(block, block);
    }
};
struct FullEqualizer {
    cydsp::SmoothedBiquadCascade<float> eq;
    cydsp::Status prepare() {
        return eq.prepare({48000, 512, 2}, 8);
    }
    cydsp::Status setTone(double bassDb, double trebleDb) noexcept {
        std::array<cydsp::BiquadCoefficients, 2> sections;
        auto status = cydsp::designShelfBiquad(false, 48000, 150, 1, bassDb, sections[0]);
        if (status != cydsp::Status::ok)
            return status;
        status = cydsp::designShelfBiquad(true, 48000, 6000, 1, trebleDb, sections[1]);
        return status == cydsp::Status::ok ? eq.setCoefficients(sections, 512) : status;
    }
};
// Two branches with borrowed prepared processors. Graph topology/edges are copied.
struct ParallelFilters {
    cydsp::Biquad<float> filters[2];
    cydsp::AudioGraph<float> graph;
    static cydsp::Status node(void* context, cydsp::AudioBlock<const float> in,
                              cydsp::AudioBlock<float> out) noexcept {
        return static_cast<cydsp::Biquad<float>*>(context)->process(in, out);
    }
    cydsp::Status prepare() {
        const cydsp::ProcessSpec spec{48000, 512, 2};
        for (auto& filter : filters) {
            const auto status = filter.prepare(spec);
            if (status != cydsp::Status::ok)
                return status;
        }
        const std::array<cydsp::GraphInput, 1> input{{{cydsp::graphExternalInput, 1}}};
        const std::array<cydsp::GraphNode<float>, 2> nodes{
            {{node, &filters[0], input}, {node, &filters[1], input}}};
        const std::array<cydsp::GraphInput, 2> output{{{0, 0.5}, {1, 0.5}}};
        return graph.prepare(spec, nodes, output);
    }
};

// Choose a convolution strategy once during preparation, including worker IR updates.
struct ProgressiveReverb {
    cydsp::AsyncConvolver<float> dsp;
    cydsp::Status prepare(std::span<const float> impulse) noexcept {
        const cydsp::ConvolutionConfig config{cydsp::ConvolutionMode::scheduled, 128, 1024};
        return dsp.prepare({48000, 512, 2}, impulse, config);
    }
    cydsp::Status updateOnWorker(std::span<const float> impulse) noexcept {
        return dsp.stageImpulse(impulse, 2048);
    }
    cydsp::Status processOnAudio(float* const* channels, std::size_t frames) noexcept {
        const cydsp::AudioBlock<float> block{channels, 2, frames};
        return dsp.process(block, block);
    }
};

struct ProgressiveMatrixReverb {
    cydsp::MatrixConvolver<float> dsp;
    cydsp::Status prepare(std::span<const cydsp::ConvolutionRoute<float>> routes) noexcept {
        const cydsp::ConvolutionConfig config{cydsp::ConvolutionMode::scheduled, 128, 1024};
        return dsp.prepare({48000, 512, 4}, 4, routes, config);
    }
    cydsp::Status process(cydsp::AudioBlock<const float> input,
                          cydsp::AudioBlock<float> output) noexcept {
        return dsp.process(input, output);
    }
};
