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
} // namespace
void spatialRenderingRegression() {
    transforms();
    render<float>();
    render<double>();
}
