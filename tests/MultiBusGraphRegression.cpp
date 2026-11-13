#include <algorithm>
#include <array>
#include <cydsp/CyDSP.h>
#include <limits>
#include <stdexcept>
void runRealtimeAudit(void (*)(void*) noexcept, void*);
namespace {
using namespace cydsp;
void require(bool v, const char* why) {
    if (!v)
        throw std::runtime_error(why);
}
void ok(Status s) {
    require(s == Status::ok, "multibus graph status");
}
template <class T> struct Processor {
    DelayLine<T> delay;
    T gain = T{1};
    void prepare(std::size_t latency, T g) {
        ok(delay.prepare({48000, 64, 1}, latency));
        ok(delay.setDelay(double(latency)));
        gain = g;
    }
    static Status process(void* raw, std::span<const AudioBlock<const T>> in,
                          std::span<AudioBlock<T>> out) noexcept {
        auto& p = *static_cast<Processor*>(raw);
        auto s = p.delay.process(in[0], out[0]);
        if (s != Status::ok)
            return s;
        for (T& value : out[0].channel(0))
            value *= p.gain;
        return Status::ok;
    }
    static void reset(void* raw) noexcept {
        static_cast<Processor*>(raw)->delay.reset();
    }
};
template <class T> void alignment() {
    const std::array<DspBus, 1> io{{{0, 1}}};
    Processor<T> a, b;
    a.prepare(8, T{1});
    b.prepare(0, T{1});
    const std::array<DspNode<T>, 2> nodes{
        {{1, &a, Processor<T>::process, io, io, {8, 0}, Processor<T>::reset},
         {2, &b, Processor<T>::process, io, io, {0, 0}, Processor<T>::reset}}};
    const std::array<DspConnection, 4> edges{{{0, 0, 1, 0, 1},
                                              {0, 0, 2, 0, 1},
                                              {1, 0, graphOutputNode, 0, 1},
                                              {2, 0, graphOutputNode, 0, 1}}};
    MultiBusGraph<T> graph;
    ok(graph.prepare({48000, 64, 1}, io, io, nodes, edges));
    require(graph.timing().latencyFrames == 8, "graph branch latency metadata");
    AudioBuffer<T> input, output;
    ok(input.resize(1, 64));
    ok(output.resize(1, 64));
    input.block().channel(0)[0] = 1;
    std::array<AudioBlock<const T>, 1> in{input.block()};
    std::array<AudioBlock<T>, 1> out{output.block()};
    ok(graph.process(in, out));
    for (std::size_t i = 0; i < 64; ++i)
        require(output.block().channel(0)[i] == (i == 8 ? T{2} : T{}),
                "parallel graph paths misaligned");
    graph.reset();
    std::array<AudioBlock<T>, 1> aliased{input.block()};
    ok(graph.process(in, aliased));
    require(input.block().channel(0)[8] == T{2}, "graph external input snapshot alias");
    auto cyclic = edges;
    cyclic[0] = {1, 0, 1, 0, 1};
    require(graph.prepare({48000, 64, 1}, io, io, nodes, cyclic) == Status::invalid_argument &&
                graph.timing().latencyFrames == 8,
            "graph cycle prepare transaction");
    graph.reset();
    input.block().channel(0)[63] = std::numeric_limits<T>::quiet_NaN();
    std::fill(output.block().channel(0).begin(), output.block().channel(0).end(), T(7));
    require(graph.process(in, out) == Status::invalid_argument &&
                output.block().channel(0)[0] == T(7),
            "graph input preflight wrote output");
    input.block().channel(0)[63] = 0;
    struct C {
        MultiBusGraph<T>* p;
        std::span<const AudioBlock<const T>> in;
        std::span<AudioBlock<T>> out;
        bool passed = true;
    } c{&graph, in, out};
    runRealtimeAudit(
        [](void* raw) noexcept {
            auto& c = *static_cast<C*>(raw);
            c.passed = c.p->process(c.in, c.out) == Status::ok;
            c.p->reset();
        },
        &c);
    require(c.passed, "multibus graph realtime");
}
template <class T> void sidechainAndBypass() {
    const std::array<DspBus, 2> external{{{0, 1}, {9, 1}}}, jointInputs{{{0, 1}, {100, 1}}};
    const std::array<DspBus, 1> mono{{{0, 1}}};
    Processor<T> slow;
    slow.prepare(8, T{1});
    const auto sum = [](void*, std::span<const AudioBlock<const T>> in,
                        std::span<AudioBlock<T>> out) noexcept {
        for (std::size_t i = 0; i < out[0].frames(); ++i)
            out[0].channel(0)[i] = in[0].channel(0)[i] + in[1].channel(0)[i];
        return Status::ok;
    };
    const std::array<DspNode<T>, 2> nodes{
        {{1, &slow, Processor<T>::process, mono, mono, {8, 0}, Processor<T>::reset},
         {2, nullptr, sum, jointInputs, mono, {0, 0}}}};
    const std::array<DspConnection, 4> edges{
        {{0, 9, 1, 0, 1}, {0, 0, 2, 0, 1}, {1, 0, 2, 100, 1}, {2, 0, graphOutputNode, 0, 1}}};
    MultiBusGraph<T> graph;
    ok(graph.prepare({48000, 64, 1}, external, mono, nodes, edges));
    AudioBuffer<T> main, key, output;
    ok(main.resize(1, 64));
    ok(key.resize(1, 64));
    ok(output.resize(1, 64));
    main.block().channel(0)[0] = 1;
    key.block().channel(0)[0] = 2;
    std::array<AudioBlock<const T>, 2> in{main.block(), key.block()};
    std::array<AudioBlock<T>, 1> out{output.block()};
    ok(graph.process(in, out));
    require(output.block().channel(0)[8] == 3 && output.block().channel(0)[0] == 0,
            "sidechain and main alignment");
    graph.reset();
    ok(graph.setBypassed(2, true, 0));
    ok(graph.process(in, out));
    require(output.block().channel(0)[8] == 1, "bypass must use main bus only");
    MultiBusGraph<T> delayed;
    const std::array<DspNode<T>, 1> delayedNode{{nodes[0]}};
    const std::array<DspConnection, 2> direct{{{0, 0, 1, 0, 1}, {1, 0, graphOutputNode, 0, 1}}};
    ok(delayed.prepare({48000, 64, 1}, mono, mono, delayedNode, direct));
    delayed.reset();
    ok(delayed.setBypassed(1, true, 0));
    std::array<AudioBlock<const T>, 1> single{main.block()};
    ok(delayed.process(single, out));
    require(output.block().channel(0)[8] == 1 && output.block().channel(0)[0] == 0,
            "bypass lost node latency");
    auto infiniteNodes = nodes;
    infiniteNodes[0].timing.tailFrames = std::numeric_limits<std::size_t>::max();
    infiniteNodes[1].timing.tailFrames = 17;
    ok(graph.prepare({48000, 64, 1}, external, mono, infiniteNodes, edges));
    require(graph.timing().tailFrames == std::numeric_limits<std::size_t>::max(),
            "infinite freeze tail lost across graph chain or PDC");
    infiniteNodes[0].timing.tailFrames = 1u << 30;
    infiniteNodes[1].timing.tailFrames = 1;
    require(graph.prepare({48000, 64, 1}, external, mono, infiniteNodes, edges) ==
                    Status::invalid_spec &&
                graph.timing().tailFrames == std::numeric_limits<std::size_t>::max(),
            "finite tail overflow prepare transaction");
}
template <class T> void transition() {
    const std::array<DspBus, 1> io{{{0, 1}}};
    const std::array<DspConnection, 2> edges{{{0, 0, 1, 0, 1}, {1, 0, graphOutputNode, 0, 1}}};
    Processor<T> a, b;
    a.prepare(0, T{1});
    b.prepare(8, T{3});
    const std::array<DspNode<T>, 1> na{
        {{1, &a, Processor<T>::process, io, io, {0, 0}, Processor<T>::reset}}},
        nb{{{1, &b, Processor<T>::process, io, io, {8, 0}, Processor<T>::reset}}};
    MultiBusGraph<T> first, second;
    ok(first.prepare({48000, 64, 1}, io, io, na, edges));
    ok(second.prepare({48000, 64, 1}, io, io, nb, edges));
    GraphTransition<T> transition;
    ok(transition.prepare({48000, 64, 1}, io, io, 16));
    ok(transition.setInitial(std::move(first)));
    ok(transition.stage(std::move(second)));
    AudioBuffer<T> input, output;
    ok(input.resize(1, 64));
    ok(output.resize(1, 64));
    std::fill(input.block().channel(0).begin(), input.block().channel(0).end(), T{1});
    std::array<AudioBlock<const T>, 1> in{input.block()};
    std::array<AudioBlock<T>, 1> out{output.block()};
    ok(transition.process(in, out));
    for (std::size_t i = 0; i < 64; ++i)
        require(output.block().channel(0)[i] == (i < 16 ? T{} : T{1}), "transition fixed latency");
    ok(transition.startTransition(8));
    struct C {
        GraphTransition<T>* p;
        std::span<const AudioBlock<const T>> in;
        std::span<AudioBlock<T>> out;
        bool passed = true;
    } c{&transition, in, out};
    runRealtimeAudit(
        [](void* raw) noexcept {
            auto& c = *static_cast<C*>(raw);
            c.passed = c.p->process(c.in, c.out) == Status::ok;
        },
        &c);
    require(c.passed, "graph transition alloc/free");
    for (std::size_t i = 0; i < 64; ++i) {
        const double expected = i < 16 ? 1 : i < 24 ? 1 + 2 * double(i - 15) / 8 : 3;
        require(std::abs(double(output.block().channel(0)[i]) - expected) < 1e-7,
                "graph latency-aligned warmup/fade oracle");
    }
    require(!transition.transitionRemainingFrames() && !transition.warmupRemainingFrames(),
            "graph fade did not finish");
    ok(transition.process(in, out));
    for (T sample : output.block().channel(0))
        require(sample == 3, "new graph not retained after fade");
}
} // namespace
void multiBusGraphRegression() {
    alignment<float>();
    alignment<double>();
    sidechainAndBypass<float>();
    sidechainAndBypass<double>();
    transition<float>();
    transition<double>();
}
