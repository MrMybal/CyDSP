#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <cydsp/CyDSP.h>
#include <new>
#include <stdexcept>
#if defined(_MSC_VER)
#include <malloc.h>
#endif
namespace {
thread_local bool tracked = false;
thread_local std::size_t allocations = 0, deallocations = 0;
void* allocate(std::size_t size) {
    if (tracked)
        ++allocations;
    if (auto* p = std::malloc(size ? size : 1))
        return p;
    throw std::bad_alloc();
}
void release(void* p) noexcept {
    if (p && tracked)
        ++deallocations;
    std::free(p);
}
void* alignedAllocate(std::size_t size, std::size_t alignment) {
    if (tracked)
        ++allocations;
#if defined(_MSC_VER)
    if (auto* p = _aligned_malloc(size ? size : 1, alignment))
        return p;
#else
    void* p = nullptr;
    if (posix_memalign(&p, alignment, size ? size : 1) == 0)
        return p;
#endif
    throw std::bad_alloc();
}
void alignedRelease(void* p) noexcept {
    if (p && tracked)
        ++deallocations;
#if defined(_MSC_VER)
    _aligned_free(p);
#else
    std::free(p);
#endif
}
struct Scope {
    Scope() {
        allocations = 0;
        deallocations = 0;
        tracked = true;
    }
    ~Scope() {
        tracked = false;
    }
};
void require(bool value, const char* message) {
    if (!value)
        throw std::runtime_error(message);
}
} // namespace
void* operator new(std::size_t n) {
    return allocate(n);
}
void* operator new[](std::size_t n) {
    return allocate(n);
}
void operator delete(void* p) noexcept {
    release(p);
}
void operator delete[](void* p) noexcept {
    release(p);
}
void operator delete(void* p, std::size_t) noexcept {
    release(p);
}
void operator delete[](void* p, std::size_t) noexcept {
    release(p);
}
void* operator new(std::size_t n, std::align_val_t a) {
    return alignedAllocate(n, static_cast<std::size_t>(a));
}
void* operator new[](std::size_t n, std::align_val_t a) {
    return alignedAllocate(n, static_cast<std::size_t>(a));
}
void operator delete(void* p, std::align_val_t) noexcept {
    alignedRelease(p);
}
void operator delete[](void* p, std::align_val_t) noexcept {
    alignedRelease(p);
}
void operator delete(void* p, std::size_t, std::align_val_t) noexcept {
    alignedRelease(p);
}
void operator delete[](void* p, std::size_t, std::align_val_t) noexcept {
    alignedRelease(p);
}
namespace {
using namespace cydsp;
template <class T> void audit() {
    const ProcessSpec spec{48000, 64, 2};
    const std::array<T, 3> ir{T{1}, T{0.5}, T{0.25}};
    std::vector<T> longIr(257, T{0.03125});
    AudioBuffer<T> input, output, real;
    require(input.resize(2, 64) == Status::ok && output.resize(2, 64) == Status::ok &&
                real.resize(1, 64) == Status::ok,
            "audit buffers");
    Gain<T> gain;
    Biquad<T> biquad;
    BiquadBank<T> bank;
    SmoothedBiquad<T> smooth;
    SmoothedBiquadCascade<T> eq;
    BiquadCascade<T> cascade;
    Fir<T> fir;
    DelayLine<T> delay;
    HybridConvolver<T> hybrid;
    PartitionedConvolver<T> partitioned;
    NonuniformConvolver<T> nonuniform;
    ScheduledConvolver<T> scheduled;
    std::array<Convolver<T>, 3> configured;
    std::array<SmoothedConvolver<T>, 3> configuredSmooth;
    std::array<AsyncConvolver<T>, 3> configuredAsync;
    MatrixConvolver<T> matrix;
    MatrixConvolver<T> progressiveMatrix;
    SmoothedConvolver<T> changing;
    AsyncConvolver<T> asynchronous;
    Resampler<T> resampler;
    ChannelMatrix<T> mixing;
    AudioGraph<T> graph;
    RealFft<T> fft;
    ComplexFft<T> complex;
    const std::array<BiquadCoefficients, 2> coefficients{{{0.5, 0, 0, 0, 0}, {0.5, 0, 0, 0, 0}}};
    const std::array<double, 4> gains{0.5, 0.5, 0.5, 0.5};
    const std::array<ConvolutionRoute<T>, 2> routes{{{0, 0, longIr}, {1, 1, longIr}}};
    const std::array<GraphInput, 1> from{{{graphExternalInput, 1}}}, to{{{0, 1}}};
    const std::array<GraphNode<T>, 1> nodes{{{nullptr, nullptr, from}}};
    require(gain.prepare(spec) == Status::ok && biquad.prepare(spec) == Status::ok &&
                bank.prepare(spec) == Status::ok && smooth.prepare(spec) == Status::ok &&
                eq.prepare(spec, 4) == Status::ok &&
                cascade.prepare(spec, coefficients) == Status::ok &&
                fir.prepare(spec, ir) == Status::ok && delay.prepare(spec, 128) == Status::ok &&
                hybrid.prepare(spec, longIr, 8) == Status::ok &&
                partitioned.prepare(spec, longIr, 8) == Status::ok &&
                nonuniform.prepare(spec, longIr, 8, 64) == Status::ok &&
                scheduled.prepare(spec, longIr, 8, 64) == Status::ok &&
                matrix.prepare(spec, 2, routes, 8, 64) == Status::ok &&
                progressiveMatrix.prepare(spec, 2, routes, {ConvolutionMode::scheduled, 8, 64}) ==
                    Status::ok &&
                changing.prepare(spec, ir, 8) == Status::ok &&
                changing.stageImpulse(longIr) == Status::ok &&
                asynchronous.prepare(spec, ir, 8) == Status::ok &&
                asynchronous.stageImpulse(longIr, 129) == Status::ok &&
                resampler.prepare({48000, 44100, 2, 64, 64, 32, 256, 0.75, 3.0}) == Status::ok &&
                mixing.prepare(spec, 2, gains) == Status::ok &&
                graph.prepare(spec, nodes, to) == Status::ok && fft.prepare(64) == Status::ok &&
                complex.prepare(64) == Status::ok,
            "audit prepare");
    for (std::size_t i = 0; i < 3; ++i) {
        const ConvolutionConfig config{static_cast<ConvolutionMode>(i), 8, 64};
        require(configured[i].prepare(spec, longIr, config) == Status::ok &&
                    configuredSmooth[i].prepare(spec, ir, config) == Status::ok &&
                    configuredSmooth[i].stageImpulse(longIr) == Status::ok &&
                    configuredAsync[i].prepare(spec, ir, config) == Status::ok &&
                    configuredAsync[i].stageImpulse(longIr, 129) == Status::ok,
                "configured realtime plans");
    }
    std::array<std::complex<T>, 33> spectrum{};
    std::array<std::complex<T>, 64> ci{}, co{};
    bool ok = true;
    {
        Scope guard;
        ok = ok && gain.setGain(0.5, 257) == Status::ok && delay.setDelay(32, 257) == Status::ok &&
             smooth.setCoefficients(coefficients[0], 129) == Status::ok &&
             eq.setCoefficients(coefficients, 129) == Status::ok &&
             resampler.setRatio(0.8, 257) == Status::ok &&
             changing.startTransition(129) == Status::ok &&
             mixing.setMatrix(gains, 129) == Status::ok;
        for (auto& plan : configuredSmooth)
            ok = plan.startTransition(129) == Status::ok && ok;
        for (int i = 0; i < 32; ++i) {
            const auto in = input.block(), out = output.block();
            for (std::size_t j = 0; j < 3; ++j)
                ok = configured[j].process(in, out) == Status::ok &&
                     configuredSmooth[j].process(in, out) == Status::ok &&
                     configuredAsync[j].process(in, out) == Status::ok && ok;
            ok = ok && gain.process(in, out) == Status::ok &&
                 biquad.process(in, out) == Status::ok && bank.process(in, out) == Status::ok &&
                 smooth.process(in, out) == Status::ok && eq.process(in, out) == Status::ok &&
                 cascade.process(in, out) == Status::ok && fir.process(in, out) == Status::ok &&
                 delay.process(in, out) == Status::ok && hybrid.process(in, out) == Status::ok &&
                 partitioned.process(in, out) == Status::ok &&
                 nonuniform.process(in, out) == Status::ok &&
                 scheduled.process(in, out) == Status::ok &&
                 progressiveMatrix.process(in, out) == Status::ok &&
                 matrix.process(in, out) == Status::ok && changing.process(in, out) == Status::ok &&
                 asynchronous.process(in, out) == Status::ok &&
                 resampler.process(in, out).status == Status::ok &&
                 mixing.process(in, out) == Status::ok && graph.process(in, out) == Status::ok &&
                 fft.forward(real.block().channel(0), spectrum) == Status::ok &&
                 fft.inverse(spectrum, real.block().channel(0)) == Status::ok &&
                 complex.process(ci, co, FftDirection::forward) == Status::ok;
        }
        for (std::size_t i = 0; i < 3; ++i) {
            // Leave scheduled FFT work partial before reset; completed fades retain old plans.
            ok = configured[i].process(input.block(17), output.block(17)) == Status::ok && ok;
            configured[i].reset();
            configuredSmooth[i].reset();
            configuredAsync[i].reset();
        }
        for (int i = 0; i < 64 && !resampler.isFinished(); ++i) {
            const auto step = resampler.finish(output.block());
            ok = step.status == Status::ok && ok;
        }
        ok = resampler.isFinished() && ok;
        gain.reset();
        biquad.reset();
        bank.reset();
        smooth.reset();
        eq.reset();
        cascade.reset();
        fir.reset();
        delay.reset();
        hybrid.reset();
        partitioned.reset();
        nonuniform.reset();
        ok = scheduled.process(input.block(17), output.block(17)) == Status::ok && ok;
        scheduled.reset();
        ok = progressiveMatrix.process(input.block(17), output.block(17)) == Status::ok && ok;
        progressiveMatrix.reset();
        matrix.reset();
        changing.reset();
        asynchronous.reset();
        resampler.reset();
        mixing.reset();
        graph.reset();
        fft.reset();
    }
    require(ok, "realtime audit processing status");
    require(allocations == 0 && deallocations == 0,
            "allocation or free inside prepared processing/activation/reset");
}
} // namespace
void realtimeRegression() {
    audit<float>();
    audit<double>();
}

void runRealtimeAudit(void (*function)(void*) noexcept, void* context) {
    {
        Scope scope;
        function(context);
    }
    require(allocations == 0 && deallocations == 0, "allocation/free in expanded processing/reset");
}

void assertNoRealtimeAllocations(void (*callback)(void*) noexcept, void* context) {
    {
        Scope guard;
        callback(context);
    }
    require(allocations == 0 && deallocations == 0,
            "plugin/bridge allocated during realtime processing");
}

void variableSincRealtimeRegression() {
    using namespace cydsp;
    SincInterpolator<double> sinc;
    require(sinc.prepare({48000, 32, 1}, 96000) == Status::ok, "sinc positions prepare");
    AudioBuffer<double> source, output;
    require(source.resize(1, 256) == Status::ok && output.resize(1, 32) == Status::ok,
            "sinc position buffers");
    std::fill(source.block().channel(0).begin(), source.block().channel(0).end(), 1.);
    std::array<double, 32> positions;
    for (std::size_t i = 0; i < 32; ++i)
        positions[i] = 64 + double(i) * 1.5;
    Status result;
    {
        Scope scope;
        result = sinc.processPositions(source.block(), positions, output.block());
    }
    require(result == Status::ok && allocations == 0 && deallocations == 0,
            "variable sinc realtime");
    for (double sample : output.block().channel(0))
        require(std::abs(sample - 1) < 1e-12, "variable sinc unity");
}
