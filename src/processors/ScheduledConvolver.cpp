// Partitioned overlap-add derived from FFTConvolver.
// Copyright (c) 2017 HiFi-LoFi. MIT: third_party/licenses/FFTConvolver.txt.
// CyDSP: bounded single-thread jobs, incremental packed real FFT and zero-latency prefix.
#include "cydsp/processors/ScheduledConvolver.h"
#include "cydsp/processors/HybridConvolver.h"
#include "detail/CheckedSize.h"
#include "detail/FftPlanAccess.h"
#include "detail/IncrementalFft.h"
#include "detail/PartitionSchedule.h"
#include "detail/RateClock.h"
#include "detail/RealFftAlgorithm.h"
#include <algorithm>
#include <new>
#include <stdexcept>
namespace cydsp {
template <Sample T>
    requires(!std::is_const_v<T>)
struct ScheduledConvolver<T>::State {
    static constexpr std::size_t quantum = detail::fftWorkQuantum;
    enum class Phase { idle, pack, forward, split, mac, merge, inverse, emit };
    ProcessSpec spec{};
    HybridConvolver<T> prefix;
    RealFft<T> fft;
    VectorOps ops;
    const detail::Kernels<T>* kernels = nullptr;
    // State stays at a stable address under unique ownership; FFT storage never resizes after
    // prepare.
    std::complex<T>* work = nullptr;
    const std::size_t* reverse = nullptr;
    const std::complex<T>* fftWeights = nullptr;
    const std::complex<T>* realWeights = nullptr;
    AudioBuffer<T> tailBuffer;
    std::vector<T> input, workInput, ready, nextReady, overlap;
    std::vector<std::complex<T>> filters, history, sum;
    std::size_t p = 0, parts = 0, tile = 0, tail = 0, position = 0, head = 0, channel = 0,
                cursor = 0, part = 0;
    std::size_t fftHalf = 1, fftCursor = 0, units = 0, baseJobs = 0, extraJobs = 0, debt = 0;
    bool pipeline = false;
    Phase phase = Phase::idle;
    Status initialize(const ProcessSpec& requested, std::span<const T> ir, std::size_t early,
                      std::size_t late, CpuBackend backend) {
        spec = requested;
        p = late;
        tail = ir.size() - 1;
        tile = std::min(spec.maxBlockFrames, std::size_t{64});
        auto status = prefix.prepare({spec.sampleRate, tile, spec.channels},
                                     ir.first(std::min(ir.size(), 2 * p)), early, backend);
        if (status != Status::ok)
            return status;
        if (ir.size() <= 2 * p)
            return Status::ok;
        status = VectorOps::create(prefix.backend(), ops);
        if (status != Status::ok)
            return status;
        kernels = &detail::VectorAccess::kernels<T>(ops);
        status = fft.prepare(2 * p, prefix.backend());
        if (status != Status::ok)
            return status;
        work = detail::FftPlanAccess::scratch(fft).data();
        reverse = detail::FftPlanAccess::reverse(fft).data();
        fftWeights = detail::FftPlanAccess::complexTwiddles(fft).data();
        realWeights = detail::FftPlanAccess::realTwiddles(fft).data();
        status = tailBuffer.resize(spec.channels, tile);
        if (status != Status::ok)
            return status;
        const auto impulse = ir.subspan(2 * p);
        parts = 1 + (impulse.size() - 1) / p;
        std::size_t spectra{}, histories{}, samples{};
        if (!detail::checkedMultiply(parts, p + 1, spectra) ||
            !detail::checkedMultiply(spectra, spec.channels, histories) ||
            !detail::checkedMultiply(p, spec.channels, samples))
            return Status::invalid_argument;
        std::size_t stages = 0;
        for (auto n = p; n > 1; n /= 2)
            ++stages;
        const auto chunks = (p + quantum - 1) / quantum,
                   macChunks = (p + 1 + quantum - 1) / quantum;
        const auto butterflyChunks = p > 1 ? (p / 2 + quantum - 1) / quantum : 0;
        std::size_t products{}, transforms{}, perChannel{};
        if (!detail::checkedMultiply(parts, macChunks, products) ||
            !detail::checkedMultiply(2 * stages, butterflyChunks, transforms) ||
            !detail::checkedAdd(products, transforms, perChannel) ||
            !detail::checkedAdd(perChannel, 4 * chunks, perChannel) ||
            !detail::checkedMultiply(perChannel, spec.channels, units))
            return Status::invalid_argument;
        baseJobs = units / p;
        extraJobs = units % p;
        filters.resize(spectra);
        history.resize(histories);
        sum.resize(p + 1);
        input.resize(samples);
        workInput.resize(samples);
        ready.resize(samples);
        nextReady.resize(samples);
        overlap.resize(samples);
        std::vector<T> time(2 * p);
        for (std::size_t id = 0; id < parts; ++id) {
            std::fill(time.begin(), time.end(), T{});
            const auto offset = id * p, count = std::min(p, impulse.size() - offset);
            std::copy_n(impulse.data() + offset, count, time.data());
            (void)fft.forward(time, std::span{filters}.subspan(id * (p + 1), p + 1));
        }
        reset();
        return Status::ok;
    }
    void reset() noexcept {
        prefix.reset();
        for (auto* buffer : {&input, &workInput, &ready, &nextReady, &overlap})
            std::fill(buffer->begin(), buffer->end(), T{});
        std::fill(history.begin(), history.end(), std::complex<T>{});
        std::fill(sum.begin(), sum.end(), std::complex<T>{});
        position = head = channel = cursor = part = fftCursor = debt = 0;
        fftHalf = 1;
        pipeline = false;
        phase = Phase::idle;
    }
    bool transformChunk(bool inverse) noexcept {
        return detail::advanceFftChunk<T>(*kernels, {work, p}, fftWeights, fftHalf, fftCursor,
                                          inverse);
    }
    void runOne() noexcept {
        switch (phase) {
        case Phase::pack: {
            const auto end = std::min(p, cursor + quantum);
            for (auto i = cursor; i < end; ++i) {
                const auto real = 2 * i < p ? workInput[channel * p + 2 * i] : T{};
                const auto imag = 2 * i + 1 < p ? workInput[channel * p + 2 * i + 1] : T{};
                work[reverse[i]] = {real, imag};
                sum[i] = {};
            }
            if (cursor == 0)
                sum[p] = {};
            cursor = end;
            if (cursor == p) {
                cursor = 0;
                fftHalf = 1;
                fftCursor = 0;
                phase = p > 1 ? Phase::forward : Phase::split;
            }
            break;
        }
        case Phase::forward:
            if (transformChunk(false)) {
                phase = Phase::split;
                cursor = 0;
            }
            break;
        case Phase::split: {
            auto* spectrum = history.data() + (channel * parts + head) * (p + 1);
            const auto end = std::min(p, cursor + quantum);
            if (cursor == 0) {
                spectrum[0] = detail::splitRealBin<T>({work, p}, {realWeights, p - 1}, 0);
                spectrum[p] = detail::splitRealBin<T>({work, p}, {realWeights, p - 1}, p);
            }
            detail::splitRealRange<T>(*kernels, work, realWeights, spectrum, p,
                                      std::max(cursor, std::size_t{1}), end);
            cursor = end;
            if (cursor == p) {
                phase = Phase::mac;
                cursor = part = 0;
            }
            break;
        }
        case Phase::mac: {
            const auto bins = p + 1, count = std::min(quantum, bins - cursor);
            const auto ring = detail::convolutionHistoryIndex(head, part, parts);
            kernels->complexMac(reinterpret_cast<const T*>(
                                    history.data() + (channel * parts + ring) * bins + cursor),
                                reinterpret_cast<const T*>(filters.data() + part * bins + cursor),
                                reinterpret_cast<T*>(sum.data() + cursor), count);
            cursor += count;
            if (cursor == bins) {
                cursor = 0;
                if (++part == parts) {
                    sum[0].imag(T{});
                    sum[p].imag(T{});
                    phase = Phase::merge;
                }
            }
            break;
        }
        case Phase::merge: {
            const auto end = std::min(p, cursor + quantum);
            if (cursor == 0)
                work[reverse[0]] =
                    detail::mergeRealBin<T>({sum.data(), p + 1}, {realWeights, p - 1}, 0);
            detail::mergeRealRange<T>(*kernels, sum.data(), realWeights, work, reverse, p,
                                      std::max(cursor, std::size_t{1}), end);
            cursor = end;
            if (cursor == p) {
                cursor = 0;
                fftHalf = 1;
                fftCursor = 0;
                phase = p > 1 ? Phase::inverse : Phase::emit;
            }
            break;
        }
        case Phase::inverse:
            if (transformChunk(true)) {
                phase = Phase::emit;
                cursor = 0;
            }
            break;
        case Phase::emit: {
            const auto end = std::min(p, cursor + quantum);
            const auto scale = T{1} / static_cast<T>(p);
            if (p == 1) {
                nextReady[channel] = work[0].real() + overlap[channel];
                overlap[channel] = work[0].imag();
            } else
                for (auto i = cursor; i < end; i += 2) {
                    const auto first = work[i / 2] * scale, second = work[(i + p) / 2] * scale;
                    const auto offset = channel * p + i;
                    nextReady[offset] = first.real() + overlap[offset];
                    nextReady[offset + 1] = first.imag() + overlap[offset + 1];
                    overlap[offset] = second.real();
                    overlap[offset + 1] = second.imag();
                }
            cursor = end;
            if (cursor == p) {
                cursor = 0;
                phase = ++channel == spec.channels ? Phase::idle : Phase::pack;
            }
            break;
        }
        case Phase::idle:
            break;
        }
    }
    void processTail(AudioBlock<const T> source, AudioBlock<T> output) noexcept {
        for (std::size_t f = 0; f < source.frames(); ++f) {
            for (std::size_t ch = 0; ch < spec.channels; ++ch) {
                input[ch * p + position] = source.channel(ch)[f];
                output.channel(ch)[f] = ready[ch * p + position];
            }
            if (pipeline) {
                const auto jobs = detail::advancePartitionJobs(baseJobs, extraJobs, p, debt);
                for (std::size_t job = 0; job < jobs; ++job)
                    runOne();
            }
            if (++position == p) {
                if (pipeline) {
                    ready.swap(nextReady);
                    if (++head == parts)
                        head = 0;
                }
                input.swap(workInput);
                position = channel = cursor = part = debt = 0;
                fftHalf = 1;
                fftCursor = 0;
                phase = Phase::pack;
                pipeline = true;
            }
        }
    }
};
template <Sample T>
    requires(!std::is_const_v<T>)
ScheduledConvolver<T>::ScheduledConvolver() noexcept = default;
template <Sample T>
    requires(!std::is_const_v<T>)
ScheduledConvolver<T>::~ScheduledConvolver() = default;
template <Sample T>
    requires(!std::is_const_v<T>)
ScheduledConvolver<T>::ScheduledConvolver(ScheduledConvolver&&) noexcept = default;
template <Sample T>
    requires(!std::is_const_v<T>)
ScheduledConvolver<T>& ScheduledConvolver<T>::operator=(ScheduledConvolver&& other) noexcept {
    if (this != &other)
        state_ = std::move(other.state_);
    return *this;
}
template <Sample T>
    requires(!std::is_const_v<T>)
Status ScheduledConvolver<T>::prepare(const ProcessSpec& spec, std::span<const T> ir,
                                      std::size_t early, std::size_t late,
                                      CpuBackend backend) noexcept {
    if (!spec.isValid())
        return Status::invalid_spec;
    if (ir.empty() || !isPowerOfTwo(early) || early > 65536 || !isPowerOfTwo(late) ||
        late < early || late > (std::size_t{1} << 23))
        return Status::invalid_argument;
    for (auto value : ir)
        if (!isFinite(value))
            return Status::invalid_argument;
    try {
        auto next = std::make_unique<State>();
        const auto status = next->initialize(spec, ir, early, late, backend);
        if (status != Status::ok)
            return status;
        state_.swap(next);
        return Status::ok;
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (const std::length_error&) {
        return Status::invalid_argument;
    }
}
template <Sample T>
    requires(!std::is_const_v<T>)
void ScheduledConvolver<T>::reset() noexcept {
    if (state_)
        state_->reset();
}
template <Sample T>
    requires(!std::is_const_v<T>)
std::size_t ScheduledConvolver<T>::tailFrames() const noexcept {
    return state_ ? state_->tail : 0;
}
template <Sample T>
    requires(!std::is_const_v<T>)
CpuBackend ScheduledConvolver<T>::backend() const noexcept {
    return state_ ? state_->prefix.backend() : CpuBackend::scalar;
}
template <Sample T>
    requires(!std::is_const_v<T>)
bool ScheduledConvolver<T>::schedulingActive() const noexcept {
    return state_ && state_->parts;
}
template <Sample T>
    requires(!std::is_const_v<T>)
std::size_t ScheduledConvolver<T>::workUnitsPerPartition() const noexcept {
    return state_ ? state_->units : 0;
}
template <Sample T>
    requires(!std::is_const_v<T>)
std::size_t ScheduledConvolver<T>::maximumWorkUnitsPerFrame() const noexcept {
    return state_ ? state_->baseJobs + (state_->extraJobs ? 1 : 0) : 0;
}
template <Sample T>
    requires(!std::is_const_v<T>)
Status ScheduledConvolver<T>::process(AudioBlock<const T> source, AudioBlock<T> target) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& state = *state_;
    const auto status = validateProcessBlocks(state.spec, source, target);
    if (status != Status::ok)
        return status;
    for (std::size_t offset = 0; offset < source.frames();) {
        const auto count = std::min(state.tile, source.frames() - offset);
        const auto input = source.subBlock(offset, count);
        auto output = target.subBlock(offset, count);
        if (state.parts) {
            auto tail = state.tailBuffer.block(count);
            state.processTail(input, tail);
            (void)state.prefix.process(input, output);
            for (std::size_t ch = 0; ch < state.spec.channels; ++ch)
                state.kernels->addScaled(tail.channel(ch).data(), output.channel(ch).data(), count,
                                         T{1});
        } else
            (void)state.prefix.process(input, output);
        offset += count;
    }
    return Status::ok;
}
template class ScheduledConvolver<float>;
template class ScheduledConvolver<double>;
} // namespace cydsp
