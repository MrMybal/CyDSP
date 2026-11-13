#include <array>
#include <cmath>
#include <cydsp/CyDSP.h>
#include <numbers>
#include <stdexcept>
#include <vector>
void runRealtimeAudit(void (*)(void*) noexcept, void*);
namespace {
using namespace cydsp;
void require(bool v, const char* m) {
    if (!v)
        throw std::runtime_error(m);
}
void ok(Status s) {
    require(s == Status::ok, "restoration diagnostics status");
}
void response() {
    const std::array<double, 3> delay{0, 0, 1};
    FilterResponse result;
    ok(firResponse(delay, 48000, 3000, result));
    require(std::abs(result.gain - std::polar(1., -std::numbers::pi / 4)) < 1e-14 &&
                std::abs(result.groupDelayFrames - 2) < 1e-14,
            "analytic FIR phase and group delay");
    BiquadCoefficients c;
    ok(designBiquad(BiquadType::lowPass, 48000, 1000, .707, c));
    ok(iirResponse({&c, 1}, 48000, 1000, result));
    require(std::abs(std::abs(result.gain) - .707) < 1e-10 && result.groupDelayFrames > 0,
            "analytic IIR response");
}
template <class T> void test() {
    const ProcessSpec spec{8000, 128, 1};
    AudioBuffer<T> in, out, desired, predicted;
    ok(in.resize(1, 128));
    ok(out.resize(1, 128));
    ok(desired.resize(1, 128));
    ok(predicted.resize(1, 128));
    AdaptiveFilter<T> adaptive;
    ok(adaptive.prepare(spec, 8));
    AdaptiveParameters ap;
    ap.step = .5;
    ok(adaptive.setParameters(ap));
    std::uint64_t random = 123;
    double previous1 = 0, previous2 = 0, lastError = 0;
    for (unsigned b = 0; b < 80; ++b) {
        for (std::size_t i = 0; i < 128; ++i) {
            random ^= random >> 12;
            random ^= random << 25;
            random ^= random >> 27;
            const double x = double((random * 2685821657736338717ULL) >> 11) * 0x1.0p-52 - 1;
            in.block().channel(0)[i] = T(x);
            desired.block().channel(0)[i] = T(.5 * x + .25 * previous1 - .1 * previous2);
            previous2 = previous1;
            previous1 = x;
        }
        ok(adaptive.process(in.block(), desired.block(), predicted.block(), out.block()));
        if (b > 75)
            for (T x : out.block().channel(0))
                lastError += double(x) * x;
    }
    const auto weights = adaptive.coefficients(0);
    require(std::abs(weights[0] - .5) < 1e-6 && std::abs(weights[1] - .25) < 1e-6 &&
                std::abs(weights[2] + .1) < 1e-6 && lastError < 1e-10,
            "NLMS independent FIR identification");
    FirRateConverter<T> down, up;
    ok(down.prepare(spec, 4, RateConversion::decimate));
    ok(up.prepare(spec, 4, RateConversion::interpolate));
    AudioBuffer<T> rateOut;
    ok(rateOut.resize(1, 512));
    double stopEnergy = 0;
    for (unsigned b = 0; b < 15; ++b) {
        for (std::size_t i = 0; i < 128; ++i)
            in.block().channel(0)[i] =
                T(std::sin(2 * std::numbers::pi * 2400 * double(b * 128 + i) / 8000));
        ok(down.process(in.block(), rateOut.block(down.outputFrames(128))));
        if (b > 5)
            for (std::size_t i = 0; i < 32; ++i)
                stopEnergy += double(rateOut.block().channel(0)[i]) * rateOut.block().channel(0)[i];
    }
    require(stopEnergy < 1e-6, "FIR decimator anti-alias rejection");
    for (auto& x : in.block().channel(0))
        x = 1;
    for (int b = 0; b < 3; ++b)
        ok(up.process(in.block(), rateOut.block(512)));
    for (T x : rateOut.block().channel(0))
        require(std::abs(double(x) - 1) < .001, "polyphase interpolation DC gain");
    DyadicFilterBank<T> bank;
    ok(bank.prepare(spec, 8));
    AudioBuffer<T> bands, reconstructed;
    ok(bands.resize(8, 16));
    ok(reconstructed.resize(1, 128));
    for (std::size_t i = 0; i < 128; ++i)
        in.block().channel(0)[i] = T(std::sin(.13 * i) + .1 * std::cos(.7 * i));
    ok(bank.analyze(in.block(), bands.block()));
    ok(bank.synthesize(bands.block(), reconstructed.block()));
    for (std::size_t i = 0; i < 128; ++i)
        require(std::abs(double(in.block().channel(0)[i]) - reconstructed.block().channel(0)[i]) <
                    1e-6,
                "orthogonal multirate perfect reconstruction");
    Dehum<T> hum;
    ok(hum.prepare(spec));
    ok(hum.setParameters({50, 30, 60, 4}));
    double humEnergy = 0;
    for (unsigned b = 0; b < 160; ++b) {
        for (std::size_t i = 0; i < 128; ++i)
            in.block().channel(0)[i] =
                T(std::sin(2 * std::numbers::pi * 50 * double(b * 128 + i) / 8000));
        ok(hum.process(in.block(), out.block()));
        if (b > 150)
            for (T x : out.block().channel(0))
                humEnergy += double(x) * x;
    }
    require(humEnergy < .001, "50Hz hum attenuation");
    Declicker<T> click;
    ok(click.prepare(spec, 8));
    for (auto& x : in.block().channel(0))
        x = T(.1);
    in.block().channel(0)[40] = T(1);
    ok(click.process(in.block(), out.block()));
    require(std::abs(double(out.block().channel(0)[48]) - .1) < 1e-6 &&
                click.repairedSamples() == 1,
            "isolated click repair");
    SpectralDenoiser<T> denoise;
    ok(denoise.prepare(spec, {128, 32, WindowType::hann}));
    DenoiseParameters np;
    np.reductionDb = 20;
    np.smoothingMs = 0;
    ok(denoise.setParameters(np));
    std::vector<double> profile(65, 1e6);
    ok(denoise.setNoiseProfile(profile));
    for (unsigned b = 0; b < 3; ++b) {
        for (auto& x : in.block().channel(0))
            x = 0;
        if (b == 0)
            in.block().channel(0)[17] = 1;
        ok(denoise.process(in.block(), out.block()));
        for (std::size_t i = 0; i < 128; ++i)
            require(std::abs(double(out.block().channel(0)[i]) - (b * 128 + i == 145 ? .1 : 0.)) <
                        2e-6,
                    "denoiser spectral gain floor");
    }
    SpectralDenoiser<T> resonance;
    ok(resonance.prepare(spec, {256, 64, WindowType::hann}));
    DenoiseParameters rp;
    rp.reductionDb = 0;
    rp.strength = 0;
    rp.smoothingMs = 0;
    rp.resonanceThresholdDb = 6;
    rp.resonanceReductionDb = 12;
    rp.persistenceMs = 200;
    ok(resonance.setParameters(rp));
    double beforePersistent = 0, afterPersistent = 0;
    for (unsigned b = 0; b < 32; ++b) {
        for (std::size_t i = 0; i < 128; ++i)
            in.block().channel(0)[i] =
                T(.1 * std::sin(2 * std::numbers::pi * 500 * double(b * 128 + i) / 8000));
        ok(resonance.process(in.block(), out.block()));
        if (b >= 6 && b < 8)
            for (T x : out.block().channel(0))
                beforePersistent += double(x) * x;
        if (b >= 24 && b < 26)
            for (T x : out.block().channel(0))
                afterPersistent += double(x) * x;
    }
    require(beforePersistent > .5 && afterPersistent < beforePersistent * .1,
            "persistent resonance dwell and attenuation");
    TransferAnalyzer<T> transfer;
    ok(transfer.prepare(spec, 256, 64, 8));
    for (unsigned b = 0; b < 10; ++b) {
        for (std::size_t i = 0; i < 128; ++i) {
            const double t = double(b * 128 + i);
            in.block().channel(0)[i] = T(std::sin(.03 * t) + .3 * std::cos(.731 * t));
            desired.block().channel(0)[i] = T(in.block().channel(0)[i] * T(.5));
        }
        ok(transfer.process(in.block(), desired.block()));
    }
    for (std::size_t k = 0; k < transfer.bins(); ++k) {
        const auto bin = transfer.bin(0, k);
        require(std::abs(bin.gain - std::complex<double>(.5, 0)) < 1e-10 &&
                    bin.coherence > 1 - 1e-12 && std::abs(bin.groupDelayFrames) < 1e-9,
                "Welch transfer gain phase coherence");
    }
    std::vector<T> tone(2048);
    for (std::size_t i = 0; i < tone.size(); ++i) {
        const double phase = 2 * std::numbers::pi * 997 * double(i) / 48000;
        tone[i] = T(.1 + .5 * std::sin(phase) + .05 * std::sin(2 * phase));
    }
    DistortionReport distortion;
    ok(measureDistortion<T>(tone, 48000, 997, distortion, 5));
    require(std::abs(distortion.thd - .1) < 1e-6 && std::abs(distortion.dc - .1) < 1e-7 &&
                distortion.noiseRms < 1e-7,
            "noncoherent THD least squares");
    MeasurementGenerator<T> generator;
    ok(generator.prepare(spec));
    MeasurementSignalParameters signal;
    signal.startHz = 100;
    signal.endHz = 3000;
    signal.durationSeconds = .032;
    signal.fadeSeconds = 0;
    ok(generator.setParameters(signal));
    std::vector<T> sweep(256), inverse(256), recording(258), impulse(16);
    for (unsigned b = 0; b < 2; ++b) {
        ok(generator.process(out.block()));
        std::copy(out.block().channel(0).begin(), out.block().channel(0).end(),
                  sweep.begin() + b * 128);
    }
    ok(inverseLogSweep<T>(sweep, 100, 3000, inverse));
    double peak = 0;
    for (std::size_t i = 0; i < 256; ++i)
        peak += double(sweep[i]) * inverse[255 - i];
    require(std::abs(peak - 1) < 1e-6, "inverse sweep normalization");
    for (std::size_t i = 0; i < sweep.size(); ++i) {
        recording[i] += sweep[i];
        recording[i + 1] += T(.5) * sweep[i];
        recording[i + 2] -= T(.25) * sweep[i];
    }
    ok(deconvolveSweep<T>(recording, sweep, impulse, -180));
    for (std::size_t i = 0; i < impulse.size(); ++i)
        require(std::abs(double(impulse[i]) - (i == 0   ? 1
                                               : i == 1 ? .5
                                               : i == 2 ? -.25
                                                        : 0)) < 1e-5,
                "sweep deconvolution known impulse");
    struct Audit {
        AdaptiveFilter<T>* a;
        Dehum<T>* h;
        Declicker<T>* c;
        SpectralDenoiser<T>* n;
        TransferAnalyzer<T>* t;
        FirRateConverter<T>* r;
        AudioBlock<const T> in, desired;
        AudioBlock<T> pred, out, rate;
        bool pass = true;
    } audit{&adaptive,         &hum,        &click,           &denoise,
            &transfer,         &down,       in.block(),       desired.block(),
            predicted.block(), out.block(), rateOut.block(32)};
    runRealtimeAudit(
        [](void* raw) noexcept {
            auto& c = *static_cast<Audit*>(raw);
            c.pass = c.a->process(c.in, c.desired, c.pred, c.out) == Status::ok &&
                     c.h->process(c.in, c.out) == Status::ok &&
                     c.c->process(c.in, c.out) == Status::ok &&
                     c.n->process(c.in, c.out) == Status::ok &&
                     c.t->process(c.in, c.desired) == Status::ok &&
                     c.r->process(c.in, c.rate) == Status::ok;
            c.a->reset();
            c.h->reset();
            c.c->reset();
            c.n->reset();
            c.t->reset();
            c.r->reset();
        },
        &audit);
    require(audit.pass, "restoration diagnostics realtime");
}
} // namespace
void restorationDiagnosticsRegression() {
    response();
    test<float>();
    test<double>();
}
