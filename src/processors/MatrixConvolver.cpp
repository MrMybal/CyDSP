// Partitioned overlap-add derived from FFTConvolver.
// Copyright (c) 2017 HiFi-LoFi. MIT: third_party/licenses/FFTConvolver.txt.
#include "cydsp/processors/MatrixConvolver.h"
#include "detail/CheckedSize.h"
#include "detail/FftPlanAccess.h"
#include "detail/IncrementalFft.h"
#include "detail/MatrixAlgorithm.h"
#include "detail/PartitionSchedule.h"
#include "detail/RateClock.h"
#include "detail/RealFftAlgorithm.h"
#include "detail/VectorTable.h"
#include <algorithm>
namespace cydsp {
template <Sample T>
    requires(!std::is_const_v<T>)
struct MatrixConvolver<T>::ProgressiveTier {
    static constexpr std::size_t quantum = detail::fftWorkQuantum;
    enum class Phase { idle, pack, forward, split, mac, merge, inverse, emit };
    Tier data;
    std::vector<T> workInput, nextReady;
    const detail::Kernels<T>* kernels = nullptr;
    std::complex<T>* work = nullptr;
    const std::size_t* reverse = nullptr;
    const std::complex<T>* fftWeights = nullptr;
    const std::complex<T>* realWeights = nullptr;
    std::size_t channel = 0, cursor = 0, route = 0, part = 0, fftHalf = 1, fftCursor = 0;
    std::size_t units = 0, baseJobs = 0, extraJobs = 0, debt = 0;
    bool pipeline = false;
    Phase phase = Phase::idle;
    // Called only on a fresh retained State. The owner catches allocation exceptions.
    Status prepare(std::span<const ConvolutionRoute<T>> definitions, std::size_t offset,
                   std::size_t p, CpuBackend backend) {
        const auto status = data.prepare(definitions, offset,
                                         std::numeric_limits<std::size_t>::max(), p, backend, true);
        if (status != Status::ok)
            return status;
        std::size_t stages = 0, products = 0;
        for (auto n = p; n > 1; n /= 2)
            ++stages;
        for (const auto& filter : data.routes)
            if (!detail::checkedAdd(products, filter.parts, products))
                return Status::invalid_argument;
        const auto chunks = (p + quantum - 1) / quantum;
        const auto macChunks = (p + 1 + quantum - 1) / quantum;
        const auto fftChunks = stages * ((p / 2 + quantum - 1) / quantum);
        std::size_t inputUnits{}, outputUnits{}, macUnits{};
        if (!detail::checkedMultiply(data.inputs.size(), 2 * chunks + fftChunks, inputUnits) ||
            !detail::checkedMultiply(data.outputs.size(), 2 * chunks + fftChunks, outputUnits) ||
            !detail::checkedMultiply(products, macChunks, macUnits) ||
            !detail::checkedAdd(inputUnits, outputUnits, units) ||
            !detail::checkedAdd(units, macUnits, units))
            return Status::invalid_argument;
        baseJobs = units / p;
        extraJobs = units % p;
        workInput.resize(data.input.size());
        nextReady.resize(data.ready.size());
        kernels = &detail::VectorAccess::kernels<T>(data.ops);
        work = detail::FftPlanAccess::scratch(data.fft).data();
        reverse = detail::FftPlanAccess::reverse(data.fft).data();
        fftWeights = detail::FftPlanAccess::complexTwiddles(data.fft).data();
        realWeights = detail::FftPlanAccess::realTwiddles(data.fft).data();
        // Filter preparation used this time buffer; runtime uses the packed FFT workspace.
        std::vector<T>().swap(data.time);
        reset();
        return Status::ok;
    }
    void reset() noexcept {
        data.reset();
        std::fill(workInput.begin(), workInput.end(), T{});
        std::fill(nextReady.begin(), nextReady.end(), T{});
        channel = cursor = route = part = fftCursor = debt = 0;
        fftHalf = 1;
        pipeline = false;
        phase = Phase::idle;
    }
    void runOne() noexcept {
        const auto p = data.partition, bins = p + 1;
        switch (phase) {
        case Phase::pack: {
            const auto end = std::min(p, cursor + quantum);
            detail::packRealChunk(workInput.data() + channel * p, work, reverse, p, cursor, end);
            cursor = end;
            if (cursor == p) {
                cursor = fftCursor = 0;
                fftHalf = 1;
                phase = p > 1 ? Phase::forward : Phase::split;
            }
            break;
        }
        case Phase::forward:
            if (detail::advanceFftChunk<T>(*kernels, {work, p}, fftWeights, fftHalf, fftCursor,
                                           false)) {
                phase = Phase::split;
                cursor = 0;
            }
            break;
        case Phase::split: {
            auto* spectrum = data.history.data() + (channel * data.parts + data.head) * bins;
            const auto end = std::min(p, cursor + quantum);
            if (cursor == 0) {
                spectrum[0] = detail::splitRealBin<T>({work, p}, {realWeights, p - 1}, 0);
                spectrum[p] = detail::splitRealBin<T>({work, p}, {realWeights, p - 1}, p);
            }
            detail::splitRealRange<T>(*kernels, work, realWeights, spectrum, p,
                                      std::max(cursor, std::size_t{1}), end);
            cursor = end;
            if (cursor == p) {
                cursor = 0;
                if (++channel == data.inputs.size()) {
                    route = part = channel = 0;
                    phase = Phase::mac;
                } else
                    phase = Phase::pack;
            }
            break;
        }
        case Phase::mac: {
            const auto& filter = data.routes[route];
            const auto count = std::min(quantum, bins - cursor);
            const auto ring = detail::convolutionHistoryIndex(data.head, part, data.parts);
            kernels->complexMac(
                reinterpret_cast<const T*>(data.history.data() +
                                           (filter.input * data.parts + ring) * bins + cursor),
                reinterpret_cast<const T*>(data.filters.data() + filter.offset + part * bins +
                                           cursor),
                reinterpret_cast<T*>(data.sum.data() + filter.output * bins + cursor), count);
            cursor += count;
            if (cursor == bins) {
                cursor = 0;
                if (++part == filter.parts) {
                    part = 0;
                    if (++route == data.routes.size()) {
                        channel = 0;
                        phase = Phase::merge;
                    }
                }
            }
            break;
        }
        case Phase::merge: {
            auto* spectrum = data.sum.data() + channel * bins;
            const auto end = std::min(p, cursor + quantum);
            if (cursor == 0) {
                spectrum[0].imag(T{});
                spectrum[p].imag(T{});
                work[reverse[0]] =
                    detail::mergeRealBin<T>({spectrum, bins}, {realWeights, p - 1}, 0);
            }
            detail::mergeRealRange<T>(*kernels, spectrum, realWeights, work, reverse, p,
                                      std::max(cursor, std::size_t{1}), end);
            cursor = end;
            if (cursor == p) {
                cursor = fftCursor = 0;
                fftHalf = 1;
                phase = p > 1 ? Phase::inverse : Phase::emit;
            }
            break;
        }
        case Phase::inverse:
            if (detail::advanceFftChunk<T>(*kernels, {work, p}, fftWeights, fftHalf, fftCursor,
                                           true)) {
                phase = Phase::emit;
                cursor = 0;
            }
            break;
        case Phase::emit: {
            const auto end = std::min(p, cursor + quantum);
            detail::emitRealChunk(*kernels, work, data.overlap.data() + channel * p,
                                  nextReady.data() + channel * p, p, cursor, end);
            auto* spectrum = data.sum.data() + channel * bins;
            std::fill(spectrum + cursor, spectrum + end, std::complex<T>{});
            if (cursor == 0)
                spectrum[p] = {};
            cursor = end;
            if (cursor == p) {
                cursor = 0;
                phase = ++channel == data.outputs.size() ? Phase::idle : Phase::merge;
            }
            break;
        }
        case Phase::idle:
            break;
        }
    }
    void process(AudioBlock<const T> source, AudioBlock<T> output) noexcept {
        const auto p = data.partition;
        for (std::size_t ch = 0; ch < output.channels(); ++ch)
            kernels->fill(output.channel(ch).data(), output.frames(), T{});
        for (std::size_t offset = 0; offset < source.frames();) {
            const auto count = std::min(p - data.position, source.frames() - offset);
            for (std::size_t ch = 0; ch < data.inputs.size(); ++ch)
                kernels->copy(source.channel(data.inputs[ch]).data() + offset,
                              data.input.data() + ch * p + data.position, count);
            for (std::size_t ch = 0; ch < data.outputs.size(); ++ch)
                kernels->copy(data.ready.data() + ch * p + data.position,
                              output.channel(data.outputs[ch]).data() + offset, count);
            if (pipeline) {
                // Pending jobs read workInput and write nextReady, not these buffers.
                const auto credit = std::uint64_t(debt) + std::uint64_t(extraJobs) * count;
                const auto due = baseJobs * count + std::size_t(credit / p);
                debt = std::size_t(credit % p);
                for (std::size_t job = 0; job < due; ++job)
                    runOne();
            }
            data.position += count;
            offset += count;
            if (data.position == p) {
                if (pipeline) {
                    data.ready.swap(nextReady);
                    if (++data.head == data.parts)
                        data.head = 0;
                }
                data.input.swap(workInput);
                data.position = channel = cursor = route = part = debt = fftCursor = 0;
                fftHalf = 1;
                pipeline = true;
                phase = Phase::pack;
            }
        }
    }
};
template <Sample T>
    requires(!std::is_const_v<T>)
MatrixConvolver<T>::MatrixConvolver() noexcept = default;
template <Sample T>
    requires(!std::is_const_v<T>)
MatrixConvolver<T>::~MatrixConvolver() = default;
template <Sample T>
    requires(!std::is_const_v<T>)
MatrixConvolver<T>::MatrixConvolver(MatrixConvolver&& other) noexcept {
    swap(other);
}
template <Sample T>
    requires(!std::is_const_v<T>)
MatrixConvolver<T>& MatrixConvolver<T>::operator=(MatrixConvolver&& other) noexcept {
    if (this != &other) {
        MatrixConvolver next(std::move(other));
        swap(next);
    }
    return *this;
}
template <Sample T>
    requires(!std::is_const_v<T>)
std::size_t MatrixConvolver<T>::workUnitsPerPartition() const noexcept {
    return progressive_ ? progressive_->units : 0;
}
template <Sample T>
    requires(!std::is_const_v<T>)
std::size_t MatrixConvolver<T>::maximumWorkUnitsPerFrame() const noexcept {
    return progressive_ ? progressive_->baseJobs + (progressive_->extraJobs ? 1 : 0) : 0;
}

template <Sample T>
    requires(!std::is_const_v<T>)
Status MatrixConvolver<T>::Tier::prepare(std::span<const ConvolutionRoute<T>> definitions,
                                         std::size_t offset, std::size_t end, std::size_t p,
                                         CpuBackend backend, bool progressive,
                                         bool spectralHead) noexcept {
    auto status = VectorOps::create(backend, ops);
    if (status != Status::ok)
        return status;
    try {
        const auto map = [](auto& list, std::size_t id) {
            auto at = std::find(list.begin(), list.end(), id);
            if (at != list.end())
                return static_cast<std::size_t>(at - list.begin());
            list.push_back(id);
            return list.size() - 1;
        };
        for (const auto& def : definitions) {
            const auto limit = std::min(end, def.impulse.size());
            if (limit <= offset)
                continue;
            const auto count = limit - offset, n = 1 + (count - 1) / p;
            std::size_t spectra{}, size{};
            if (!detail::checkedMultiply(n, p + 1, spectra) ||
                !detail::checkedAdd(filters.size(), spectra, size))
                return Status::invalid_argument;
            const auto id = routes.size();
            routes.push_back({map(inputs, def.inputChannel), map(outputs, def.outputChannel), n,
                              filters.size()});
            filters.resize(size);
            parts = std::max(parts, n);
            if (!progressive)
                for (std::size_t part = 1; part < n; ++part)
                    jobs.push_back({id, part});
        }
        if (routes.empty())
            return Status::ok;
        // The head shares this tier's transforms, so it exists only with a tier
        // whose first partition starts exactly one partition into the impulse.
        // Every route gets one, including impulses shorter than a partition.
        if (spectralHead && offset == p)
            for (const auto& def : definitions) {
                std::size_t size{};
                if (!detail::checkedAdd(headFilters.size(), p + 1, size))
                    return Status::invalid_argument;
                heads.push_back({map(inputs, def.inputChannel), map(outputs, def.outputChannel),
                                 headFilters.size()});
                headFilters.resize(size);
            }
        std::size_t samples{}, hist{}, sums{};
        if (!detail::checkedMultiply(inputs.size(), p, samples) ||
            !detail::checkedMultiply(inputs.size(), parts, hist) ||
            !detail::checkedMultiply(hist, p + 1, hist) ||
            !detail::checkedMultiply(outputs.size(), p + 1, sums))
            return Status::invalid_argument;
        input.resize(samples);
        history.resize(hist);
        sum.resize(sums);
        if (!detail::checkedMultiply(outputs.size(), p, samples))
            return Status::invalid_argument;
        ready.resize(samples);
        overlap.resize(samples);
        time.resize(2 * p);
        status = fft.prepare(2 * p, backend);
        if (status != Status::ok)
            return status;
        std::size_t id = 0;
        for (const auto& def : definitions) {
            const auto limit = std::min(end, def.impulse.size());
            if (limit <= offset)
                continue;
            const auto count = limit - offset;
            const auto& route = routes[id++];
            for (std::size_t part = 0; part < route.parts; ++part) {
                std::fill(time.begin(), time.end(), T{});
                std::copy_n(def.impulse.data() + offset + part * p, std::min(p, count - part * p),
                            time.data());
                (void)fft.forward(time,
                                  std::span{filters}.subspan(route.offset + part * (p + 1), p + 1));
            }
        }
        for (std::size_t h = 0; h < heads.size(); ++h) {
            const auto& impulse = definitions[h].impulse;
            std::fill(time.begin(), time.end(), T{});
            std::copy_n(impulse.data(), std::min(p, impulse.size()), time.data());
            (void)fft.forward(time, std::span{headFilters}.subspan(heads[h].offset, p + 1));
        }
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (const std::length_error&) {
        return Status::invalid_argument;
    }
    partition = p;
    baseJobs = jobs.size() / p;
    extraJobs = jobs.size() % p;
    reset();
    return Status::ok;
}
template <Sample T>
    requires(!std::is_const_v<T>)
void MatrixConvolver<T>::Tier::reset() noexcept {
    for (auto* data : {&input, &ready, &overlap, &time})
        std::fill(data->begin(), data->end(), T{});
    std::fill(history.begin(), history.end(), std::complex<T>{});
    std::fill(sum.begin(), sum.end(), std::complex<T>{});
    position = head = done = debt = 0;
    pending = false;
}
template <Sample T>
    requires(!std::is_const_v<T>)
void MatrixConvolver<T>::Tier::accumulate(const Filter& route, std::size_t part) noexcept {
    const auto bins = partition + 1, ring = detail::convolutionHistoryIndex(head, part, parts);
    detail::VectorAccess::kernels<T>(ops).complexMac(
        reinterpret_cast<const T*>(history.data() + (route.input * parts + ring) * bins),
        reinterpret_cast<const T*>(filters.data() + route.offset + part * bins),
        reinterpret_cast<T*>(sum.data() + route.output * bins), bins);
}
template <Sample T>
    requires(!std::is_const_v<T>)
void MatrixConvolver<T>::Tier::boundary() noexcept {
    const auto bins = partition + 1;
    for (std::size_t ch = 0; ch < inputs.size(); ++ch) {
        std::copy_n(input.data() + ch * partition, partition, time.data());
        std::fill(time.begin() + static_cast<std::ptrdiff_t>(partition), time.end(), T{});
        (void)fft.forward(time, std::span{history}.subspan((ch * parts + head) * bins, bins));
    }
    for (const auto& route : routes)
        accumulate(route, 0);
    if (heads.empty())
        emit();
    else
        pending = true;
    done = debt = 0;
    if (++head == parts)
        head = 0;
}
// Inverse transforms of the accumulated spectra into the next partition's output.
template <Sample T>
    requires(!std::is_const_v<T>)
void MatrixConvolver<T>::Tier::emit() noexcept {
    const auto bins = partition + 1;
    for (std::size_t ch = 0; ch < outputs.size(); ++ch) {
        auto spectrum = std::span{sum}.subspan(ch * bins, bins);
        spectrum.front().imag(T{});
        spectrum.back().imag(T{});
        (void)fft.inverse(spectrum, time);
        for (std::size_t i = 0; i < partition; ++i) {
            ready[ch * partition + i] = time[i] + overlap[ch * partition + i];
            overlap[ch * partition + i] = time[partition + i];
        }
    }
    std::fill(sum.begin(), sum.end(), std::complex<T>{});
    pending = false;
}
// A whole partition is available at a partition boundary. Its forward transform
// is the one the boundary needs anyway; multiplying it by the head spectra and
// adding the products to the spectra already accumulated for this partition
// lets one inverse transform per output deliver the head and the older
// partitions together. The second half of that transform carries the head's
// tail into the overlap, so the direct FIR is not needed for these frames.
template <Sample T>
    requires(!std::is_const_v<T>)
void MatrixConvolver<T>::Tier::alignedPartition(AudioBlock<const T> source, AudioBlock<T> output,
                                                std::size_t offset) noexcept {
    const auto bins = partition + 1;
    const auto& kernels = detail::VectorAccess::kernels<T>(ops);
    for (std::size_t ch = 0; ch < inputs.size(); ++ch) {
        std::copy_n(source.channel(inputs[ch]).data() + offset, partition, time.data());
        std::fill(time.begin() + static_cast<std::ptrdiff_t>(partition), time.end(), T{});
        (void)fft.forward(time, std::span{history}.subspan((ch * parts + head) * bins, bins));
    }
    for (const auto& h : heads)
        kernels.complexMac(
            reinterpret_cast<const T*>(history.data() + (h.input * parts + head) * bins),
            reinterpret_cast<const T*>(headFilters.data() + h.offset),
            reinterpret_cast<T*>(sum.data() + h.output * bins), bins);
    for (std::size_t ch = 0; ch < outputs.size(); ++ch) {
        auto spectrum = std::span{sum}.subspan(ch * bins, bins);
        spectrum.front().imag(T{});
        spectrum.back().imag(T{});
        (void)fft.inverse(spectrum, time);
        auto* destination = output.channel(outputs[ch]).data() + offset;
        for (std::size_t i = 0; i < partition; ++i) {
            destination[i] = time[i] + overlap[ch * partition + i];
            overlap[ch * partition + i] = time[partition + i];
        }
    }
    std::fill(sum.begin(), sum.end(), std::complex<T>{});
    // The same products the sample scheduler and the boundary would complete.
    for (const auto& job : jobs)
        accumulate(routes[job.filter], job.part);
    for (const auto& route : routes)
        accumulate(route, 0);
    pending = true;
    done = debt = 0;
    if (++head == parts)
        head = 0;
}
template <Sample T>
    requires(!std::is_const_v<T>)
void MatrixConvolver<T>::Tier::process(AudioBlock<const T> source, AudioBlock<T> output,
                                       bool aligned) noexcept {
    for (std::size_t ch = 0; ch < output.channels(); ++ch)
        ops.fill(output.channel(ch), T{});
    if (!partition)
        return;
    const auto& kernels = detail::VectorAccess::kernels<T>(ops);
    for (std::size_t offset = 0; offset < source.frames();) {
        if (!position && !heads.empty()) {
            if (aligned && source.frames() - offset >= partition) {
                alignedPartition(source, output, offset);
                offset += partition;
                continue;
            }
            if (pending)
                emit();
        }
        const auto count = std::min(partition - position, source.frames() - offset);
        for (std::size_t ch = 0; ch < inputs.size(); ++ch)
            kernels.copy(source.channel(inputs[ch]).data() + offset,
                         input.data() + ch * partition + position, count);
        for (std::size_t ch = 0; ch < outputs.size(); ++ch)
            kernels.copy(ready.data() + ch * partition + position,
                         output.channel(outputs[ch]).data() + offset, count);
        position += count;
        offset += count;
        const auto due = detail::duePartitionJobs(jobs.size(), position, partition);
        while (done < due) {
            const auto job = jobs[done++];
            accumulate(routes[job.filter], job.part);
        }
        if (position == partition) {
            boundary();
            position = 0;
        }
    }
}
template <Sample T>
    requires(!std::is_const_v<T>)
void MatrixConvolver<T>::swap(MatrixConvolver& other) noexcept {
    std::swap(spec_, other.spec_);
    std::swap(config_, other.config_);
    progressive_.swap(other.progressive_);
    routes_.swap(other.routes_);
    headHistory_.swap(other.headHistory_);
    std::swap(early_, other.early_);
    std::swap(late_, other.late_);
    input_.swap(other.input_);
    scratch_.swap(other.scratch_);
    std::swap(ops_, other.ops_);
    std::swap(outputs_, other.outputs_);
    std::swap(tile_, other.tile_);
    std::swap(tail_, other.tail_);
    std::swap(headSize_, other.headSize_);
    std::swap(write_, other.write_);
    std::swap(capacity_, other.capacity_);
    std::swap(headZero_, other.headZero_);
}
template <Sample T>
    requires(!std::is_const_v<T>)
Status MatrixConvolver<T>::prepare(const ProcessSpec& spec, std::size_t outputs,
                                   std::span<const ConvolutionRoute<T>> routes, std::size_t early,
                                   std::size_t late, CpuBackend backend) noexcept {
    return prepare(spec, outputs, routes, {ConvolutionMode::nonuniform, early, late}, backend);
}
template <Sample T>
    requires(!std::is_const_v<T>)
Status MatrixConvolver<T>::prepare(const ProcessSpec& spec, std::size_t outputs,
                                   std::span<const ConvolutionRoute<T>> routes,
                                   const ConvolutionConfig& config, CpuBackend backend) noexcept {
    if (!spec.isValid())
        return Status::invalid_spec;
    const auto early = config.earlyPartition, late = config.latePartition;
    if (config.mode != ConvolutionMode::hybrid && config.mode != ConvolutionMode::nonuniform &&
        config.mode != ConvolutionMode::scheduled)
        return Status::invalid_argument;
    if (!outputs || !isPowerOfTwo(early) || early > 65536 ||
        (config.mode != ConvolutionMode::hybrid &&
         (!isPowerOfTwo(late) || late < early || late > (1u << 23))))
        return Status::invalid_argument;
    for (const auto& route : routes) {
        if (route.inputChannel >= spec.channels || route.outputChannel >= outputs ||
            route.impulse.empty())
            return Status::invalid_argument;
        for (auto x : route.impulse)
            if (!isFinite(x))
                return Status::invalid_argument;
    }
    MatrixConvolver next;
    auto status = VectorOps::create(backend, next.ops_);
    if (status != Status::ok)
        return status;
    next.tile_ = std::min(spec.maxBlockFrames, std::size_t{64});
    // A spectral head handles one whole early partition per step.
    const bool spectralHead = config.spectralHead && spec.maxBlockFrames >= early;
    next.capacity_ = spectralHead ? std::max(next.tile_, early) : next.tile_;
    status = next.input_.resize(spec.channels, next.capacity_);
    if (status != Status::ok)
        return status;
    status = next.scratch_.resize(outputs, next.capacity_);
    if (status != Status::ok)
        return status;
    std::size_t history{};
    if (!detail::checkedMultiply(early + next.capacity_, spec.channels, history))
        return Status::invalid_argument;
    try {
        next.headHistory_.resize(history);
        next.routes_.reserve(routes.size());
        for (const auto& route : routes) {
            const auto head = route.impulse.first(std::min(early, route.impulse.size()));
            next.routes_.push_back(
                {route.inputChannel, route.outputChannel, {head.rbegin(), head.rend()}});
            next.tail_ = std::max(next.tail_, route.impulse.size() - 1);
        }
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (const std::length_error&) {
        return Status::invalid_argument;
    }
    const auto end = config.mode == ConvolutionMode::hybrid
                         ? std::numeric_limits<std::size_t>::max()
                         : late * (config.mode == ConvolutionMode::scheduled ? 2 : 1);
    status =
        next.early_.prepare(routes, early, end, early, next.ops_.backend(), false, spectralHead);
    if (status != Status::ok)
        return status;
    if (config.mode == ConvolutionMode::scheduled) {
        const auto needed = std::any_of(routes.begin(), routes.end(),
                                        [end](const auto& r) { return r.impulse.size() > end; });
        if (needed) {
            try {
                next.progressive_ = std::make_unique<ProgressiveTier>();
                status = next.progressive_->prepare(routes, end, late, next.ops_.backend());
            } catch (const std::bad_alloc&) {
                return Status::allocation_failed;
            } catch (const std::length_error&) {
                return Status::invalid_argument;
            }
        }
    } else if (config.mode == ConvolutionMode::nonuniform)
        status = next.late_.prepare(routes, late, std::numeric_limits<std::size_t>::max(), late,
                                    next.ops_.backend());
    if (status != Status::ok)
        return status;
    next.config_ = config;
    next.spec_ = spec;
    next.outputs_ = outputs;
    next.headSize_ = early;
    swap(next);
    return Status::ok;
}
template <Sample T>
    requires(!std::is_const_v<T>)
void MatrixConvolver<T>::reset() noexcept {
    early_.reset();
    if (progressive_)
        progressive_->reset();
    late_.reset();
    write_ = 0;
    headZero_ = true;
    std::fill(headHistory_.begin(), headHistory_.end(), T{});
}
template <Sample T>
    requires(!std::is_const_v<T>)
Status MatrixConvolver<T>::process(AudioBlock<const T> input, AudioBlock<T> output) noexcept {
    if (!tile_)
        return Status::not_prepared;
    if (!input.isValid() || !output.isValid())
        return Status::invalid_argument;
    if (input.channels() != spec_.channels || output.channels() != outputs_)
        return Status::channel_mismatch;
    if (input.frames() != output.frames())
        return Status::invalid_argument;
    if (input.frames() > spec_.maxBlockFrames)
        return Status::block_too_large;
    if (input.frames() > std::numeric_limits<std::size_t>::max() / sizeof(T))
        return Status::invalid_argument;
    const auto status = detail::validateMatrixAlias(
        spec_.channels, outputs_, input.frames() * sizeof(T),
        [input](auto ch) { return reinterpret_cast<std::uintptr_t>(input.channel(ch).data()); },
        [output](auto ch) { return reinterpret_cast<std::uintptr_t>(output.channel(ch).data()); });
    if (status != Status::ok)
        return status;

    if (routes_.empty()) {
        for (std::size_t ch = 0; ch < outputs_; ++ch)
            ops_.fill(output.channel(ch), T{});
        return Status::ok;
    }
    const bool spectral = !early_.heads.empty();
    for (std::size_t offset = 0; offset < input.frames();) {
        // With a spectral head, steps end at early partition boundaries so each
        // new partition can be taken whole when the call still holds all of it.
        const bool aligned =
            spectral && !early_.position && input.frames() - offset >= headSize_;
        auto count = aligned ? headSize_ : std::min(tile_, input.frames() - offset);
        if (spectral && !aligned)
            count = std::min(count, headSize_ - early_.position);
        auto saved = input_.block(count);
        auto out = output.subBlock(offset, count);
        for (std::size_t ch = 0; ch < spec_.channels; ++ch)
            (void)ops_.copy(input.channel(ch).subspan(offset, count), saved.channel(ch));
        early_.process(saved, out, aligned);
        if (progressive_ || late_.partition) {
            auto scratch = scratch_.block(count);
            if (progressive_)
                progressive_->process(saved, scratch);
            else
                late_.process(saved, scratch);
            for (std::size_t ch = 0; ch < outputs_; ++ch)
                (void)ops_.addScaled(scratch.channel(ch), out.channel(ch), T{1});
        }
        // One contiguous history window per tile. Each dot reads the same ordered
        // samples as the mirrored ring, without per-frame modulo/route bookkeeping.
        // A spectrally convolved partition leaves its head tail in the tier
        // overlap: the direct history then holds zeros for those frames. Coming
        // from direct frames, the FIR still runs once over the old history alone.
        if (aligned && headZero_) {
            offset += count;
            continue;
        }
        const auto stride = headSize_ + capacity_;
        for (std::size_t ch = 0; ch < spec_.channels; ++ch) {
            auto* history = headHistory_.data() + ch * stride;
            if (aligned)
                std::fill_n(history + headSize_, count, T{});
            else
                std::copy_n(saved.channel(ch).data(), count, history + headSize_);
        }
        const auto& kernels = detail::VectorAccess::kernels<T>(ops_);
        for (const auto& route : routes_) {
            const auto* data = headHistory_.data() + route.input * stride + headSize_ + 1 -
                               route.coefficients.size();
            auto destination = out.channel(route.output);
            if (kernels.firHead && count >= 16)
                kernels.firHead(data, route.coefficients.data(), destination.data(), count,
                                route.coefficients.size());
            else
                for (std::size_t f = 0; f < count; ++f)
                    destination[f] +=
                        kernels.dot(data + f, route.coefficients.data(), route.coefficients.size());
        }
        for (std::size_t ch = 0; ch < spec_.channels; ++ch) {
            auto* history = headHistory_.data() + ch * stride;
            // Forward copy is valid for this leftward overlapping shift.
            for (std::size_t f = 0; f < headSize_; ++f)
                history[f] = history[count + f];
        }
        headZero_ = aligned;
        offset += count;
    }
    return Status::ok;
}
template class MatrixConvolver<float>;
template class MatrixConvolver<double>;
} // namespace cydsp
