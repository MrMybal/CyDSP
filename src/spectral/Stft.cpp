#include "cydsp/spectral/Stft.h"
#include "cydsp/core/AudioBuffer.h"
#include "cydsp/math/VectorOps.h"
#include "cydsp/processors/RealFft.h"
#include "detail/Analysis.h"
#include <algorithm>
#include <vector>
namespace cydsp {
template <Sample T> struct Stft<T>::State {
    ProcessSpec spec;
    StftConfig config;
    RealFft<T> fft;
    VectorOps ops;
    AudioBuffer<T> input, output, zeros;
    std::vector<T> window, synthesis, scratch;
    std::vector<std::complex<T>> spectrum;
    Processor processor = nullptr;
    void* context = nullptr;
    std::size_t head = 0, since = 0, remaining = 0;
    std::uint64_t frames = 0;
    bool flushing = false;
    void analyze() noexcept {
        const auto n = config.fftFrames;
        for (std::size_t ch = 0; ch < spec.channels; ++ch) {
            const auto source = input.block().channel(ch);
            (void)ops.copy(source.subspan(head), std::span<T>(scratch).first(n - head));
            if (head)
                (void)ops.copy(source.first(head), std::span<T>(scratch).subspan(n - head));
            (void)ops.multiply(scratch, window, scratch);
            (void)fft.forward(scratch, spectrum);
            if (processor)
                processor(context, ch, spectrum);
            spectrum.front().imag(T{});
            spectrum.back().imag(T{});
            (void)fft.inverse(spectrum, scratch);
            (void)ops.multiply(scratch, synthesis, scratch);
            auto target = output.block().channel(ch);
            (void)ops.addScaled(std::span<const T>(scratch).first(n - head), target.subspan(head),
                                T{1});
            if (head)
                (void)ops.addScaled(std::span<const T>(scratch).subspan(n - head),
                                    target.first(head), T{1});
        }
        ++frames;
        since = 0;
    }
    void run(AudioBlock<const T> in, AudioBlock<T> out) noexcept {
        for (std::size_t i = 0; i < in.frames(); ++i) {
            for (std::size_t ch = 0; ch < spec.channels; ++ch)
                input.block().channel(ch)[head] = in.channel(ch)[i];
            for (std::size_t ch = 0; ch < spec.channels; ++ch) {
                auto channel = output.block().channel(ch);
                out.channel(ch)[i] = channel[head];
                channel[head] = T{};
            }
            if (++head == config.fftFrames)
                head = 0;
            if (++since == config.hopFrames)
                analyze();
        }
    }
};
template <Sample T> Stft<T>::Stft() = default;
template <Sample T> Stft<T>::~Stft() = default;
template <Sample T> Stft<T>::Stft(Stft&&) noexcept = default;
template <Sample T> Stft<T>& Stft<T>::operator=(Stft&&) noexcept = default;
template <Sample T>
Status Stft<T>::prepare(const ProcessSpec& spec, const StftConfig& config,
                        CpuBackend backend) noexcept {
    if (!detail::validAnalysisSpec(spec) || !isPowerOfTwo(config.fftFrames) ||
        config.fftFrames < 16 || config.fftFrames > 65536 || !config.hopFrames ||
        config.hopFrames > config.fftFrames || config.fftFrames % config.hopFrames ||
        unsigned(config.window) > unsigned(WindowType::kaiser))
        return Status::invalid_spec;
    try {
        auto s = std::make_unique<State>();
        s->spec = spec;
        s->config = config;
        auto status = VectorOps::create(backend, s->ops);
        if (status != Status::ok)
            return status;
        status = s->fft.prepare(config.fftFrames, backend);
        if (status != Status::ok)
            return status;
        for (auto* b : {&s->input, &s->output}) {
            status = b->resize(spec.channels, config.fftFrames);
            if (status != Status::ok)
                return status;
        }
        status = s->zeros.resize(spec.channels, spec.maxBlockFrames);
        if (status != Status::ok)
            return status;
        s->window.resize(config.fftFrames);
        s->synthesis.resize(config.fftFrames);
        s->scratch.resize(config.fftFrames);
        s->spectrum.resize(s->fft.bins());
        status = makeWindow(config.window, std::span<T>(s->window), WindowSymmetry::periodic);
        if (status != Status::ok)
            return status;
        for (std::size_t phase = 0; phase < config.hopFrames; ++phase) {
            double sum = 0;
            for (std::size_t i = phase; i < config.fftFrames; i += config.hopFrames)
                sum += double(s->window[i]) * s->window[i];
            if (sum < 1e-12)
                return Status::invalid_argument;
            for (std::size_t i = phase; i < config.fftFrames; i += config.hopFrames)
                s->synthesis[i] = T(double(s->window[i]) / sum);
        }
        state_.swap(s);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
template <Sample T> void Stft<T>::setProcessor(Processor p, void* c) noexcept {
    if (state_) {
        state_->processor = p;
        state_->context = c;
    }
}
template <Sample T> Status Stft<T>::process(AudioBlock<const T> in, AudioBlock<T> out) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    auto status = validateProcessBlocks(s.spec, in, out);
    if (status != Status::ok)
        return status;
    status = detail::validateAnalysis(s.spec, in);
    if (status != Status::ok)
        return status;
    if (s.flushing)
        return Status::end_of_stream;
    s.run(in, out);
    return Status::ok;
}
template <Sample T> Status Stft<T>::flush(AudioBlock<T> out) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    auto status = validateProcessBlocks<T>(s.spec, out, out);
    if (status != Status::ok)
        return status;
    if (!out.frames())
        return s.flushing && !s.remaining ? Status::end_of_stream : Status::ok;
    if (!s.flushing) {
        s.flushing = true;
        s.remaining = 2 * s.config.fftFrames;
    }
    const auto count = std::min(out.frames(), s.remaining);
    s.run(s.zeros.block(count), out.subBlock(0, count));
    s.remaining -= count;
    for (std::size_t ch = 0; ch < out.channels(); ++ch)
        s.ops.fill(out.channel(ch).subspan(count), T{});
    return s.remaining ? Status::ok : Status::end_of_stream;
}
template <Sample T> void Stft<T>::reset() noexcept {
    if (!state_)
        return;
    auto& s = *state_;
    s.head = s.since = s.remaining = 0;
    s.frames = 0;
    s.flushing = false;
    for (auto* b : {&s.input, &s.output})
        for (std::size_t ch = 0; ch < s.spec.channels; ++ch)
            s.ops.fill(b->block().channel(ch), T{});
}
template <Sample T> std::size_t Stft<T>::latencyFrames() const noexcept {
    return state_ ? state_->config.fftFrames : 0;
}
template <Sample T> std::size_t Stft<T>::tailFrames() const noexcept {
    return state_ ? 2 * state_->config.fftFrames : 0;
}
template <Sample T> std::size_t Stft<T>::bins() const noexcept {
    return state_ ? state_->fft.bins() : 0;
}
template <Sample T> std::uint64_t Stft<T>::frameCount() const noexcept {
    return state_ ? state_->frames : 0;
}
template <Sample T> CpuBackend Stft<T>::backend() const noexcept {
    return state_ ? state_->ops.backend() : CpuBackend::scalar;
}
template class Stft<float>;
template class Stft<double>;
} // namespace cydsp
