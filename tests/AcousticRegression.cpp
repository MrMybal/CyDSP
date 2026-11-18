#include <algorithm>
#include <array>
#include <cmath>
#include <cydsp/CyDSP.h>
#include <numbers>
#include <stdexcept>
void runRealtimeAudit(void (*)(void*) noexcept, void*);
namespace {
using namespace cydsp;
void require(bool v, const char* text) {
    if (!v)
        throw std::runtime_error(text);
}
void ok(Status s) {
    require(s == Status::ok, "acoustics status");
}
void rooms() {
    AcousticRoom room;
    room.volumeCubicMetres = 100;
    room.surfaceSquareMetres = 100;
    room.meanAbsorption = {.2, .2, .2};
    RoomResponse response;
    ok(estimateRoomResponse(room, {}, response, RoomDecayModel::sabine));
    require(std::abs(response.decaySeconds[0] - 24 * std::numbers::ln10 * 100 / (343 * 20)) < 1e-12,
            "Sabine room response");
    AcousticPortalGraph graph;
    const std::array<AcousticRoom, 3> rooms{{{1}, {2}, {3}}};
    std::array<AcousticPortal, 3> portals{{{10, 1, 2, 10, 1, {.5, .5, .5}},
                                           {11, 2, 3, 20, 1, {.5, .5, .5}},
                                           {12, 1, 3, 100, 1, {1, 1, 1}}}};
    ok(graph.prepare(rooms, portals));
    std::array<std::uint32_t, 4> ids{};
    std::size_t count = 0;
    AcousticTravel travel;
    ok(graph.route(1, 3, ids, count, travel));
    require(count == 2 && ids[0] == 10 && ids[1] == 11 &&
                std::abs(travel.seconds - 30. / 343) < 1e-15 && travel.bandGains[0] == .25,
            "portal shortest transfer path");
    portals[0].opening = portals[2].opening = 0;
    ok(graph.prepare(rooms, portals));
    count = 99;
    require(graph.route(1, 3, ids, count, travel) == Status::disconnected && count == 99,
            "closed portal route transaction");

    std::array<AcousticPortal, 2> parallel{{{101, 1, 2, 10}, {102, 1, 2, 11}}};
    parallel[0].medium.absorptionDbPerMetre = {2, 2, 2};
    ok(graph.prepare(rooms, parallel));
    ok(graph.route(1, 2, ids, count, travel));
    require(count == 1 && ids[0] == 101 && std::abs(travel.bandGains[0] - .1) < 1e-14,
            "unweighted portal travel time");
    ok(graph.prepare(rooms, parallel, .01));
    ok(graph.route(1, 2, ids, count, travel));
    require(count == 1 && ids[0] == 102 && travel.bandGains[0] == 1,
            "weighted portal medium absorption");
    parallel[0].medium.absorptionDbPerMetre = {1000, 1000, 0};
    parallel[1].bandGains = {.5, .5, .5};
    ok(graph.prepare(rooms, parallel, .01));
    ok(graph.route(1, 2, ids, count, travel));
    require(ids[0] == 101, "portal weight uses transmitted RMS across bands");
    parallel[0].opening = .25;
    ok(graph.prepare(rooms, parallel, .01));
    ok(graph.route(1, 2, ids, count, travel));
    require(ids[0] == 102, "portal opening contributes to loss");
    RoomCellGrid grid;
    std::array<RoomResponse, 8> values;
    for (std::size_t i = 0; i < 8; ++i)
        values[i].decaySeconds = {double(i + 1), double(i + 1), double(i + 1)};
    ok(grid.prepare({0, 0, 0}, {1, 1, 1}, {2, 2, 2}, values));
    ok(grid.sample({.5, .5, .5}, response));
    require(response.decaySeconds[0] == 4.5, "trilinear room cell interpolation");
    ok(grid.sample({-10, -10, -10}, response));
    require(response.decaySeconds[0] == 1, "room cell boundary clamp");
}
template <class T> void paths() {
    AcousticPathRenderer<T> renderer;
    ok(renderer.prepare({8000, 64, 1}, 3, .1));
    std::array<AcousticPath, 2> paths{
        {{17, {1, 0, 0}, 3. / 8000, {.5, .5, .5}}, {29, {0, 1, 0}, 7. / 8000, {-.25, -.25, -.25}}}};
    ok(renderer.setPaths(paths, 0));
    AudioBuffer<T> input, output;
    ok(input.resize(1, 64));
    ok(output.resize(3, 64));
    input.block().channel(0)[0] = 1;
    ok(renderer.process(input.block(), output.block()));
    for (std::size_t i = 0; i < 64; ++i)
        require(std::abs(double(output.block().channel(0)[i]) - (i == 3 ? .5 : 0)) < 1e-7 &&
                    std::abs(double(output.block().channel(1)[i]) - (i == 7 ? -.25 : 0)) < 1e-7,
                "acoustic path delayed gains");
    require(renderer.path(0).id == 17 && renderer.path(1).id == 29, "stable path slots");
    std::swap(paths[0], paths[1]);
    ok(renderer.setPaths(paths, 8));
    require(renderer.path(0).id == 17 && renderer.path(1).id == 29,
            "path input reorder changed slots");
    ok(renderer.setPaths({}, 8));
    ok(renderer.process(input.block(8), output.block(8)));
    require(renderer.activePaths() == 0, "removed paths did not fade");
    ok(renderer.setPaths(paths, 8));
    struct C {
        AcousticPathRenderer<T>* p;
        AudioBlock<const T> in;
        AudioBlock<T> out;
        bool passed = true;
    } c{&renderer, input.block(), output.block()};
    runRealtimeAudit(
        [](void* raw) noexcept {
            auto& c = *static_cast<C*>(raw);
            c.passed = c.p->process(c.in, c.out) == Status::ok;
            c.p->reset();
        },
        &c);
    require(c.passed, "path realtime");
}
template <class T> void subSamplePath() {
    AcousticPathRenderer<T> renderer;
    ok(renderer.prepare({48000, 16, 1}, 1, .01));
    AcousticPath path;
    path.delaySeconds = .5 / 48000;
    ok(renderer.setPaths({&path, 1}, 0));
    AudioBuffer<T> input, output;
    ok(input.resize(1, 16));
    ok(output.resize(1, 16));
    std::ranges::fill(input.block().channel(0), T{1});
    ok(renderer.process(input.block(), output.block()));
    require(std::abs(output.block().channel(0)[0] - .5) < 1e-7, "causal sub-sample delay at onset");
    for (auto sample : output.block().channel(0).subspan(1))
        require(std::abs(sample - 1.) < 1e-7, "sub-sample path changes DC gain");
    path.delaySeconds = .25 / 48000;
    ok(renderer.setPaths({&path, 1}, 8));
    ok(renderer.process(input.block(), output.block()));
    for (auto sample : output.block().channel(0))
        require(std::abs(sample - 1.) < 1e-7, "sub-sample path fade changes DC gain");
}
template <class T> void fdn(CpuBackend backend) {
    FdnReverb<T> reverb;
    FdnConfig config;
    config.lines = 8;
    ok(reverb.prepare({8000, 128, 2}, config, backend));
    FdnParameters parameters;
    parameters.decaySeconds = {.12, .12, .12};
    parameters.preDelayMs = 0;
    parameters.diffusion = 0;
    ok(reverb.setParameters(parameters, 0));
    AudioBuffer<T> input, output;
    ok(input.resize(2, 128));
    ok(output.resize(2, 128));
    double early = 0, late = 0;
    for (std::size_t block = 0; block < 125; ++block) {
        for (std::size_t ch = 0; ch < 2; ++ch)
            std::fill(input.block().channel(ch).begin(), input.block().channel(ch).end(), T{});
        if (!block)
            input.block().channel(0)[0] = 1;
        ok(reverb.process(input.block(), output.block()));
        for (std::size_t ch = 0; ch < 2; ++ch)
            for (T sample : output.block().channel(ch)) {
                require(isFinite(sample), "FDN nonfinite response");
                const double energy = double(sample) * sample;
                if (block < 16)
                    early += energy;
                if (block > 100)
                    late += energy;
            }
    }
    require(early > 1e-4 && late < early * 1e-12, "FDN nominal decay");
    parameters.decaySeconds = {.1, 20, .05};
    ok(reverb.setParameters(parameters));
    parameters.freeze = true;
    ok(reverb.setParameters(parameters));
    require(reverb.nominalTailFrames() == std::numeric_limits<std::size_t>::max(),
            "FDN freeze infinite tail flag");
    struct C {
        FdnReverb<T>* p;
        AudioBlock<const T> in;
        AudioBlock<T> out;
        bool passed = true;
    } c{&reverb, input.block(), output.block()};
    runRealtimeAudit(
        [](void* raw) noexcept {
            auto& c = *static_cast<C*>(raw);
            for (int i = 0; i < 64; ++i)
                c.passed &= c.p->process(c.in, c.out) == Status::ok;
            c.p->reset();
        },
        &c);
    require(c.passed, "FDN realtime");
}
template <class T> void bank() {
    RoomReverbBank<T> bank;
    ok(bank.prepare({8000, 128, 2}, 2));
    FdnParameters p;
    p.preDelayMs = 0;
    p.diffusion = 0;
    ok(bank.setRoom(0, p, 0));
    AudioBuffer<T> input, output;
    ok(input.resize(2, 128));
    ok(output.resize(2, 128));
    input.block().channel(0)[0] = 1;
    double energy = 0;
    for (int block = 0; block < 24; ++block) {
        ok(bank.beginBlock(128));
        if (!block)
            ok(bank.send(0, input.block()));
        ok(bank.render());
        ok(bank.mix(output.block()));
        for (T v : bank.roomOutput(1).channel(0))
            require(v == 0, "room excitation leaked into other room");
        if (block > 1)
            for (T v : output.block().channel(0))
                energy += double(v) * v;
    }
    require(energy > 1e-5, "shared room tail lost after sends stopped");
    struct C {
        RoomReverbBank<T>* p;
        bool passed = true;
    } c{&bank};
    runRealtimeAudit(
        [](void* raw) noexcept {
            auto& c = *static_cast<C*>(raw);
            c.passed = c.p->beginBlock(128) == Status::ok;
            c.passed &= c.p->render() == Status::ok;
            c.p->reset();
        },
        &c);
    require(c.passed, "room bank realtime");
    HybridReverb<T> hybrid;
    const std::array<T, 3> ir{T{1}, T(.5), T(.25)};
    ok(hybrid.prepare({8000, 128, 2}, ir));
    ok(hybrid.setMix(1, 0, 0));
    ok(hybrid.process(input.block(), output.block()));
    for (std::size_t i = 0; i < 128; ++i)
        require(std::abs(double(output.block().channel(0)[i]) - (i < 3 ? double(ir[i]) : 0)) < 1e-7,
                "hybrid IR branch integration");
}
} // namespace
void acousticRegression() {
    rooms();
    paths<float>();
    paths<double>();
    subSamplePath<float>();
    subSamplePath<double>();
    bank<float>();
    bank<double>();
    for (auto backend : {CpuBackend::scalar, CpuBackend::sse2, CpuBackend::avx2, CpuBackend::avx512,
                         CpuBackend::neon, CpuBackend::sve}) {
        VectorOps ops;
        if (VectorOps::create(backend, ops) != Status::ok)
            continue;
        fdn<float>(backend);
        fdn<double>(backend);
    }
}
