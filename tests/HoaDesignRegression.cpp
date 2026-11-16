#include <algorithm>
#include <complex>
#include <cydsp/CyDSP.h>
#include <iostream>
#include <limits>
#include <numbers>
#include <stdexcept>
using namespace cydsp;
void runRealtimeAudit(void (*)(void*) noexcept, void*);
namespace {
void require(bool value, const char* message) {
    if (!value)
        throw std::runtime_error(message);
}
void ok(Status s, const char* message) {
    require(s == Status::ok, message);
}
constexpr double pi = std::numbers::pi;
void grid() {
    std::vector<Vec3> dirs(128);
    std::vector<double> weights(128);
    ok(sphericalFibonacci(dirs, weights), "Fibonacci grid");
    double sum = 0;
    Vec3 mean{};
    for (std::size_t i = 0; i < dirs.size(); ++i) {
        require(std::abs(length(dirs[i]) - 1) < 3e-15, "unit spherical grid");
        sum += weights[i];
        mean = mean + dirs[i] * (1. / 128);
    }
    require(std::abs(sum - 4 * pi) < 5e-14 && length(mean) < .001,
            "approximate spherical area and balance");
    const auto saved = dirs;
    require(sphericalFibonacci(dirs, {weights.data(), 127}) == Status::invalid_argument &&
                dirs[0].x == saved[0].x,
            "grid preflight preserves output");
}
template <class T> void analytic() {
    const std::array<Vec3, 3> dirs{{{1, 0, 0}, {0, 1, 0}, {0, -1, 0}}};
    AnalyticHrirOptions config;
    config.taps = 192;
    HrirBank<T> bank;
    AnalyticHrirReport report;
    ok(analyticHrirs(config, dirs, bank, &report), "generate analytic HRIRs");
    require(bank.points().size() == 3 && bank.taps() == 192 && bank.sampleRate() == 48000,
            "owning HRIR bank");
    const auto front = bank.points()[0], left = bank.points()[1], right = bank.points()[2];
    for (std::size_t i = 0; i < config.taps; ++i) {
        require(front.left[i] == front.right[i], "analytic front symmetry");
        require(std::abs(double(left.left[i]) - double(right.right[i])) < 1e-7 &&
                    std::abs(double(left.right[i]) - double(right.left[i])) < 1e-7,
                "analytic lateral mirror");
    }
    AnalyticBinaural<T> model;
    ok(model.prepare({48000, 64, 1}), "analytic model");
    const BinauralEmitter emitter{{0, 1, 0}, 1, 0};
    ok(model.setEmitters({&emitter, 1}, 0), "analytic pose");
    AudioBuffer<T> in, out;
    ok(in.resize(1, 64), "analytic input");
    ok(out.resize(2, 64), "analytic output");
    in.block().channel(0)[0] = 1;
    for (std::size_t block = 0; block < 3; ++block) {
        ok(model.process(in.block(), out.block()), "analytic direct model impulse");
        for (std::size_t j = 0; j < 64; ++j)
            require(out.block().channel(0)[j] == left.left[block * 64 + j] &&
                        out.block().channel(1)[j] == left.right[block * 64 + j],
                    "HRIR helper matches direct streaming model");
        in.block().channel(0)[0] = 0;
    }
    const auto data = bank.points()[0].left.data();
    auto moved = std::move(bank);
    require(bank.points().empty() && moved.points()[0].left.data() == data,
            "HRIR bank move lifetime");
    auto bad = dirs;
    bad[2] = {0, 0, 0};
    require(analyticHrirs(config, bad, moved) != Status::ok &&
                moved.points()[0].left.data() == data,
            "analytic failure preserves owning bank");
    config.maximumStorageBytes = 16;
    require(analyticHrirs(config, dirs, moved) == Status::invalid_spec, "analytic storage budget");
    HrtfSet<T> shared;
    HrtfPreparation prep;
    prep.detectOnsets = false;
    ok(shared.prepare(moved.points(), prep), "analytic bank feeds shared HRTF");
    HrirBinaural<T> hrir;
    ok(hrir.prepare({48000, 64, 1}, shared), "analytic bank feeds direct HRIR renderer");
    in.block().channel(0)[0] = 1;
    ok(hrir.setDirection({0, 1, 0}, 0), "analytic HRIR direction");
    ok(hrir.process(in.block(), out.block()), "analytic FIR playback");
    for (std::size_t j = 0; j < 64; ++j)
        require(std::abs(double(out.block().channel(0)[j]) - double(left.left[j])) < 1e-7,
                "analytic generated FIR playback");
}
template <class T>
std::vector<T> impulse(HoaBinaural<T>& render, unsigned order, Vec3 direction, std::size_t frames,
                       std::size_t split = 64) {
    const auto channels = (order + 1) * (order + 1);
    AudioBuffer<T> in, out;
    ok(in.resize(channels, 64), "HOA input");
    ok(out.resize(2, 64), "HOA output");
    std::array<double, 64> y{};
    ok(ambisonicCoefficients(order, direction, {y.data(), channels}), "HOA encoder");
    std::vector<T> signal(2 * frames);
    render.reset();
    for (std::size_t offset = 0; offset < frames;) {
        const auto n = std::min(split, frames - offset);
        for (std::size_t ch = 0; ch < channels; ++ch) {
            std::fill(in.block().channel(ch).begin(), in.block().channel(ch).end(), T{});
            if (offset == 0)
                in.block().channel(ch)[0] = T(y[ch]);
        }
        ok(render.process(in.block(n), out.block(n)), "HOA impulse rendering");
        for (unsigned ear = 0; ear < 2; ++ear)
            std::copy_n(out.block(n).channel(ear).begin(), n,
                        signal.begin() + ear * frames + offset);
        offset += n;
    }
    return signal;
}
template <class T> std::complex<double> frequency(std::span<const T> h, double cycles) {
    std::complex<double> value{};
    for (std::size_t j = 0; j < h.size(); ++j)
        value += double(h[j]) * std::polar(1., -2 * pi * cycles * double(j));
    return value;
}
template <class T> void decoder() {
    constexpr std::size_t count = 96, n = 256;
    std::vector<Vec3> directions(count);
    std::vector<double> weights(count);
    ok(sphericalFibonacci(directions, weights), "design grid");
    std::vector<T> amplitudes(2 * count);
    std::vector<HrirPoint<T>> points(count);
    for (std::size_t i = 0; i < count; ++i) {
        const auto d = directions[i];
        amplitudes[2 * i] = T(1 + .25 * d.y);
        amplitudes[2 * i + 1] = T(1 - .25 * d.y);
        points[i] = {d,
                     {amplitudes.data() + 2 * i, 1},
                     {amplitudes.data() + 2 * i + 1, 1},
                     std::round(6 * (1 - d.y)),
                     std::round(6 * (1 + d.y))};
    }
    HrtfPreparation prep;
    prep.detectOnsets = false;
    HrtfSet<T> bank;
    ok(bank.prepare(points, prep), "synthetic delayed bank");
    for (unsigned order = 1; order <= 3; ++order) {
        const ProcessSpec spec{48000, 64, (order + 1) * (order + 1)};
        HoaDecoderDesign design;
        design.fftSize = n;
        design.iterations = 24;
        design.spatialWeights = weights;
        HoaBinaural<T> ls, mag;
        design.method = HoaDecoderMethod::leastSquares;
        ok(ls.prepare(spec, order, bank, design), "weighted spectral LS");
        design.method = HoaDecoderMethod::magLS;
        ok(mag.prepare(spec, order, bank, design), "MagLS prepare");
        const auto a = ls.designReport(), b = mag.designReport();
        require(b.spectralMetricsAvailable && b.fftSize == n && b.filterTaps == n &&
                    mag.latencyFrames() == n / 2,
                "MagLS causality and length");
        require(b.highBandMagnitudeError < a.highBandMagnitudeError * .75,
                "MagLS magnitude improvement");
        require(std::abs(a.lowBandComplexError - b.lowBandComplexError) < 1e-12,
                "MagLS preserves complex LF design");
        require(std::abs(b.initialHighBandMagnitudeError - a.highBandMagnitudeError) < 1e-12,
                "LS baseline diagnostics");
        std::cout << "QUALITY HOA " << (std::same_as<T, float> ? "float" : "double") << " order "
                  << order << " design HF magnitude RMS LS=" << a.highBandMagnitudeError
                  << " MagLS=" << b.highBandMagnitudeError << "\n";
        // Unseen directions are not part of the fit.
        std::vector<Vec3> holdout(17);
        ok(sphericalFibonacci(holdout), "holdout grid");
        double lsError = 0, magError = 0;
        for (auto d : holdout) {
            d = rotate({std::cos(.11), 0, 0, std::sin(.11)}, d);
            const auto h0 = impulse(ls, order, d, n), h1 = impulse(mag, order, d, n);
            for (unsigned ear = 0; ear < 2; ++ear)
                for (unsigned bin = 32; bin < 106; bin += 7) {
                    const double target = double(T(1 + (ear ? -.25 : .25) * d.y));
                    const double l =
                        std::abs(frequency<T>({h0.data() + ear * n, n}, (double(bin) + .37) / n)) -
                        target;
                    const double m =
                        std::abs(frequency<T>({h1.data() + ear * n, n}, (double(bin) + .37) / n)) -
                        target;
                    lsError += l * l;
                    magError += m * m;
                }
            // Below 1750 Hz, both retain exactly the same complex transfer.
            const auto l = frequency<T>({h0.data(), n}, 4. / n),
                       m = frequency<T>({h1.data(), n}, 4. / n);
            require(std::abs(l - m) < (std::same_as<T, float> ? 2e-6 : 2e-12),
                    "rendered LF phase/magnitude unchanged");
        }
        require(magError < lsError * .65, "MagLS unseen-direction magnitude improves");
        std::cout << "QUALITY HOA unseen HF squared-error ratio=" << magError / lsError << "\n";
        const auto one = impulse(mag, order, {1, 0, 0}, n, 64),
                   split = impulse(mag, order, {1, 0, 0}, n, 17);
        for (std::size_t j = 0; j < one.size(); ++j)
            require(std::abs(double(one[j]) - double(split[j])) <
                        (std::same_as<T, float> ? 2e-6 : 2e-12),
                    "MagLS block segmentation");
        const auto tail = mag.tailFrames();
        auto bad = design;
        bad.maximumStorageBytes = 16;
        require(mag.prepare(spec, order, bank, bad) != Status::ok && mag.tailFrames() == tail,
                "failed design transactional");
        AudioBuffer<T> in, out;
        ok(in.resize(spec.channels, 64), "audit field");
        ok(out.resize(2, 64), "audit ears");
        struct Audit {
            HoaBinaural<T>* p;
            AudioBuffer<T>*in, *out;
            Status status = Status::ok;
        } audit{&mag, &in, &out};
        runRealtimeAudit(
            [](void* p) noexcept {
                auto& a = *static_cast<Audit*>(p);
                a.status = a.p->setRotation({});
                if (a.status == Status::ok)
                    a.status = a.p->process(a.in->block(), a.out->block());
                a.p->reset();
            },
            &audit);
        ok(audit.status, "MagLS realtime allocation/free audit");
    }
}
void taperAndRotation() {
    std::vector<Vec3> dirs(64);
    ok(sphericalFibonacci(dirs), "maxRE grid");
    std::vector<double> ir(dirs.size());
    std::vector<HrirPoint<double>> points;
    for (std::size_t i = 0; i < dirs.size(); ++i) {
        ir[i] = 1 + .25 * dirs[i].x;
        points.push_back({dirs[i], {ir.data() + i, 1}, {ir.data() + i, 1}});
    }
    HrtfSet<double> bank;
    HrtfPreparation p;
    p.detectOnsets = false;
    ok(bank.prepare(points, p), "maxRE bank");
    HoaBinaural<double> plain, taper;
    HoaDecoderDesign d;
    d.method = HoaDecoderMethod::leastSquares;
    d.fftSize = 64;
    d.regularization = 0;
    ok(plain.prepare({48000, 64, 4}, 1, bank, d), "plain decoder");
    d.maxRe = true;
    ok(taper.prepare({48000, 64, 4}, 1, bank, d), "maxRE decoder");
    auto h = impulse(taper, 1, {1, 0, 0}, 64);
    const auto response = frequency<double>({h.data(), 64}, .25);
    require(std::abs(std::abs(response) - (1 + .25 / std::sqrt(3.))) < 1e-12,
            "exact first-order maxRE weight");
    const Quaternion q{std::cos(.2), 0, 0, std::sin(.2)};
    auto a = impulse(taper, 1, rotate(q, {1, 0, 0}), 64);
    ok(taper.setRotation(q), "HOA active rotation");
    auto b = impulse(taper, 1, {1, 0, 0}, 64);
    for (std::size_t j = 0; j < a.size(); ++j)
        require(std::abs(a[j] - b[j]) < 2e-12, "HOA designed filter rotation covariance");
    const auto previous = taper.tailFrames();
    d.spatialWeights = std::span<const double>(ir).first(3);
    require(taper.prepare({48000, 64, 4}, 1, bank, d) != Status::ok &&
                taper.tailFrames() == previous,
            "quadrature size validation");
}
} // namespace
void hoaDesignRegression() {
    grid();
    analytic<float>();
    analytic<double>();
    decoder<float>();
    decoder<double>();
    taperAndRotation();
}
