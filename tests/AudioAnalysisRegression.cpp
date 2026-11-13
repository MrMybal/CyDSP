#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <cydsp/CyDSP.h>
#include <iostream>
#include <limits>
#include <numbers>
#include <numeric>
#include <source_location>
#include <stdexcept>
#include <vector>
void runRealtimeAudit(void (*)(void*) noexcept, void*);
namespace {
using namespace cydsp;
void require(bool v, const char* why,
             std::source_location where = std::source_location::current()) {
    if (!v)
        throw std::runtime_error(std::string(why) + " line " + std::to_string(where.line()));
}
void ok(Status s, std::source_location where = std::source_location::current()) {
    require(s == Status::ok, "audio analysis status", where);
}
constexpr std::array backends{CpuBackend::scalar, CpuBackend::sse2, CpuBackend::avx2,
                              CpuBackend::avx512, CpuBackend::neon, CpuBackend::sve};
template <class F> void chunks(std::size_t frames, std::size_t maximum, F run) {
    const std::array<std::size_t, 5> sizes{1, 7, 113, 31, 509};
    std::size_t at = 0, block = 0;
    while (at < frames) {
        const auto count = std::min({sizes[block++ % sizes.size()], frames - at, maximum});
        run(at, count);
        at += count;
    }
}
template <class T> void levels() {
    LevelMeter<T> meter;
    ok(meter.prepare({48000, 128, 2}, 257));
    AudioBuffer<T> data;
    ok(data.resize(2, 2053));
    for (std::size_t ch = 0; ch < 2; ++ch)
        for (std::size_t i = 0; i < 2053; ++i)
            data.block().channel(ch)[i] =
                T(.3 * std::sin(double(i) * .17) + .08 * double(int((i + ch * 3) % 11) - 5));
    chunks(2053, 128, [&](std::size_t at, std::size_t count) {
        ok(meter.process(data.block().subBlock(at, count)));
        const auto end = at + count, first = end > 257 ? end - 257 : 0;
        std::array<long double, 2> squares{};
        long double cross = 0;
        for (std::size_t ch = 0; ch < 2; ++ch) {
            long double sum = 0;
            double peak = 0, held = 0;
            for (std::size_t i = 0; i < end; ++i)
                held = std::max(held, std::abs(double(data.block().channel(ch)[i])));
            for (std::size_t i = first; i < end; ++i) {
                const auto x = static_cast<long double>(data.block().channel(ch)[i]);
                sum += x;
                squares[ch] += x * x;
                peak = std::max(peak, std::abs(double(x)));
            }
            const auto measured = meter.levels(ch);
            require(measured.peak == peak && measured.heldPeak == held, "sliding peak oracle");
            require(std::abs(measured.mean - double(sum / static_cast<long double>(end - first))) <
                        1e-12,
                    "mean oracle");
            require(std::abs(measured.rms -
                             std::sqrt(double(squares[ch] /
                                              static_cast<long double>(end - first)))) < 1e-12,
                    "RMS oracle");
        }
        for (std::size_t i = first; i < end; ++i)
            cross +=
                static_cast<long double>(data.block().channel(0)[i]) * data.block().channel(1)[i];
        require(std::abs(meter.stereoCorrelation() -
                         double(cross / std::sqrt(squares[0] * squares[1]))) < 1e-12,
                "stereo correlation oracle");
    });
    const auto saved = meter.levels(0);
    data.block().channel(1)[127] = std::numeric_limits<T>::quiet_NaN();
    require(meter.process(data.block(128)) == Status::invalid_argument &&
                meter.levels(0).rms == saved.rms,
            "nonfinite input changed meter");
    data.block().channel(1)[127] = 0;
    ok(meter.process(data.block(0)));
    require(meter.levels(0).rms == saved.rms, "empty level block");
    require(meter.prepare({0, 128, 2}) == Status::invalid_spec && meter.windowFrames() == 257,
            "level prepare transaction");
    auto moved = std::move(meter);
    require(meter.process(data.block(1)) == Status::not_prepared, "moved level meter");
    moved.reset();
    require(moved.levels(0).rms == 0 && std::isinf(moved.levels(0).peakDbfs()),
            "level reset silence");
}
template <class T> void lowLevelPrecision() {
    LevelMeter<T> meter;
    ok(meter.prepare({48000, 1, 1}, 1));
    T value = T(1);
    const T* pointer[]{&value};
    ok(meter.process({pointer, 1, 1}));
    value = T(1e-9);
    ok(meter.process({pointer, 1, 1}));
    require(std::abs(meter.levels(0).rms - double(value)) < 1e-20,
            "small energy lost when large sample leaves window");
}
template <class T> void spectrum(CpuBackend backend) {
    SpectrumAnalyzer<T> p;
    SpectrumConfig config;
    config.fftFrames = 64;
    config.hopFrames = 17;
    ok(p.prepare({48000, 128, 2}, config, backend));
    AudioBuffer<T> input;
    ok(input.resize(2, 1001));
    for (std::size_t ch = 0; ch < 2; ++ch)
        for (std::size_t i = 0; i < 1001; ++i)
            input.block().channel(ch)[i] =
                T(.7 * std::sin(2 * std::numbers::pi * 5 * double(i) / 64 + double(ch)) +
                  .21 * std::cos(2 * std::numbers::pi * 11 * double(i) / 64));
    chunks(1001, 128,
           [&](auto at, auto count) { ok(p.process(input.block().subBlock(at, count))); });
    const auto expectedFrames = 1 + (1001 - 64) / 17, end = 64 + (expectedFrames - 1) * 17;
    require(p.frameCount() == expectedFrames && p.hasFrame() && p.bins() == 33, "FFT hop timing");
    for (std::size_t ch = 0; ch < 2; ++ch) {
        double weightedEnergy = 0, windowSquares = 0, rawEnergy = 0;
        for (std::size_t i = 0; i < 64; ++i) {
            const double w = .5 - .5 * std::cos(2 * std::numbers::pi * double(i) / 64),
                         x = input.block().channel(ch)[end - 64 + i];
            weightedEnergy += x * x * w * w;
            windowSquares += w * w;
            rawEnergy += x * x;
        }
        for (std::size_t k = 0; k < 33; ++k) {
            std::complex<double> dft{};
            for (std::size_t i = 0; i < 64; ++i)
                dft += double(input.block().channel(ch)[end - 64 + i]) *
                       (.5 - .5 * std::cos(2 * std::numbers::pi * double(i) / 64)) *
                       std::polar(1., -2 * std::numbers::pi * double(k * i) / 64);
            require(std::abs(p.complexSpectrum(ch)[k] - dft) < 2e-11,
                    "independent spectrum DFT oracle");
            require(std::abs(p.magnitudes(ch)[k] -
                             std::abs(dft) * (k == 0 || k == 32 ? 1 : 2) / 32) < 1e-12,
                    "spectrum amplitude scaling");
        }
        const double power =
            std::accumulate(p.powerDensity(ch).begin(), p.powerDensity(ch).end(), 0.) * 48000 / 64;
        require(std::abs(power - weightedEnergy / windowSquares) < 1e-12,
                "PSD Parseval normalization");
        require(std::abs(p.features(ch).rms - std::sqrt(rawEnergy / 64)) < 1e-12, "spectral RMS");
        require(p.features(ch).dominantHz == 3750 && p.features(ch).flatness >= 0 &&
                    p.features(ch).flatness <= 1,
                "spectral feature bounds");
    }
    const auto count = p.frameCount();
    input.block().channel(1)[4] = std::numeric_limits<T>::infinity();
    require(p.process(input.block(16)) == Status::invalid_argument && p.frameCount() == count,
            "spectrum error mutation");
    input.block().channel(1)[4] = 0;
    config.fftFrames = 63;
    require(p.prepare({48000, 128, 2}, config, backend) == Status::invalid_spec && p.bins() == 33,
            "FFT plan transaction");
    auto moved = std::move(p);
    require(p.process(input.block(1)) == Status::not_prepared && moved.hasFrame(), "spectrum move");
    moved.reset();
    require(!moved.hasFrame() && std::all_of(moved.magnitudes(0).begin(), moved.magnitudes(0).end(),
                                             [](double x) { return x == 0; }),
            "spectrum reset");
}
void bands(CpuBackend backend) {
    for (auto scale : {SpectralBandScale::mel, SpectralBandScale::logarithmic}) {
        SpectralBands p;
        SpectralBandConfig config;
        config.bands = 12;
        config.minimumHz = 80;
        config.scale = scale;
        ok(p.prepare(48000, 2048, config, backend));
        std::vector<double> input(1025, 1), output(12);
        ok(p.process(input, output));
        for (double x : output)
            require(std::abs(x - 1) < 1e-13, "normalized band flat-response oracle");
        const auto frequencies = p.centerFrequencies();
        require(frequencies.size() == 12 && std::is_sorted(frequencies.begin(), frequencies.end()),
                "band frequency order");
        std::fill(input.begin(), input.end(), 0);
        input[100] = 1;
        ok(p.process(input, output));
        require(
            std::all_of(output.begin(), output.end(), [](double x) { return x >= 0 && x <= 1; }),
            "band impulse weights");
        const auto saved = output;
        input[5] = -1;
        require(p.process(input, output) == Status::invalid_argument && output == saved,
                "band input transaction");
        config.bands = 0;
        require(p.prepare(48000, 2048, config, backend) == Status::invalid_argument &&
                    p.centerFrequencies().size() == 12,
                "band prepare transaction");
    }
}
template <class T> void constantQ(CpuBackend backend) {
    ConstantQAnalyzer<T> p;
    ConstantQConfig config;
    config.minimumHz = 480;
    config.bins = 12;
    config.hopFrames = 37;
    ok(p.prepare({8000, 128, 1}, config, backend));
    AudioBuffer<T> input;
    ok(input.resize(1, 1024));
    const double tone = p.centerFrequencies()[3];
    for (std::size_t i = 0; i < 1024; ++i)
        input.block().channel(0)[i] =
            T(.6 * std::cos(2 * std::numbers::pi * tone * double(i) / 8000));
    chunks(1024, 128,
           [&](auto at, auto count) { ok(p.process(input.block().subBlock(at, count))); });
    const auto largest = p.windowFrames(0), frames = 1 + (1024 - largest) / 37,
               end = largest + (frames - 1) * 37;
    require(p.hasFrame() && p.frameCount() == frames, "CQT hop timing");
    for (std::size_t k = 0; k < 12; ++k) {
        const auto n = p.windowFrames(k);
        std::complex<double> sum{};
        double weight = 0;
        for (std::size_t i = 0; i < n; ++i) {
            const double w = .5 - .5 * std::cos(2 * std::numbers::pi * double(i) / double(n - 1));
            sum +=
                double(input.block().channel(0)[end - n + i]) * w *
                std::polar(1., -2 * std::numbers::pi * p.centerFrequencies()[k] * double(i) / 8000);
            weight += w;
        }
        require(std::abs(p.magnitudes(0)[k] - 2 * std::abs(sum) / weight) < 2e-12,
                "independent variable-window CQT oracle");
    }
    const auto amplitudes = p.magnitudes(0);
    require(std::size_t(std::max_element(amplitudes.begin(), amplitudes.end()) -
                        amplitudes.begin()) == 3 &&
                std::abs(amplitudes[3] - .6) < .001,
            "CQT tone calibration");
    config.maximumKernelFrames = 4;
    require(p.prepare({8000, 128, 1}, config, backend) == Status::invalid_argument && p.hasFrame(),
            "CQT prepare transaction");
    p.reset();
    require(!p.hasFrame(), "CQT reset");
}
template <class T> void truePeak(CpuBackend backend, unsigned factor) {
    TruePeakMeter<T> a, b;
    for (auto* p : {&a, &b})
        ok(p->prepare({48000, 256, 1}, factor, 64, backend));
    AudioBuffer<T> input;
    ok(input.resize(1, 8192));
    for (std::size_t i = 0; i < 8192; ++i) {
        const double fade = std::min({1., double(i) / 480, double(8191 - i) / 480});
        input.block().channel(0)[i] =
            T(.5 * fade * std::sin(2 * std::numbers::pi * .25 * double(i) + std::numbers::pi / 4));
    }
    for (std::size_t at = 0; at < 8192; at += 256)
        ok(a.process(input.block().subBlock(at, 256)));
    chunks(8192, 256,
           [&](auto at, auto count) { ok(b.process(input.block().subBlock(at, count))); });
    a.flush();
    b.flush();
    require(std::abs(a.samplePeak(0) - std::sqrt(.125)) < 2e-8, "sample peak calibration");
    require(std::abs(a.truePeak(0) - .5) < .001 && a.truePeak(0) > a.samplePeak(0) * 1.4,
            "intersample peak analytical fixture");
    require(a.truePeak(0) == b.truePeak(0) && a.samplePeak(0) == b.samplePeak(0),
            "true peak split invariance");
    require(std::abs(a.truePeakDbtp(0) - 20 * std::log10(.5)) < .02, "dBTP calibration");
    const auto saved = a.truePeak(0);
    input.block().channel(0)[10] = std::numeric_limits<T>::quiet_NaN();
    require(a.process(input.block(16)) == Status::invalid_argument && a.truePeak(0) == saved,
            "true peak input transaction");
    a.reset();
    require(a.truePeak(0) == 0 && std::isinf(a.truePeakDbtp(0)), "true peak reset");
}
struct DirectFilter {
    std::array<double, 3> b{};
    std::array<double, 2> a{}, x{}, y{};
    double step(double value) {
        const double result = b[0] * value + b[1] * x[0] + b[2] * x[1] - a[0] * y[0] - a[1] * y[1];
        x[1] = x[0];
        x[0] = value;
        y[1] = y[0];
        y[0] = result;
        return result;
    }
};
double referenceLoudness(std::span<const double> energies, double relative) {
    const double absolute = std::pow(10., (-70 + .691) / 10);
    double sum = 0;
    std::size_t count = 0;
    for (double e : energies)
        if (e > absolute) {
            sum += e;
            ++count;
        }
    if (!count)
        return -std::numeric_limits<double>::infinity();
    const double gate = std::max(absolute, sum / double(count) * relative);
    sum = 0;
    count = 0;
    for (double e : energies)
        if (e > gate) {
            sum += e;
            ++count;
        }
    return count ? -.691 + 10 * std::log10(sum / double(count))
                 : -std::numeric_limits<double>::infinity();
}
template <class T> void loudness() {
    constexpr std::size_t frames = 48000 * 7;
    AudioBuffer<T> input;
    ok(input.resize(2, frames));
    std::vector<double> power(frames);
    std::array<DirectFilter, 2> shelf, highpass;
    for (std::size_t ch = 0; ch < 2; ++ch) {
        shelf[ch].b = {1.53512485958697, -2.69169618940638, 1.19839281085285};
        shelf[ch].a = {-1.69065929318241, .73248077421585};
        highpass[ch].b = {1, -2, 1};
        highpass[ch].a = {-1.99004745483398, .99007225036621};
    }
    for (std::size_t i = 0; i < frames; ++i) {
        const double amplitude = i < 96000 ? .2 : i < 192000 ? .000002 : .02;
        for (std::size_t ch = 0; ch < 2; ++ch) {
            const T sample = T(amplitude * std::sin(2 * std::numbers::pi * 997 * double(i) / 48000 +
                                                    double(ch) * .3));
            input.block().channel(ch)[i] = sample;
            const auto weighted = highpass[ch].step(shelf[ch].step(double(sample)));
            power[i] += weighted * weighted;
        }
    }
    std::vector<double> blocks, shortBlocks;
    for (std::size_t end = 19200; end <= frames; end += 4800)
        blocks.push_back(std::accumulate(power.begin() + static_cast<std::ptrdiff_t>(end - 19200),
                                         power.begin() + static_cast<std::ptrdiff_t>(end), 0.) /
                         19200);
    for (std::size_t end = 144000; end <= frames; end += 4800)
        shortBlocks.push_back(
            std::accumulate(power.begin() + static_cast<std::ptrdiff_t>(end - 144000),
                            power.begin() + static_cast<std::ptrdiff_t>(end), 0.) /
            144000);
    LoudnessMeter<T> p, split;
    LoudnessConfig config;
    config.maximumGatingBlocks = 128;
    for (auto* meter : {&p, &split})
        ok(meter->prepare({48000, 512, 2}, config));
    for (std::size_t at = 0; at < frames; at += 512)
        ok(p.process(input.block().subBlock(at, std::min<std::size_t>(512, frames - at))));
    chunks(frames, 512,
           [&](auto at, auto count) { ok(split.process(input.block().subBlock(at, count))); });
    const auto expected = referenceLoudness(blocks, .1), measured = p.integratedLufs();
    require(std::abs(measured - expected) < 1e-7,
            "BS.1770 independent direct-form filter/gating oracle");
    require(p.gatingBlocks() == blocks.size() && split.gatingBlocks() == blocks.size() &&
                p.integratedLufs() == split.integratedLufs(),
            "loudness split invariance");
    const double shortExpected = -.691 + 10 * std::log10(shortBlocks.back()),
                 momentaryExpected = -.691 + 10 * std::log10(blocks.back());
    require(std::abs(p.levels().shortTermLufs - shortExpected) < 1e-7 &&
                std::abs(p.levels().momentaryLufs - momentaryExpected) < 1e-7,
            "M/S rolling windows");
    // Reference quantiles of the absolute- and -20 LU-relative-gated short history.
    const double absolute = std::pow(10., (-70 + .691) / 10);
    double sum = 0;
    std::size_t eligible = 0;
    for (double value : shortBlocks)
        if (value > absolute) {
            sum += value;
            ++eligible;
        }
    const double threshold = std::max(absolute, sum / double(eligible) * .01);
    std::vector<double> values;
    for (double value : shortBlocks)
        if (value > threshold)
            values.push_back(-.691 + 10 * std::log10(value));
    std::sort(values.begin(), values.end());
    auto quantile = [&](double fraction) {
        const double index = fraction * double(values.size() - 1);
        const auto first = std::size_t(index);
        return values[first] + (values[std::min(first + 1, values.size() - 1)] - values[first]) *
                                   (index - double(first));
    };
    require(std::abs(p.loudnessRangeLu() - (quantile(.95) - quantile(.1))) < 1e-7,
            "LRA gate/quantile oracle");
    std::cout << "QUALITY loudness oracle absolute LUFS error=" << std::abs(measured - expected)
              << '\n';
    p.reset();
    require(std::isinf(p.integratedLufs()) && !p.levels().momentaryReady &&
                p.loudnessRangeLu() == 0,
            "loudness reset");
    // A 997 Hz -20 dBFS peak mono tone indicates approximately -23.01 LUFS.
    LoudnessMeter<T> calibrated;
    ok(calibrated.prepare({48000, 512, 1}, config));
    AudioBuffer<T> tone;
    ok(tone.resize(1, 48000 * 4));
    for (std::size_t i = 0; i < tone.block().frames(); ++i)
        tone.block().channel(0)[i] =
            T(.1 * std::sin(2 * std::numbers::pi * 997 * double(i) / 48000));
    chunks(tone.block().frames(), 512,
           [&](auto at, auto count) { ok(calibrated.process(tone.block().subBlock(at, count))); });
    require(std::abs(calibrated.levels().shortTermLufs + 23.0103) < .02 &&
                calibrated.loudnessRangeLu() < .01,
            "997 Hz loudness calibration");
    LoudnessMeter<T> bounded;
    config.maximumGatingBlocks = 1;
    ok(bounded.prepare({8000, 4096, 1}, config));
    std::vector<T> zeros(4096);
    const T* pointer[]{zeros.data()};
    require(bounded.process({pointer, 1, 4096}) == Status::would_block &&
                bounded.gatingBlocks() == 0 && !bounded.levels().momentaryReady,
            "history overflow partially consumed input");
    ok(bounded.process({pointer, 1, 3200}));
    const auto old = bounded.levels();
    require(bounded.gatingBlocks() == 1, "bounded history fill");
    require(bounded.process({pointer, 1, 800}) == Status::would_block &&
                bounded.levels().momentaryLufs == old.momentaryLufs,
            "history full transaction");
    LoudnessMeter<T> surround;
    std::array<double, 6> weights{1, 1, 1, 0, 1.41, 1.41};
    config.maximumGatingBlocks = 32;
    config.channelWeights = weights;
    ok(surround.prepare({48000, 512, 6}, config));
    AudioBuffer<T> lfe;
    ok(lfe.resize(6, 512));
    std::fill(lfe.block().channel(3).begin(), lfe.block().channel(3).end(), T(.5));
    for (unsigned i = 0; i < 40; ++i)
        ok(surround.process(lfe.block()));
    require(std::isinf(surround.integratedLufs()), "LFE energy not excluded");
}
template <class T> void loudnessRates() {
    for (double rate : {44100., 96000.}) {
        LoudnessMeter<T> meter;
        LoudnessConfig config;
        config.maximumGatingBlocks = 64;
        std::array<double, 1> weights{1};
        config.channelWeights = weights;
        ok(meter.prepare({rate, 512, 1}, config));
        weights[0] = 0; // prepare copies the channel layout.
        AudioBuffer<T> input;
        ok(input.resize(1, 512));
        const auto total = std::size_t(rate * 4);
        for (std::size_t at = 0; at < total; at += 512) {
            const auto count = std::min<std::size_t>(512, total - at);
            for (std::size_t i = 0; i < count; ++i)
                input.block().channel(0)[i] =
                    T(.1 * std::sin(2 * std::numbers::pi * 997 * double(at + i) / rate));
            ok(meter.process(input.block(count)));
        }
        require(std::abs(meter.levels().shortTermLufs + 23.0103) < .035,
                "non-48k K-weighting calibration/channel weights ownership");
    }
}

template <class T> void realtime() {
    const ProcessSpec spec{8000, 128, 2};
    LevelMeter<T> level;
    TruePeakMeter<T> peak;
    SpectrumAnalyzer<T> spectrum;
    ConstantQAnalyzer<T> cq;
    LoudnessMeter<T> loudness;
    ok(level.prepare(spec, 31));
    ok(peak.prepare(spec));
    SpectrumConfig sc;
    sc.fftFrames = 64;
    sc.hopFrames = 16;
    ok(spectrum.prepare(spec, sc));
    ConstantQConfig cc;
    cc.minimumHz = 1000;
    cc.bins = 4;
    cc.binsPerOctave = 1; // Last frequency must remain below Nyquist.
    cc.bins = 2;
    ok(cq.prepare(spec, cc));
    LoudnessConfig lc;
    lc.maximumGatingBlocks = 64;
    ok(loudness.prepare(spec, lc));
    AudioBuffer<T> input;
    ok(input.resize(2, 128));
    std::fill(input.block().channel(0).begin(), input.block().channel(0).end(), T(.125));
    struct Context {
        LevelMeter<T>* l;
        TruePeakMeter<T>* p;
        SpectrumAnalyzer<T>* s;
        ConstantQAnalyzer<T>* c;
        LoudnessMeter<T>* m;
        AudioBlock<const T> b;
        bool passed = true;
    };
    Context context{&level, &peak, &spectrum, &cq, &loudness, input.block()};
    runRealtimeAudit(
        [](void* raw) noexcept {
            auto& c = *static_cast<Context*>(raw);
            for (unsigned i = 0; i < 250; ++i) {
                c.passed &= c.l->process(c.b) == Status::ok;
                c.passed &= c.p->process(c.b) == Status::ok;
                c.passed &= c.s->process(c.b) == Status::ok;
                c.passed &= c.c->process(c.b) == Status::ok;
                c.passed &= c.m->process(c.b) == Status::ok;
            }
            (void)c.m->integratedLufs();
            (void)c.m->loudnessRangeLu();
            c.p->flush();
            c.l->reset();
            c.p->reset();
            c.s->reset();
            c.c->reset();
            c.m->reset();
        },
        &context);
    require(context.passed, "audio analysis allocation-free execution");
}
} // namespace
void audioAnalysisRegression() {
    lowLevelPrecision<float>();
    lowLevelPrecision<double>();
    levels<float>();
    levels<double>();
    for (auto backend : backends) {
        VectorOps ops;
        if (VectorOps::create(backend, ops) != Status::ok)
            continue;
        spectrum<float>(backend);
        spectrum<double>(backend);
        bands(backend);
        constantQ<float>(backend);
        constantQ<double>(backend);
        for (unsigned factor : {4u, 8u}) {
            truePeak<float>(backend, factor);
            truePeak<double>(backend, factor);
        }
    }
    loudness<float>();
    loudness<double>();
    loudnessRates<float>();
    loudnessRates<double>();
    realtime<float>();
    realtime<double>();
}
