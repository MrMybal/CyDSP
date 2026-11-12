#include "PluginInternal.h"
#include <algorithm>
#include <atomic>
#include <cstring>
namespace cydsp {
namespace detail {
thread_local const PluginBackend* currentAudioPlugin = nullptr;
bool validPluginTransport(const PluginTransport& t) noexcept {
    const auto fixed = [](double value) {
        return isFinite(value) && value > -2147483648.0 && value < 2147483648.0;
    };
    return (!t.hasTempo || (isFinite(t.tempo) && t.tempo > 0 && t.tempo <= 1000000 &&
                            isFinite(t.tempoIncrement))) &&
           (!t.hasBeats || (fixed(t.beats) && fixed(t.barStart) && fixed(t.loopStartBeats) &&
                            fixed(t.loopEndBeats))) &&
           (!t.hasSeconds ||
            (fixed(t.seconds) && fixed(t.loopStartSeconds) && fixed(t.loopEndSeconds))) &&
           (!t.hasSignature || (t.numerator && t.denominator));
}
} // namespace detail
Status PluginEventBuffer::prepare(std::size_t events, std::size_t bytes) noexcept {
    if (!events || events > 65536 || bytes > 16 * 1024 * 1024)
        return Status::invalid_spec;
    try {
        std::vector<PluginEvent> next(events);
        std::vector<std::uint8_t> payload(bytes);
        events_.swap(next);
        bytes_.swap(payload);
        reset();
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
Status PluginEventBuffer::add(const PluginEvent& e) noexcept {
    if (events_.empty())
        return Status::not_prepared;
    if (static_cast<unsigned>(e.kind) > static_cast<unsigned>(PluginEventKind::ump) ||
        (count_ && e.frame < events_[count_ - 1].frame) || !isFinite(e.value) || e.port < -1 ||
        e.channel < -1 || e.channel > 15 || e.key < -1 || e.key > 127 || e.noteId < -1)
        return Status::invalid_argument;
    if ((e.kind == PluginEventKind::noteOn && (e.port < 0 || e.channel < 0 || e.key < 0)) ||
        ((e.kind == PluginEventKind::noteOn || e.kind == PluginEventKind::noteOff) &&
         (e.value < 0 || e.value > 1)) ||
        (e.kind == PluginEventKind::midi1 &&
         (!MidiMessageView(e.bytes).valid() || e.bytes.size() > 3 || e.port < 0)) ||
        (e.kind == PluginEventKind::sysex &&
         (!MidiMessageView(e.bytes).valid() || e.bytes[0] != 0xf0 || e.port < 0)) ||
        (e.kind == PluginEventKind::ump && (!e.packet.valid() || e.port < 0)) ||
        (e.kind == PluginEventKind::transport && !detail::validPluginTransport(e.time)))
        return Status::invalid_argument;
    if (count_ == events_.size() || e.bytes.size() > bytes_.size() - used_)
        return Status::update_queue_full;
    auto copy = e;
    if (!e.bytes.empty()) {
        std::memmove(bytes_.data() + used_, e.bytes.data(), e.bytes.size());
        copy.bytes = {bytes_.data() + used_, e.bytes.size()};
        used_ += e.bytes.size();
    }
    events_[count_++] = copy;
    return Status::ok;
}
const PluginEvent* PluginEventBuffer::event(std::size_t i) const noexcept {
    return i < count_ ? &events_[i] : nullptr;
}
struct PluginInstance::State {
    std::unique_ptr<PluginBackend> backend;
    std::thread::id main = std::this_thread::get_id();
    std::atomic<std::uint32_t> phase{std::uint32_t(PluginLifecycle::initialized)};
    PluginPrepareOptions options;
    struct Interval {
        std::uintptr_t begin = 0, end = 0;
        std::size_t bus = 0, channel = 0;
        bool output = false;
    };
    std::vector<Interval> intervals;
    std::int64_t minimumSteadyFrame = -1;
    std::atomic<std::uint32_t> latency{0}, tail{0}, timingFlags{0};
    void setTiming(PluginTiming t) noexcept {
        latency.store(t.latencyFrames, std::memory_order_relaxed);
        tail.store(t.tailFrames, std::memory_order_relaxed);
        timingFlags.store((t.latencyKnown ? 1u : 0u) | (t.tailKnown ? 2u : 0u) |
                              (t.infiniteTail ? 4u : 0u),
                          std::memory_order_release);
    }
    bool editor = false;
    bool onMain() const noexcept {
        return main == std::this_thread::get_id() && detail::currentAudioPlugin != backend.get();
    }
    ~State() {
        // Application must stop the audio owner before destruction. Destruction on
        // main may use a serialized symbolic audio scope for final backend cleanup.
        const auto p = PluginLifecycle(phase.load());
        if (p == PluginLifecycle::processing) {
            detail::PluginAudioScope audio(backend.get());
            backend->stop();
        }
        backend->closeEditor();
        if (p == PluginLifecycle::active || p == PluginLifecycle::processing)
            (void)backend->deactivate();
    }
};
PluginInstance::PluginInstance() = default;
PluginInstance::~PluginInstance() = default;
PluginInstance::PluginInstance(PluginInstance&&) noexcept = default;
PluginInstance& PluginInstance::operator=(PluginInstance&&) noexcept = default;
Status PluginInstance::adopt(std::unique_ptr<PluginBackend> backend,
                             PluginInstance& result) noexcept {
    if (!backend || (result.state_ && !result.state_->onMain()))
        return Status::invalid_argument;
    if (result.lifecycle() == PluginLifecycle::processing)
        return Status::transition_in_progress;
    try {
        auto s = std::make_unique<State>();
        s->backend = std::move(backend);
        result.state_.swap(s);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
const PluginDescription* PluginInstance::description() const noexcept {
    return state_ ? &state_->backend->description() : nullptr;
}
PluginLifecycle PluginInstance::lifecycle() const noexcept {
    return state_ ? PluginLifecycle(state_->phase.load(std::memory_order_acquire))
                  : PluginLifecycle::empty;
}
Status PluginInstance::activate(const PluginPrepareOptions& o) noexcept {
    if (!state_)
        return Status::not_prepared;
    if (!state_->onMain())
        return Status::invalid_argument;
    if (lifecycle() != PluginLifecycle::initialized)
        return Status::transition_in_progress;
    if (!isFinite(o.sampleRate) || o.sampleRate <= 0 || o.sampleRate > 1000000 ||
        !o.minimumFrames || o.maximumFrames < o.minimumFrames || o.maximumFrames > 1048576 ||
        !o.maximumEvents || o.maximumEvents > 65536)
        return Status::invalid_spec;
    const auto& d = *description();
    if (o.useFloat64 ? !d.float64 : !d.float32)
        return Status::unsupported_format;
    try {
        std::size_t total = 0;
        for (const auto& port : d.inputs)
            total += port.channels;
        for (const auto& port : d.outputs)
            total += port.channels;
        if (total > 32768)
            return Status::invalid_spec;
        state_->intervals.resize(total);
    } catch (...) {
        return Status::allocation_failed;
    }
    const auto r = state_->backend->activate(o);
    if (r == Status::ok) {
        state_->options = o;
        state_->minimumSteadyFrame = -1;
        state_->setTiming(state_->backend->timing());
        state_->phase.store(std::uint32_t(PluginLifecycle::active), std::memory_order_release);
    }
    return r;
}
Status PluginInstance::start() noexcept {
    if (!state_)
        return Status::not_prepared;
    if (lifecycle() != PluginLifecycle::active)
        return Status::transition_in_progress;
    detail::PluginAudioScope audio(state_->backend.get());
    const auto r = state_->backend->start();
    if (r == Status::ok)
        state_->phase.store(std::uint32_t(PluginLifecycle::processing), std::memory_order_release);
    return r;
}
template <Sample T> Status PluginInstance::processImpl(const PluginProcess<T>& p) noexcept {
    if (!state_)
        return Status::not_prepared;
    if (lifecycle() != PluginLifecycle::processing)
        return Status::not_prepared;
    const auto& o = state_->options;
    const auto& d = *description();
    if (o.useFloat64 != std::same_as<T, double>)
        return Status::unsupported_format;
    if (p.frames < o.minimumFrames || p.frames > o.maximumFrames)
        return Status::block_too_large;
    if (p.inputs.size() != d.inputs.size() || p.outputs.size() != d.outputs.size())
        return Status::channel_mismatch;
    for (std::size_t i = 0; i < p.inputs.size(); ++i)
        if (!p.inputs[i].isValid() || p.inputs[i].channels() != d.inputs[i].channels ||
            p.inputs[i].frames() < p.frames)
            return Status::channel_mismatch;
    for (std::size_t i = 0; i < p.outputs.size(); ++i)
        if (!p.outputs[i].isValid() || p.outputs[i].channels() != d.outputs[i].channels ||
            p.outputs[i].frames() < p.frames)
            return Status::channel_mismatch;
    // Sweep prepared memory intervals. Read-only inputs may overlap, but an
    // output can overlap only its explicitly paired same-channel input.
    auto& intervals = state_->intervals;
    std::size_t used = 0;
    const auto append = [&](const auto& blocks, bool output) {
        for (std::size_t bus = 0; bus < blocks.size(); ++bus)
            for (std::size_t ch = 0; ch < blocks[bus].channels(); ++ch) {
                const auto begin = reinterpret_cast<std::uintptr_t>(blocks[bus].channel(ch).data());
                const auto bytes = std::size_t(p.frames) * sizeof(T);
                if (begin > std::numeric_limits<std::uintptr_t>::max() - bytes ||
                    used == intervals.size())
                    return false;
                intervals[used++] = {begin, begin + bytes, bus, ch, output};
            }
        return true;
    };
    if (!append(p.inputs, false) || !append(p.outputs, true))
        return Status::invalid_argument;
    std::sort(intervals.begin(), intervals.begin() + std::ptrdiff_t(used),
              [](const auto& a, const auto& b) {
                  return a.begin < b.begin || (a.begin == b.begin && a.end < b.end);
              });
    std::uintptr_t inputEnd = 0, outputEnd = 0;
    for (std::size_t first = 0; first < used;) {
        auto last = first + 1;
        while (last < used && intervals[last].begin == intervals[first].begin)
            ++last;
        const typename State::Interval* output = nullptr;
        bool input = false;
        for (auto i = first; i < last; ++i)
            if (intervals[i].output) {
                if (output)
                    return Status::partial_overlap;
                output = &intervals[i];
            } else
                input = true;
        const auto begin = intervals[first].begin;
        if ((output && (inputEnd > begin || outputEnd > begin)) || (input && outputEnd > begin))
            return Status::partial_overlap;
        if (output)
            for (auto i = first; i < last; ++i)
                if (!intervals[i].output) {
                    const auto& in = intervals[i];
                    const auto& out = *output;
                    if (in.end != out.end || in.channel != out.channel ||
                        d.outputs[out.bus].inPlacePair != d.inputs[in.bus].id ||
                        d.inputs[in.bus].inPlacePair != d.outputs[out.bus].id)
                        return Status::partial_overlap;
                }
        for (auto i = first; i < last; ++i)
            if (intervals[i].output)
                outputEnd = std::max(outputEnd, intervals[i].end);
            else
                inputEnd = std::max(inputEnd, intervals[i].end);
        first = last;
    }
    if (p.inputEvents) {
        if (p.inputEvents == p.outputEvents)
            return Status::invalid_argument;
        if (p.inputEvents->size() > o.maximumEvents)
            return Status::block_too_large;
        for (std::size_t i = 0; i < p.inputEvents->size(); ++i)
            if (p.inputEvents->event(i)->frame >= p.frames)
                return Status::invalid_argument;
    }
    if (p.steadyFrame >= 0 && state_->minimumSteadyFrame >= 0 &&
        p.steadyFrame < state_->minimumSteadyFrame)
        return Status::invalid_argument;
    if (p.steadyFrame < -1 ||
        (p.steadyFrame >= 0 &&
         p.steadyFrame > std::numeric_limits<std::int64_t>::max() - p.frames) ||
        (p.transport && !detail::validPluginTransport(*p.transport)))
        return Status::invalid_argument;
    if (p.outputEvents)
        p.outputEvents->reset();
    detail::PluginAudioScope audio(state_->backend.get());
    const auto r = state_->backend->process(p);
    state_->minimumSteadyFrame = p.steadyFrame < 0 ? -1 : p.steadyFrame + p.frames;
    if (r != Status::ok) {
        for (const auto& bus : p.outputs)
            for (std::size_t ch = 0; ch < bus.channels(); ++ch)
                std::fill_n(bus.channel(ch).begin(), p.frames, T{});
        if (p.outputEvents)
            p.outputEvents->reset();
    }
    return r;
}
Status PluginInstance::process(const PluginProcess<float>& p) noexcept {
    return processImpl(p);
}
Status PluginInstance::process(const PluginProcess<double>& p) noexcept {
    return processImpl(p);
}
Status PluginInstance::stop() noexcept {
    if (!state_)
        return Status::not_prepared;
    if (lifecycle() != PluginLifecycle::processing)
        return Status::transition_in_progress;
    detail::PluginAudioScope audio(state_->backend.get());
    state_->backend->stop();
    state_->phase.store(std::uint32_t(PluginLifecycle::active), std::memory_order_release);
    return Status::ok;
}
Status PluginInstance::reset() noexcept {
    if (!state_)
        return Status::not_prepared;
    if (lifecycle() != PluginLifecycle::active && lifecycle() != PluginLifecycle::processing)
        return Status::not_prepared;
    detail::PluginAudioScope audio(state_->backend.get());
    const auto r = state_->backend->reset();
    if (r == Status::ok)
        state_->minimumSteadyFrame = -1;
    return r;
}
Status PluginInstance::deactivate() noexcept {
    if (!state_)
        return Status::not_prepared;
    if (!state_->onMain())
        return Status::invalid_argument;
    if (lifecycle() != PluginLifecycle::active)
        return Status::transition_in_progress;
    const auto r = state_->backend->deactivate();
    if (r == Status::ok)
        state_->phase.store(std::uint32_t(PluginLifecycle::initialized), std::memory_order_release);
    return r;
}
Status PluginInstance::refresh() noexcept {
    if (!state_)
        return Status::not_prepared;
    if (!state_->onMain())
        return Status::invalid_argument;
    if (lifecycle() != PluginLifecycle::initialized)
        return Status::transition_in_progress;
    return state_->backend->refresh();
}
Status PluginInstance::parameterValue(std::uint32_t id, double& value) noexcept {
    if (!state_)
        return Status::not_prepared;
    if (!state_->onMain())
        return Status::invalid_argument;
    if (lifecycle() == PluginLifecycle::processing)
        return Status::transition_in_progress;
    return state_->backend->parameterValue(id, value);
}
Status PluginInstance::parameterText(std::uint32_t id, double value, std::string& text) noexcept {
    if (!state_)
        return Status::not_prepared;
    if (!state_->onMain() || !isFinite(value))
        return Status::invalid_argument;
    return state_->backend->parameterText(id, value, text);
}
Status PluginInstance::flush(const PluginEventBuffer& in, PluginEventBuffer& out) noexcept {
    if (!state_)
        return Status::not_prepared;
    if (!state_->onMain() || &in == &out)
        return Status::invalid_argument;
    if (lifecycle() != PluginLifecycle::initialized)
        return Status::transition_in_progress;
    out.reset();
    return state_->backend->flush(in, out);
}
Status PluginInstance::saveState(std::vector<std::uint8_t>& out, std::size_t limit) noexcept {
    if (!state_)
        return Status::not_prepared;
    if (!state_->onMain() || !limit || limit > 64 * 1024 * 1024)
        return Status::invalid_argument;
    if (lifecycle() == PluginLifecycle::processing)
        return Status::transition_in_progress;
    return state_->backend->saveState(out, limit);
}
Status PluginInstance::loadState(std::span<const std::uint8_t> in) noexcept {
    if (!state_)
        return Status::not_prepared;
    if (!state_->onMain() || in.size() > 64 * 1024 * 1024)
        return Status::invalid_argument;
    if (lifecycle() != PluginLifecycle::initialized)
        return Status::transition_in_progress;
    return state_->backend->loadState(in);
}
PluginTiming PluginInstance::timing() const noexcept {
    if (!state_)
        return {};
    const auto flags = state_->timingFlags.load(std::memory_order_acquire);
    return {state_->latency.load(std::memory_order_relaxed),
            state_->tail.load(std::memory_order_relaxed), (flags & 1u) != 0, (flags & 2u) != 0,
            (flags & 4u) != 0};
}
Status PluginInstance::openEditor(const PluginWindow& w, PluginEditorSize& size) noexcept {
    if (!state_)
        return Status::not_prepared;
    if (!state_->onMain() || !isFinite(w.scale) || w.scale <= 0 || w.scale > 16 ||
        (!w.floating && !w.handle))
        return Status::invalid_argument;
    if (state_->editor)
        return Status::transition_in_progress;
    const auto r = state_->backend->openEditor(w, size);
    if (r == Status::ok)
        state_->editor = true;
    return r;
}
Status PluginInstance::resizeEditor(PluginEditorSize& size) noexcept {
    if (!state_ || !state_->editor)
        return Status::not_prepared;
    if (!state_->onMain() || !size.width || !size.height || size.width > 32768 ||
        size.height > 32768)
        return Status::invalid_argument;
    return state_->backend->resizeEditor(size);
}
Status PluginInstance::scaleEditor(double scale) noexcept {
    if (!state_ || !state_->editor)
        return Status::not_prepared;
    if (!state_->onMain() || !isFinite(scale) || scale <= 0 || scale > 16)
        return Status::invalid_argument;
    return state_->backend->scaleEditor(scale);
}
Status PluginInstance::showEditor(bool show) noexcept {
    if (!state_ || !state_->editor)
        return Status::not_prepared;
    return state_->onMain() ? state_->backend->showEditor(show) : Status::invalid_argument;
}
Status PluginInstance::focusEditor(bool focus) noexcept {
    if (!state_ || !state_->editor)
        return Status::not_prepared;
    return state_->onMain() ? state_->backend->focusEditor(focus) : Status::invalid_argument;
}
Status PluginInstance::keyEditor(const PluginEditorKey& key) noexcept {
    if (!state_ || !state_->editor)
        return Status::not_prepared;
    if (!state_->onMain() || std::uint16_t(key.key) > std::uint16_t(PluginEditorKeyCode::f24) ||
        key.modifiers > 15)
        return Status::invalid_argument;
    return state_->backend->keyEditor(key);
}
Status PluginInstance::wheelEditor(float distance) noexcept {
    if (!state_ || !state_->editor)
        return Status::not_prepared;
    if (!state_->onMain() || !isFinite(distance))
        return Status::invalid_argument;
    return state_->backend->wheelEditor(distance);
}
Status PluginInstance::closeEditor() noexcept {
    if (!state_)
        return Status::not_prepared;
    if (!state_->onMain())
        return Status::invalid_argument;
    if (state_->editor)
        state_->backend->closeEditor();
    state_->editor = false;
    return Status::ok;
}
Status PluginInstance::pumpMainThread(std::uint32_t& requests) noexcept {
    if (!state_)
        return Status::not_prepared;
    if (!state_->onMain())
        return Status::invalid_argument;
    requests = state_->backend->pumpMainThread();
    if (requests & pluginEditorClosed)
        state_->editor = false;
    // Latency/tail queries follow each format's thread contract, outside process.
    state_->setTiming(state_->backend->timing());
    return Status::ok;
}
Status PluginInstance::close() noexcept {
    if (!state_)
        return Status::ok;
    if (!state_->onMain())
        return Status::invalid_argument;
    if (lifecycle() == PluginLifecycle::processing)
        return Status::transition_in_progress;
    state_.reset();
    return Status::ok;
}
Status createPluginProvider(PluginFormat format, std::unique_ptr<PluginProvider>& result) noexcept {
    try {
        std::unique_ptr<PluginProvider> next;
        if (format == PluginFormat::clap)
            next = detail::makeClapProvider();
        else if (format == PluginFormat::vst3)
            next = detail::makeVst3Provider();
        else
            return Status::unsupported_format;
        if (!next)
            return Status::unsupported_backend;
        result.swap(next);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
} // namespace cydsp
