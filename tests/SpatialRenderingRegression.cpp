#include <array>
#include <cmath>
#include <cydsp/CyDSP.h>
#include <numbers>
#include <stdexcept>
void runRealtimeAudit(void (*)(void*) noexcept, void*);
namespace {
using namespace cydsp;
void require(bool v, const char* m) {
    if (!v)
        throw std::runtime_error(m);
}
void ok(Status s) {
    require(s == Status::ok, "spatial rendering status");
}
void transforms() {
    std::array<double, 16> a{}, b{}, r{};
    std::array<double, 256> matrix{};
    for (unsigned order = 0; order <= 3; ++order) {
        const auto n = (order + 1) * (order + 1);
        const Quaternion q{std::cos(.37), 0, 0, std::sin(.37)};
        const Vec3 d{.3, .4, .5};
        ok(ambisonicCoefficients(order, d, {a.data(), n}));
        ok(ambisonicCoefficients(order, rotate(q, d), {b.data(), n}));
        ok(ambisonicRotation(order, q, {matrix.data(), n * n}));
        for (unsigned i = 0; i < n; ++i) {
            r[i] = 0;
            for (unsigned j = 0; j < n; ++j)
                r[i] += matrix[i * n + j] * a[j];
            require(std::abs(r[i] - b[i]) < 2e-13, "HOA rotation equivariance");
        }
        const Quaternion pitch{std::cos(.2), 0, std::sin(.2), 0};
        ok(ambisonicRotation(order, pitch, {matrix.data(), n * n}));
        ok(ambisonicCoefficients(order, rotate(pitch, d), {b.data(), n}));
        for (unsigned i = 0; i < n; ++i) {
            double v = 0;
            for (unsigned j = 0; j < n; ++j)
                v += matrix[i * n + j] * a[j];
            require(std::abs(v - b[i]) < 2e-13, "HOA elevation rotation");
        }
        for (unsigned i = 0; i < n; ++i)
            for (unsigned j = 0; j < n; ++j) {
                double product = 0;
                for (unsigned k = 0; k < n; ++k)
                    product += matrix[i * n + k] * matrix[j * n + k];
                require(std::abs(product - (i == j ? 1. : 0.)) < 2e-13, "HOA rotation energy");
            }
    }
    ok(ambisonicCoefficients(1, {1, 0, 0}, {a.data(), 4}));
    require(a[0] == 1 && std::abs(a[1]) < 1e-14 && std::abs(a[2]) < 1e-14 &&
                std::abs(a[3] - 1) < 1e-14,
            "ACN SN3D front convention");
    VbapLayout layout;
    const std::array<Vec3, 4> speakers{{{1, 1, 0}, {-1, 1, 0}, {-1, -1, 0}, {1, -1, 0}}};
    std::array<double, 4> gains{};
    ok(layout.prepare(speakers, false));
    ok(layout.gains({1, 0, 0}, gains));
    require(std::abs(gains[0] - std::sqrt(.5)) < 1e-12 &&
                std::abs(gains[3] - std::sqrt(.5)) < 1e-12 && gains[1] == 0,
            "2D VBAP front pair");
    const std::array<Vec3, 4> tetra{{{1, 1, 1}, {1, -1, -1}, {-1, 1, -1}, {-1, -1, 1}}};
    ok(layout.prepare(tetra, true));
    for (auto d : tetra) {
        ok(layout.gains(d, gains));
        double energy = 0;
        for (auto g : gains)
            energy += g * g;
        require(std::abs(energy - 1) < 1e-12, "3D VBAP energy");
    }
}
template <class T> void render() {
    const ProcessSpec spec{48000, 64, 1};
    AudioBuffer<T> in, out;
    ok(in.resize(1, 64));
    ok(out.resize(2, 64));
    AnalyticBinaural<T> analytic;
    ok(analytic.prepare(spec));
    const std::array<BinauralEmitter, 1> front{{{{1, 0, 0}, 1, 0}}};
    ok(analytic.setEmitters(front, 0));
    in.block().channel(0)[0] = 1;
    ok(analytic.process(in.block(), out.block()));
    for (std::size_t i = 0; i < 64; ++i)
        require(out.block().channel(0)[i] == out.block().channel(1)[i], "front binaural symmetry");
    analytic.reset();
    const std::array<BinauralEmitter, 1> left{{{{0, 1, 0}, 1, 0}}};
    ok(analytic.setEmitters(left, 0));
    ok(analytic.process(in.block(), out.block()));
    double l = 0, r = 0;
    for (auto v : out.block().channel(0))
        l += double(v) * v;
    for (auto v : out.block().channel(1))
        r += double(v) * v;
    require(l > r * 1.5, "lateral binaural head shadow");
    const std::array<T, 3> ir1{1, T(.5), T(.25)}, ir2{T(.5), T(-.25), 0};
    const std::array<HrirPoint<T>, 2> points{
        {{{1, 0, 0}, ir1, ir2, 0, 2}, {{-1, 0, 0}, ir2, ir1, 2, 0}}};
    HrirBinaural<T> measured;
    ok(measured.prepare(spec, points));
    ok(measured.setDirection({1, 0, 0}, 0));
    ok(measured.process(in.block(), out.block()));
    require(std::abs(double(out.block().channel(0)[0]) - 1) < 1e-6 &&
                std::abs(double(out.block().channel(1)[2]) - .5) < 1e-6,
            "HRIR onset and FIR");
    measured.reset();
    ok(measured.setDirection({0, 1, 0}, 0));
    ok(measured.process(in.block(), out.block()));
    require(std::abs(double(out.block().channel(0)[1]) - .75) < 1e-6 &&
                std::abs(double(out.block().channel(1)[1]) - .75) < 1e-6,
            "time aligned HRIR interpolation");
    const std::array<T, 2> fractionalFir{T(1), T(.5)};
    const HrirPoint<T> fractionalPoint{{1, 0, 0}, fractionalFir, fractionalFir, .5, .5};
    HrirBinaural<T> fractional;
    ok(fractional.prepare(spec, {&fractionalPoint, 1}));
    ok(fractional.process(in.block(), out.block()));
    for (std::size_t i = 0; i < 64; ++i)
        require(std::abs(double(out.block().channel(0)[i]) - (i == 0   ? .5
                                                              : i == 1 ? .75
                                                              : i == 2 ? .25
                                                                       : 0.)) < 1e-7,
                "fractional HRIR direct convolution oracle");
    AmbisonicRenderer<T> hoa;
    const std::array<Vec3, 4> tetra{{{1, 1, 1}, {1, -1, -1}, {-1, 1, -1}, {-1, -1, 1}}};
    ok(hoa.prepare(spec, 1, tetra, 0));
    AudioBuffer<T> field, rotated, decoded;
    ok(field.resize(4, 64));
    ok(rotated.resize(4, 64));
    ok(decoded.resize(4, 64));
    ok(hoa.setDirection(tetra[0]));
    ok(hoa.encode(in.block(), field.block()));
    ok(hoa.decode(field.block(), decoded.block()));
    for (std::size_t c = 0; c < 4; ++c)
        require(std::abs(double(decoded.block().channel(c)[0]) - (c == 0 ? 1. : 0.)) < 1e-6,
                "ambisonic exact tetrahedron decoder");
    struct Audit {
        AnalyticBinaural<T>* a;
        HrirBinaural<T>* h;
        AmbisonicRenderer<T>* hoa;
        AudioBlock<const T> in, field;
        AudioBlock<T> stereo, rotated;
        bool pass = true;
    } audit{&analytic, &measured, &hoa, in.block(), field.block(), out.block(), rotated.block()};
    runRealtimeAudit(
        [](void* p) noexcept {
            auto& c = *static_cast<Audit*>(p);
            c.pass = c.a->process(c.in, c.stereo) == Status::ok &&
                     c.h->process(c.in, c.stereo) == Status::ok &&
                     c.hoa->rotateField(c.field, c.rotated) == Status::ok;
            c.a->reset();
            c.h->reset();
            c.hoa->reset();
        },
        &audit);
    require(audit.pass, "spatial rendering realtime");
}

template <class T> void interpolationAndFft() {
    constexpr std::size_t taps = 256;
    std::array<std::vector<T>, 4> fir;
    for (std::size_t f = 0; f < fir.size(); ++f) {
        fir[f].resize(taps);
        for (std::size_t i = 0; i < taps; ++i)
            fir[f][i] = T(.1 * std::cos(double(i * (f + 3))) * std::exp(-double(i) / 80));
    }
    const std::array<HrirPoint<T>, 2> points{
        {{{1, 0, 0}, fir[0], fir[1], 2.25, 7.75}, {{0, 1, 0}, fir[2], fir[3], 11.5, 4.125}}};
    for (auto backend : {CpuBackend::scalar, CpuBackend::automatic})
        for (std::size_t neighbours : {0u, 1u, 2u})
            for (std::size_t fractional : {0u, 17u}) {
                HrtfPreparation preparation;
                preparation.detectOnsets = false;
                preparation.interpolationPoints = neighbours;
                preparation.fractionalDelayTaps = fractional;
                HrtfSet<T> bank;
                ok(bank.prepare(points, preparation));
                std::vector<T> left(taps), right(taps);
                std::array<double, 2> delays{};
                ok(bank.interpolate({1, 0, 0}, left, right, delays));
                require(left == fir[0] && right == fir[1] && delays[0] == 2.25 && delays[1] == 7.75,
                        "public exact HRTF FIR and delays");
                ok(bank.interpolate({1, 1, 0}, left, right, delays));
                if (neighbours != 1) {
                    for (std::size_t t = 0; t < taps; ++t)
                        require(std::abs(double(left[t]) - .5 * double(fir[0][t] + fir[2][t])) <
                                    2e-8,
                                "public symmetric interpolation");
                    require(std::abs(delays[0] - 6.875) < 1e-12, "public weighted onset delay");
                }
                const auto saved = left;
                const auto savedDelay = delays;
                require(bank.interpolate({0, 0, 0}, left, right, delays) ==
                                Status::invalid_argument &&
                            left == saved && delays == savedDelay,
                        "invalid interpolation transactional");
                require(bank.interpolate({1, 0, 0}, left, left, delays) ==
                                Status::invalid_argument &&
                            left == saved,
                        "interpolation overlap transactional");
                HrirBinaural<T> direct, spectral, automatic;
                HrirRenderConfig c;
                c.convolution = HrirConvolution::direct;
                ok(direct.prepare({48000, 257, 1}, bank, c, backend));
                c.convolution = HrirConvolution::fft;
                ok(spectral.prepare({48000, 257, 1}, bank, c, backend));
                ok(automatic.prepare({48000, 257, 1}, bank, backend));
                require(direct.latencyFrames() == spectral.latencyFrames() &&
                            direct.tailFrames() == spectral.tailFrames(),
                        "FFT adds no buffering latency/tail");
                AudioBuffer<T> in, a, b, d;
                ok(in.resize(1, 257));
                ok(a.resize(2, 257));
                ok(b.resize(2, 257));
                ok(d.resize(2, 257));
                const auto edit = [&](Vec3 direction, std::size_t fade) {
                    ok(direct.setDirection(direction, fade));
                    ok(spectral.setDirection(direction, fade));
                    ok(automatic.setDirection(direction, fade));
                };
                edit({1, 0, 0}, 0);
                std::size_t position = 0;
                for (unsigned pass = 0; pass < 60; ++pass) {
                    if (pass == 8)
                        edit({1, 1, 0}, 71); // finishes inside a large block
                    if (pass == 20)
                        edit({0, 1, 0}, 513); // spans mixed FFT/direct call sizes
                    if (pass == 45) {
                        direct.reset();
                        spectral.reset();
                        automatic.reset();
                    }
                    const auto frames =
                        std::array<std::size_t, 6>{128, 1, 257, 31, 64, 7}[pass % 6];
                    for (std::size_t i = 0; i < frames; ++i)
                        in.block().channel(0)[i] = T(.2 * std::sin(double(position++ * 7)));
                    ok(direct.process(in.block(frames), a.block(frames)));
                    ok(spectral.process(in.block(frames), b.block(frames)));
                    ok(automatic.process(in.block(frames), d.block(frames)));
                    for (unsigned ear = 0; ear < 2; ++ear)
                        for (std::size_t i = 0; i < frames; ++i) {
                            const double tolerance = std::same_as<T, float> ? 2e-7 : 2e-12;
                            require(
                                std::abs(double(a.block().channel(ear)[i]) -
                                         double(b.block().channel(ear)[i])) < tolerance,
                                "FFT HRIR equals full-history direct convolution during changes");
                            require(std::abs(double(a.block().channel(ear)[i]) -
                                             double(d.block().channel(ear)[i])) < tolerance,
                                    "automatic mixed block partition equivalence");
                        }
                }
                struct Audit {
                    HrirBinaural<T>* p;
                    HrtfSet<T>* bank;
                    AudioBuffer<T>* in;
                    AudioBuffer<T>* out;
                    std::vector<T>* l;
                    std::vector<T>* r;
                    Status status;
                } audit{&spectral, &bank, &in, &b, &left, &right, Status::ok};
                runRealtimeAudit(
                    [](void* ptr) noexcept {
                        auto& a = *static_cast<Audit*>(ptr);
                        std::array<double, 2> delays{};
                        a.p->reset();
                        a.status = a.bank->interpolate({1, 1, 0}, *a.l, *a.r, delays);
                        if (a.status == Status::ok)
                            a.status = a.p->setDirection({1, 0, 0}, 19);
                        if (a.status == Status::ok)
                            a.status = a.p->process(a.in->block(128), a.out->block(128));
                        a.p->reset();
                    },
                    &audit);
                ok(audit.status);
                c.maximumFftSize = 32;
                require(spectral.prepare({48000, 257, 1}, bank, c, backend) ==
                                Status::invalid_spec &&
                            spectral.tailFrames() == direct.tailFrames(),
                        "bounded FFT preparation preserves plan");
            }
}
} // namespace
void hrtfTimingRegression();
void spatialRenderingRegression() {
    hrtfTimingRegression();
    interpolationAndFft<float>();
    interpolationAndFft<double>();
    transforms();
    render<float>();
    render<double>();
}
