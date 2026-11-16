#include <algorithm>
#include <cmath>
#include <complex>
#include <cstring>
#include <cydsp/core/AudioBuffer.h>
#include <cydsp/spatial/Sofa.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <numbers>
#include <stdexcept>
using namespace cydsp;
void runRealtimeAudit(void (*)(void*) noexcept, void*);
namespace {
void require(bool b, const char* message) {
    if (!b)
        throw std::runtime_error(message);
}
void ok(Status s, const char* message) {
    require(s == Status::ok, message);
}
std::vector<std::byte> fixture(const char* name) {
    std::ifstream f(std::filesystem::path(CYDSP_SOFA_FIXTURES) / name,
                    std::ios::binary | std::ios::ate);
    require(bool(f), "fixture missing");
    const auto n = f.tellg();
    require(n > 0, "fixture size");
    std::vector<std::byte> bytes(static_cast<std::size_t>(n));
    f.seekg(0);
    f.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    require(bool(f), "fixture read");
    return bytes;
}
void imports() {
    for (auto backend : {SofaBackend::compact, SofaBackend::complete}) {
        if (!sofaBackendAvailable(backend))
            continue;
        SofaLoadOptions o;
        o.backend = backend;
        SofaHrtf a;
        SofaLoadReport report;
        const auto baseline = fixture("compact-v2.sofa");
        auto st = loadSofa(baseline, a, o, &report);
        if (st != Status::ok)
            std::cerr << "baseline backend " << int(backend) << " error " << int(report.error)
                      << "\n";
        ok(st, "baseline import");
        require(a.points == 2 && a.taps == 8 && a.sampleRate == 48000, "shape/rate");
        require(std::abs(a.directions[1].y - 1) < 1e-6 && a.directions[0].x == 1,
                "spherical coordinates");
        require(a.left[0] == .5f && a.right[0] == .25f && a.leftDelay[0] == 2.5 &&
                    a.rightDelay[1] == 1.25,
                "ear order/delay expansion");
        require(report.receiversSwapped && report.sourcePrecisionBits == 64 &&
                    report.precisionReduced,
                "import diagnostics");
        require(a.title == "CyDSP synthetic fixture" && a.license == "MIT", "metadata");
        for (const char* name : {"compact-v1-chunks.sofa", "compact-f32.sofa", "bigendian.sofa",
                                 "rotated.sofa", "empty-attributes.sofa"}) {
            st = loadSofa(fixture(name), a, o, &report);
            if (st != Status::ok)
                std::cerr << name << " backend " << int(backend) << " error " << int(report.error)
                          << "\n";
            ok(st, "supported fixture");
            require(std::abs(a.directions[1].y - 1) < 1e-6 && a.left[0] == .5f,
                    "coordinate/endian/compression value");
        }
        if (backend == SofaBackend::compact) {
            bool invoked = false;
            o.inflate = [](std::span<const std::byte>, std::span<std::byte>, void* p) noexcept {
                *static_cast<bool*>(p) = true;
                return Status::unsupported_format;
            };
            o.inflateContext = &invoked;
            require(loadSofa(fixture("compact-v1-chunks.sofa"), a, o, &report) != Status::ok &&
                        invoked,
                    "host inflate callback used without fallback");
            o.inflate = nullptr;
            o.inflateContext = nullptr;
        }
        const auto pipeline = loadSofa(fixture("deflate-only.sofa"), a, o, &report);
        if (backend == SofaBackend::compact)
            require(pipeline == Status::unsupported_format, "unsupported compact filter order");
        else {
            ok(pipeline, "complete deflate without shuffle");
            require(a.left[0] == .5f, "deflate value");
        }
        o.maximumPoints = 1;
        require(loadSofa(baseline, a, o, &report) != Status::ok &&
                    report.error == SofaError::limit_exceeded,
                "point budget");
        o.maximumPoints = 100000;
        o.maximumDecodedBytes = 16;
        require(loadSofa(baseline, a, o, &report) != Status::ok, "byte budget");
        o.maximumDecodedBytes = 512u * 1024u * 1024u;
        const auto snapshot = a.left;
        for (const char* name :
             {"unsupported.sofa", "varying-rate.sofa", "invalid-receiver.sofa"}) {
            require(loadSofa(fixture(name), a, o, &report) != Status::ok && a.left == snapshot,
                    "failure preserves output");
        }
        require(loadSofa(fixture("general-fir.sofa"), a, o, &report) == Status::unsupported_format,
                "GeneralFIR opt-in");
        o.allowGeneralFir = true;
        ok(loadSofa(fixture("general-fir.sofa"), a, o, &report), "GeneralFIR");
        SofaHrtf64 d;
        if (backend == SofaBackend::compact) {
            require(loadSofa(baseline, d, o, &report) == Status::unsupported_format,
                    "no implicit double loss");
            o.allowPrecisionLoss = true;
            ok(loadSofa(baseline, d, o, &report), "explicit compact double loss");
            require(report.precisionReduced, "loss reported");
        } else {
            ok(loadSofa(baseline, d, o, &report), "double import");
            require(d.left[0] == .5 + std::ldexp(1., -40) && !report.precisionReduced,
                    "double precision retained");
            for (const char* name : {"legacy-v0.sofa", "modern-fixed.sofa", "modern-single.sofa",
                                     "modern-extensible.sofa", "modern-dense.sofa"}) {
                st = loadSofa(fixture(name), d, o, &report);
                if (st != Status::ok)
                    std::cerr << name << " error " << int(report.error) << "\n";
                ok(st, "complete HDF5 layouts");
                require(d.left[0] == .5 + std::ldexp(1., -40), "complete value");
            }
        }
        // Deterministic truncated inputs exercise all section boundaries.
        for (std::size_t n = 0; n < baseline.size(); n += 113) {
            const auto status = loadSofa(std::span(baseline).first(n), a, o, &report);
            require(status != Status::ok, "truncated file must fail");
        }
        std::cout << "import backend " << int(backend) << " passed\n";
    }
}
template <class T> BasicSofaHrtf<T> asset() {
    BasicSofaHrtf<T> a;
    a.sampleRate = 48000;
    a.points = 3;
    a.taps = 3;
    a.directions = {{1, 0, 0}, {0, 1, 0}, {1, 0, 0}};
    a.distancesMetres = {1, 1, 2};
    a.left = {T(.5), T(.25), T(0), T(.25), T(.125), T(0), T(.9), T(0), T(0)};
    a.right = a.left;
    for (auto& v : a.right)
        v *= T(.5);
    a.leftDelay = {.5, 1, 0};
    a.rightDelay = {1.25, 2, 0};
    a.title = "Original";
    a.license = "MIT";
    return a;
}
template <class T> void preparation() {
    auto a = asset<T>();
    std::vector<std::byte> bytes;
    ok(encodeHrtfAsset(a, bytes), "cache encode");
    BasicSofaHrtf<T> b;
    ok(decodeHrtfAsset(bytes, b), "cache decode");
    require(a.left == b.left && a.rightDelay == b.rightDelay && b.title == a.title,
            "cache bit exact");
    const auto saved = b.left;
    bytes[21] ^= std::byte{1};
    require(decodeHrtfAsset(bytes, b) != Status::ok && b.left == saved, "checksum transactional");
    SofaPreparation c;
    c.hrtf.detectOnsets = false;
    SofaPreparationReport report;
    HrtfSet<T> set;
    ok(prepareSofaHrtf(a, set, c, &report), "shell prepare");
    require(set.points() == 2 && report.distanceMetres == 1, "distance selection");
    c.hrtf.maximumTaps = 1;
    c.hrtf.rejectTruncation = true;
    require(prepareSofaHrtf(a, set, c) != Status::ok && set.taps() == 3,
            "explicit truncation rejection");
    c.hrtf.maximumTaps = 2048;
    c.hrtf.rejectTruncation = false;
    c.diffuseFieldEqualization = true;
    c.equalizationTaps = 33;
    c.normalization = HrtfLevelNormalization::frontalEnergy;
    ok(prepareSofaHrtf(a, set, c, &report), "EQ normalization");
    require(report.diffuseFieldEqualized && report.uniformDiffuseWeights &&
                report.preparedTaps == 35,
            "EQ report");
    // A common filter/gain preserves the interaural ratio; fractional delays set zero here.
    a.leftDelay.assign(3, 0);
    a.rightDelay.assign(3, 0);
    ok(prepareSofaHrtf(a, set, c), "equalized zero delays");
    HrirBinaural<T> render;
    ok(render.prepare({48000, 64, 1}, set), "render prepare");
    AudioBuffer<T> in, out;
    ok(in.resize(1, 64), "in");
    ok(out.resize(2, 64), "out");
    in.block().channel(0)[0] = 1;
    ok(render.process(in.block(), out.block()), "render EQ");
    double energy = 0;
    for (std::size_t j = 0; j < 64; ++j) {
        const double l = out.block().channel(0)[j], r = out.block().channel(1)[j];
        require(std::abs(r - .5 * l) < 1e-6, "EQ preserves ILD");
        energy += .5 * (l * l + r * r);
    }
    require(std::abs(energy - 1) < 1e-5, "front energy normalization");
    HrtfPreparation q;
    ok(hrtfPreparation(HrtfQuality::realtimeHigh, 48000, q), "quality");
    q.detectOnsets = false;
    c.hrtf = q;
    c.diffuseFieldEqualization = false;
    c.normalization = HrtfLevelNormalization::none;
    a.leftDelay.assign(3, .5);
    a.rightDelay.assign(3, .5);
    ok(prepareSofaHrtf(a, set, c), "sinc prepare");
    ok(render.prepare({48000, 64, 1}, set), "sinc render");
    require(render.latencyFrames() == 8, "sinc latency");
    struct Audit {
        HrirBinaural<T>* render;
        AudioBuffer<T>*in, *out;
        Status s = Status::ok;
    } audit{&render, &in, &out};
    runRealtimeAudit(
        [](void* p) noexcept {
            auto& x = *static_cast<Audit*>(p);
            x.render->reset();
            x.s = x.render->setDirection({1, 1, 0}, 16);
            if (x.s == Status::ok)
                x.s = x.render->process(x.in->block(), x.out->block());
        },
        &audit);
    ok(audit.s, "no allocation render/update");
    // A delayed impulse must preserve FIR DC gain after sinc interpolation.
    render.reset();
    ok(render.setDirection({1, 0, 0}, 0), "exact direction");
    ok(render.process(in.block(), out.block()), "sinc impulse");
    double actual = 0;
    for (std::size_t j = 0; j < 64; ++j)
        actual += out.block().channel(0)[j];
    require(std::abs(actual - .75) < 1e-6, "fractional DC gain");
}
void spatialQuality() {
    // Nearest-direction search is checked against an exhaustive angular oracle.
    std::vector<Vec3> dirs;
    std::vector<double> values;
    std::vector<HrirPoint<double>> points;
    constexpr std::size_t count = 3001;
    dirs.reserve(count);
    values.reserve(count);
    points.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        const double phase = 2 * std::numbers::pi * double(i) / double(count);
        dirs.push_back({std::cos(phase), std::sin(phase), 0});
        values.push_back(.1 + double(i) / double(count));
    }
    for (std::size_t i = 0; i < count; ++i)
        points.push_back({dirs[i], {values.data() + i, 1}, {values.data() + i, 1}, 0, 0});
    HrtfPreparation cfg;
    cfg.maximumPoints = count;
    cfg.interpolationPoints = 1;
    cfg.detectOnsets = false;
    HrtfSet<double> bank;
    ok(bank.prepare(points, cfg), "large direction bank");
    HrirBinaural<double> renderer;
    ok(renderer.prepare({48000, 256, 1}, bank), "nearest render");
    AudioBuffer<double> in, out;
    ok(in.resize(1, 256), "quality input");
    ok(out.resize(2, 256), "quality output");
    in.block().channel(0)[0] = 1;
    for (unsigned t = 0; t < 53; ++t) {
        const double angle = .123 + double(t) * .117;
        const Vec3 d{std::cos(angle), std::sin(angle), 0};
        std::size_t best = 0;
        double score = -2;
        for (std::size_t i = 0; i < count; ++i)
            if (d.x * dirs[i].x + d.y * dirs[i].y > score) {
                best = i;
                score = d.x * dirs[i].x + d.y * dirs[i].y;
            }
        renderer.reset();
        ok(renderer.setDirection(d, 0), "nearest lookup");
        ok(renderer.process(in.block(), out.block()), "nearest impulse");
        require(std::abs(out.block().channel(0)[0] - values[best]) < 1e-14,
                "KD vs exhaustive nearest");
    }
    std::array<double, 1> impulse{1};
    const std::array<HrirPoint<double>, 1> one{{{{1, 0, 0}, impulse, impulse, .5, .5}}};
    double previous = 1;
    for (auto quality : {HrtfQuality::economy, HrtfQuality::realtimeHigh, HrtfQuality::reference}) {
        ok(hrtfPreparation(quality, 48000, cfg), "quality preset");
        cfg.detectOnsets = false;
        ok(bank.prepare(one, cfg), "quality bank");
        ok(renderer.prepare({48000, 256, 1}, bank), "quality renderer");
        ok(renderer.process(in.block(), out.block()), "quality impulse");
        HoaBinaural<double> hoa;
        AudioBuffer<double> hoaOut;
        ok(hoaOut.resize(2, 256), "HOA quality output");
        ok(hoa.prepare({48000, 256, 1}, 0, bank, 0), "HOA delay quality");
        ok(hoa.process(in.block(), hoaOut.block()), "HOA impulse");
        require(hoa.latencyFrames() == renderer.latencyFrames(), "HOA fractional latency");
        for (std::size_t j = 0; j < 256; ++j)
            require(std::abs(hoaOut.block().channel(0)[j] - out.block().channel(0)[j]) < 1e-13,
                    "HOA and direct fractional-delay equivalence");
        const double omega = 2 * std::numbers::pi * .2;
        std::complex<double> response{};
        for (std::size_t j = 0; j < 256; ++j)
            response += out.block().channel(0)[j] * std::polar(1., -omega * double(j));
        const auto ideal = std::polar(1., -omega * (.5 + double(renderer.latencyFrames())));
        const double error = std::abs(response - ideal);
        require(error < previous, "quality improves fractional-delay response");
        previous = error;
        if (quality == HrtfQuality::realtimeHigh)
            require(error < .001, "realtime sinc error");
        if (quality == HrtfQuality::reference)
            require(error < 1e-5, "reference sinc error");
        std::cout << "QUALITY HRTF profile=" << int(quality)
                  << " complex error at 9.6 kHz=" << error << "\n";
    }
}
void mutations() {
    const auto original = fixture("compact-v1-chunks.sofa");
    SofaLoadOptions o;
    o.backend = SofaBackend::compact;
    o.maximumDecodedBytes = 4 * 1024 * 1024;
    SofaHrtf out;
    SofaLoadReport r;
    std::uint32_t seed = 1979;
    if (!sofaBackendAvailable(o.backend))
        return;
    for (unsigned i = 0; i < 3000; ++i) {
        auto bytes = original;
        seed = seed * 1664525u + 1013904223u;
        const auto index = seed % bytes.size();
        seed = seed * 1664525u + 1013904223u;
        bytes[index] ^= std::byte(1u + (seed % 255u));
        const auto status = loadSofa(bytes, out, o, &r);
        if (status == Status::ok)
            require(out.left.size() == out.points * out.taps, "mutated success shape");
    }
    std::cout << "PASS 3000 deterministic compact-reader mutations\n";
}

} // namespace
int main(int argc, char**) {
    try {
        if (argc > 1) {
            mutations();
            return 0;
        }
        imports();
        preparation<float>();
        preparation<double>();
        spatialQuality();
        std::cout << "PASS SOFA imports, cache, preparation and rendering\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "FAIL " << e.what() << "\n";
        return 1;
    }
}
