// Original CyDSP JSFX host; private runtime: third_party/licenses/ysfx.txt
#include "JsfxEditor.h"
#include "PluginOutput.h"
#include "PluginStateBytes.h"
#include "cydsp/core/SpscQueue.h"
#include "detail/JsfxRuntime.h"
#include <array>
#include <sstream>
namespace cydsp::detail {
namespace {
struct Effect {
    ysfx_t* value = nullptr;
    ~Effect() {
        if (value)
            ysfx_free(value);
    }
};
Status openScript(const std::filesystem::path& path, Effect& result) {
    if (!path.is_absolute() || !std::filesystem::is_regular_file(path) ||
        std::filesystem::file_size(path) > 16 * 1024 * 1024)
        return Status::invalid_argument;
    auto* c = ysfx_config_new();
    if (!c)
        return Status::allocation_failed;
    ysfx_guess_file_roots(c, pathUtf8(path).c_str());
    ysfx_register_builtin_audio_formats(c);
    result.value = ysfx_new(c);
    ysfx_config_free(c);
    if (!result.value)
        return Status::allocation_failed;
    if (!ysfx_load_file(result.value, pathUtf8(path).c_str(), 0))
        return Status::unsupported_format;
    return Status::ok;
}
class JsfxBackend final : public PluginBackend {
    Effect fx_;
    PluginHostCallbacks callbacks_;
    SpscQueue<PluginEvent> uiEdits_;
#if defined(_WIN32)
    std::unique_ptr<JsfxEditor> editor_;
#endif
    PluginDescription desc_;
    PluginPrepareOptions options_;
    PluginOutput output_;
    bool active_ = false;
    std::atomic<std::uint32_t> latency_{0}, latencyKnown_{0};
    void publishTiming() noexcept {
        auto d = ysfx_get_pdc_delay(fx_.value);
        bool known = isFinite(d) && d >= 0 && d <= 4294967295.;
        latency_.store(known ? std::uint32_t(d) : 0, std::memory_order_relaxed);
        latencyKnown_.store(known ? 1u : 0u, std::memory_order_release);
    }
    const PluginParameter* parameter(std::uint32_t id) const noexcept {
        for (const auto& p : desc_.parameters)
            if (p.id == id)
                return &p;
        return nullptr;
    }
    Status set(const PluginEvent& e) noexcept {
        auto* p = parameter(e.id);
        if (!p || !isFinite(e.value) || e.value < p->minimum || e.value > p->maximum)
            return Status::invalid_argument;
        ysfx_slider_set_value(fx_.value, e.id, e.value);
        return Status::ok;
    }
    template <Sample T> Status run(const PluginProcess<T>& call) noexcept {
        output_.reset();
        if (call.transport && call.transport->hasTempo && call.transport->tempoIncrement != 0)
            return Status::unsupported_format;
        // Validate all input before changing DSP state.
        if (call.inputEvents)
            for (std::size_t i = 0; i < call.inputEvents->size(); ++i) {
                const auto& e = *call.inputEvents->event(i);
                if (e.kind == PluginEventKind::parameterValue) {
                    const auto* p = parameter(e.id);
                    if (!p || e.value < p->minimum || e.value > p->maximum || e.port != -1 ||
                        e.noteId != -1 || e.channel != -1 || e.key != -1)
                        return Status::invalid_argument;
                } else if (e.kind == PluginEventKind::gestureBegin ||
                           e.kind == PluginEventKind::gestureEnd) {
                    if (!parameter(e.id))
                        return Status::invalid_argument;
                } else if (e.kind == PluginEventKind::transport) {
                    return Status::unsupported_format;
                } else {
                    std::array<std::uint8_t, 3> bytes;
                    if (pluginMidi(e, bytes).empty() || e.port < 0 || e.port >= 16)
                        return Status::unsupported_format;
                }
            }
        while (const auto* edit = uiEdits_.peek()) {
            auto r = set(*edit);
            if (r != Status::ok)
                return r;
            r = output_.add(*edit);
            if (r != Status::ok)
                return r;
            uiEdits_.discard();
        }
        auto time = [&](std::uint32_t frame) {
            ysfx_time_info_t t{120, ysfx_playback_paused, 0, 0, {4, 4}};
            if (call.transport) {
                const auto& v = *call.transport;
                if (v.hasTempo)
                    t.tempo = v.tempo;
                t.playback_state =
                    v.recording
                        ? (v.playing ? ysfx_playback_recording : ysfx_playback_recording_paused)
                        : (v.playing ? ysfx_playback_playing : ysfx_playback_paused);
                if (v.hasSeconds)
                    t.time_position = v.seconds + frame / options_.sampleRate;
                else if (v.hasSongFrame)
                    t.time_position = (double(v.songFrame) + frame) / options_.sampleRate;
                if (v.hasBeats)
                    t.beat_position = v.beats + frame / options_.sampleRate * t.tempo / 60;
                if (v.hasSignature) {
                    t.time_signature[0] = v.numerator;
                    t.time_signature[1] = v.denominator;
                }
            } else if (call.steadyFrame >= 0)
                t.time_position = (double(call.steadyFrame) + frame) / options_.sampleRate;
            ysfx_set_time_info(fx_.value, &t);
        };
        std::size_t cursor = 0;
        std::uint32_t begin = 0;
        while (begin < call.frames) {
            // Parameter changes split cycles at exact offsets. MIDI retains its local offset.
            while (call.inputEvents && cursor < call.inputEvents->size() &&
                   call.inputEvents->event(cursor)->frame == begin &&
                   (call.inputEvents->event(cursor)->kind == PluginEventKind::parameterValue ||
                    call.inputEvents->event(cursor)->kind == PluginEventKind::gestureBegin ||
                    call.inputEvents->event(cursor)->kind == PluginEventKind::gestureEnd)) {
                const auto& e = *call.inputEvents->event(cursor++);
                if (e.kind == PluginEventKind::parameterValue) {
                    auto r = set(e);
                    if (r != Status::ok)
                        return r;
                }
            }
            std::uint32_t end = call.frames;
            if (call.inputEvents)
                for (auto i = cursor; i < call.inputEvents->size(); ++i) {
                    const auto& e = *call.inputEvents->event(i);
                    if (e.kind == PluginEventKind::parameterValue && e.frame > begin) {
                        end = e.frame;
                        break;
                    }
                }
            // Apply all same-frame controls, independent of stable MIDI/control order.
            if (call.inputEvents) {
                auto next = cursor;
                while (next < call.inputEvents->size() &&
                       call.inputEvents->event(next)->frame < end) {
                    const auto& e = *call.inputEvents->event(next++);
                    if (e.kind == PluginEventKind::parameterValue) {
                        auto r = set(e);
                        if (r != Status::ok)
                            return r;
                        continue;
                    }
                    if (e.kind == PluginEventKind::gestureBegin ||
                        e.kind == PluginEventKind::gestureEnd)
                        continue;
                    std::array<std::uint8_t, 3> bytes;
                    auto midi = pluginMidi(e, bytes);
                    ysfx_midi_event_t event{std::uint32_t(e.port), e.frame - begin,
                                            std::uint32_t(midi.size()), midi.data()};
                    if (!ysfx_send_midi(fx_.value, &event))
                        return Status::update_queue_full;
                }
                cursor = next;
            }
            std::array<const T*, 64> in{};
            std::array<T*, 64> out{};
            const auto ni = desc_.inputs.empty() ? 0u : desc_.inputs[0].channels;
            const auto no = desc_.outputs.empty() ? 0u : desc_.outputs[0].channels;
            for (std::uint32_t ch = 0; ch < ni; ++ch)
                in[ch] = call.inputs[0].channel(ch).data() + begin;
            for (std::uint32_t ch = 0; ch < no; ++ch)
                out[ch] = call.outputs[0].channel(ch).data() + begin;
            time(begin);
            if constexpr (std::same_as<T, float>)
                ysfx_process_float(fx_.value, in.data(), out.data(), ni, no, end - begin);
            else
                ysfx_process_double(fx_.value, in.data(), out.data(), ni, no, end - begin);
            ysfx_midi_event_t midi{};
            while (ysfx_receive_midi(fx_.value, &midi)) {
                if (!call.outputEvents)
                    continue;
                if (midi.offset >= end - begin || midi.bus >= 16 || !midi.size)
                    return Status::invalid_argument;
                PluginEvent e;
                e.kind = midi.data[0] == 0xf0 ? PluginEventKind::sysex : PluginEventKind::midi1;
                e.frame = begin + midi.offset;
                e.port = std::int16_t(midi.bus);
                e.bytes = {midi.data, midi.size};
                auto r = output_.add(e);
                if (r != Status::ok)
                    return r;
            }
            auto automated = ysfx_fetch_slider_automations(fx_.value);
            for (std::uint32_t id = 0; id < 64; ++id)
                if ((automated >> id) & 1u) {
                    if (call.outputEvents) {
                        PluginEvent e;
                        e.kind = PluginEventKind::parameterValue;
                        e.frame = end - 1;
                        e.id = id;
                        e.value = ysfx_slider_get_value(fx_.value, id);
                        auto r = output_.add(e);
                        if (r != Status::ok)
                            return r;
                    }
                }
            begin = end;
        }
        publishTiming();
        return output_.finish(call.outputEvents);
    }

  public:
    Status initialize(const std::filesystem::path& path, const PluginHostCallbacks& callbacks) {
        callbacks_ = callbacks;
        auto queue = uiEdits_.prepare(1024);
        if (queue != Status::ok)
            return queue;
        auto r = openScript(path, fx_);
        if (r != Status::ok)
            return r;
        auto* f = fx_.value;
        if (!jsfxMemoryWithinBudget(f))
            return Status::block_too_large;
        if (!ysfx_compile(f,
#if defined(_WIN32)
                          0
#else
                          ysfx_compile_no_gfx
#endif
                          ))
            return Status::unsupported_format;
        desc_.plugin = {PluginFormat::jsfx, "jsfx", ysfx_get_name(f),
                        ysfx_get_author(f), "",     "JSFX/EEL2"};
        auto ni = ysfx_get_num_inputs(f), no = ysfx_get_num_outputs(f);
        if (ni > 64 || no > 64)
            return Status::invalid_spec;
        if (ni)
            desc_.inputs.push_back({0, ni, 0xffffffffu, "Inputs", "audio", true, true});
        if (no)
            desc_.outputs.push_back({1, no, 0xffffffffu, "Outputs", "audio", true, true});
        for (std::uint32_t i = 0; i < 16; ++i) {
            desc_.noteInputs.push_back({i, 2, 2, "MIDI bus " + std::to_string(i)});
            desc_.noteOutputs.push_back({i, 2, 2, "MIDI bus " + std::to_string(i)});
        }
        for (std::uint32_t id = 0; id < 64; ++id)
            if (ysfx_slider_exists(f, id)) {
                ysfx_slider_range_t range{};
                ysfx_slider_get_range(f, id, &range);
                if (!isFinite(range.min) || !isFinite(range.max) || range.min > range.max ||
                    !isFinite(range.def) || range.def < range.min || range.def > range.max)
                    return Status::invalid_spec;
                PluginParameter p;
                p.id = id;
                p.name = ysfx_slider_get_name(f, id);
                p.minimum = range.min;
                p.maximum = range.max;
                p.defaultValue = range.def;
                p.stepped = ysfx_slider_is_enum(f, id);
                p.automatable = true;
                p.hidden = !ysfx_slider_is_initially_visible(f, id);
                desc_.parameters.push_back(std::move(p));
            }
        desc_.float32 = desc_.float64 = desc_.state = true;
#if defined(_WIN32)
        desc_.editor = ysfx_has_section(f, ysfx_section_gfx);
#endif
        ysfx_init(f);
        publishTiming();
        return Status::ok;
    }
    const PluginDescription& description() const noexcept override {
        return desc_;
    }
    Status activate(const PluginPrepareOptions& o) noexcept override {
        try {
            output_.prepare(o.maximumEvents, 16 * 1024 * 1024);
            options_ = o;
            ysfx_set_sample_rate(fx_.value, o.sampleRate);
            ysfx_set_block_size(fx_.value, o.maximumFrames);
            ysfx_set_midi_capacity(
                fx_.value, std::min<std::uint32_t>(16 * 1024 * 1024, o.maximumEvents * 1024),
                false);
            // Retain all declared local VM memory before audio; scripts may still allocate
            // strings/files/global memory, which remains foreign-code behavior.
            if (!jsfxPrepareMemory(fx_.value))
                return Status::allocation_failed;
            ysfx_init(fx_.value);
            publishTiming();
            active_ = true;
            return Status::ok;
        } catch (...) {
            return Status::allocation_failed;
        }
    }
    Status deactivate() noexcept override {
        active_ = false;
        return Status::ok;
    }
    Status start() noexcept override {
        return active_ ? Status::ok : Status::not_prepared;
    }
    void stop() noexcept override {}
    Status reset() noexcept override {
        return Status::unsupported_format;
    } // @init can allocate or access files.
    Status process(const PluginProcess<float>& p) noexcept override {
        return run(p);
    }
    Status process(const PluginProcess<double>& p) noexcept override {
        return run(p);
    }
    PluginTiming timing() const noexcept override {
        return {latency_.load(std::memory_order_relaxed), 0,
                latencyKnown_.load(std::memory_order_acquire) != 0, false, false};
    }
    Status parameterValue(std::uint32_t id, double& v) noexcept override {
        if (!parameter(id))
            return Status::invalid_argument;
        v = ysfx_slider_get_value(fx_.value, id);
        return Status::ok;
    }
    Status parameterText(std::uint32_t id, double v, std::string& text) noexcept override {
        if (!parameter(id) || !isFinite(v))
            return Status::invalid_argument;
        try {
            if (ysfx_slider_is_enum(fx_.value, id)) {
                auto* name = ysfx_slider_get_enum_name(
                    fx_.value, id, std::uint32_t(std::max(0., v - parameter(id)->minimum)));
                if (name) {
                    text = name;
                    return Status::ok;
                }
            }
            text = std::to_string(v);
            return Status::ok;
        } catch (...) {
            return Status::allocation_failed;
        }
    }
    Status flush(const PluginEventBuffer& events, PluginEventBuffer&) noexcept override {
        for (std::size_t i = 0; i < events.size(); ++i) {
            auto& e = *events.event(i);
            if (e.kind != PluginEventKind::parameterValue)
                return Status::unsupported_format;
            auto r = set(e);
            if (r != Status::ok)
                return r;
        }
        return Status::ok;
    }
    Status saveState(std::vector<std::uint8_t>& out, std::size_t limit) noexcept override {
        try {
            std::unique_ptr<ysfx_state_t, decltype(&ysfx_state_free)> s(ysfx_save_state(fx_.value),
                                                                        ysfx_state_free);
            if (!s)
                return Status::io_error;
            if (s->slider_count > 64 || s->data_size > limit || limit < 16 + s->slider_count * 12 ||
                s->data_size > limit - 16 - s->slider_count * 12)
                return Status::block_too_large;
            std::vector<std::uint8_t> b{'C', 'y', 'J', 'S', 'F', 'X', 1, 0};
            store32(b, s->slider_count);
            store32(b, std::uint32_t(s->data_size));
            for (std::uint32_t i = 0; i < s->slider_count; ++i) {
                store32(b, s->sliders[i].index);
                store64(b, std::bit_cast<std::uint64_t>(s->sliders[i].value));
            }
            b.insert(b.end(), s->data, s->data + s->data_size);
            out.swap(b);
            return Status::ok;
        } catch (...) {
            return Status::allocation_failed;
        }
    }
    Status loadState(std::span<const std::uint8_t> b) noexcept override {
        constexpr std::array<std::uint8_t, 8> sig{'C', 'y', 'J', 'S', 'F', 'X', 1, 0};
        if (b.size() < 16 || !std::equal(sig.begin(), sig.end(), b.begin()))
            return Status::unsupported_format;
        auto n = load32(b, 8), len = load32(b, 12);
        if (n > 64 || 16 + std::size_t(n) * 12 > b.size() ||
            len != b.size() - 16 - std::size_t(n) * 12)
            return Status::invalid_argument;
        std::array<ysfx_state_slider_t, 64> sliders{};
        std::uint64_t seen = 0;
        for (std::uint32_t i = 0; i < n; ++i) {
            auto id = load32(b, 16 + i * 12);
            auto v = std::bit_cast<double>(load64(b, 20 + i * 12));
            auto* p = parameter(id);
            if (!p || !isFinite(v) || v < p->minimum || v > p->maximum || ((seen >> id) & 1))
                return Status::invalid_argument;
            seen |= std::uint64_t(1) << id;
            sliders[i] = {id, v};
        }
        ysfx_state_t s{sliders.data(), n, const_cast<std::uint8_t*>(b.data() + 16 + n * 12), len};
        try {
            return ysfx_load_state(fx_.value, &s) ? Status::ok : Status::io_error;
        } catch (...) {
            return Status::allocation_failed;
        }
    }
#if defined(_WIN32)
    Status openEditor(const PluginWindow& window, PluginEditorSize& size) noexcept override {
        if (!desc_.editor)
            return Status::unsupported_format;
        try {
            auto e = std::make_unique<JsfxEditor>(fx_.value);
            auto r = e->open(window, size);
            if (r == Status::ok)
                editor_ = std::move(e);
            return r;
        } catch (...) {
            return Status::allocation_failed;
        }
    }
    Status resizeEditor(PluginEditorSize& size) noexcept override {
        try {
            return editor_ ? editor_->resize(size) : Status::not_prepared;
        } catch (...) {
            return Status::allocation_failed;
        }
    }
    Status scaleEditor(double scale) noexcept override {
        try {
            return editor_ ? editor_->scale(scale) : Status::not_prepared;
        } catch (...) {
            return Status::allocation_failed;
        }
    }
    Status showEditor(bool value) noexcept override {
        return editor_ ? editor_->show(value) : Status::not_prepared;
    }
    Status focusEditor(bool value) noexcept override {
        return editor_ ? editor_->focus(value) : Status::not_prepared;
    }
    Status keyEditor(const PluginEditorKey& key) noexcept override {
        try {
            return editor_ ? editor_->key(key) : Status::not_prepared;
        } catch (...) {
            return Status::allocation_failed;
        }
    }
    Status wheelEditor(float value) noexcept override {
        return editor_ ? editor_->wheel(value) : Status::not_prepared;
    }
    void closeEditor() noexcept override {
        editor_.reset();
    }
    std::uint32_t pumpMainThread() noexcept override {
        if (!editor_)
            return 0;
        try {
            editor_->pump();
            auto changed = ysfx_fetch_slider_automations(fx_.value);
            for (std::uint32_t id = 0; id < 64; ++id)
                if ((changed >> id) & 1u) {
                    PluginEvent e;
                    e.kind = PluginEventKind::parameterValue;
                    e.id = id;
                    e.value = ysfx_slider_get_value(fx_.value, id);
                    e.flags = 1;
                    if (uiEdits_.push(e) && callbacks_.edited)
                        callbacks_.edited(callbacks_.context, e);
                }
            return changed ? pluginStateDirty : 0;
        } catch (...) {
            return pluginEditorHide;
        }
    }
#endif
};
class JsfxProvider final : public PluginProvider {
  public:
    Status scan(const std::filesystem::path& p,
                std::vector<PluginDescriptor>& out) noexcept override {
        try {
            Effect f;
            auto r = openScript(p, f);
            if (r != Status::ok)
                return r;
            std::vector<PluginDescriptor> next{{PluginFormat::jsfx, "jsfx", ysfx_get_name(f.value),
                                                ysfx_get_author(f.value), "", "JSFX/EEL2"}};
            out.swap(next);
            return Status::ok;
        } catch (...) {
            return Status::io_error;
        }
    }
    Status create(const std::filesystem::path& p, std::string_view id,
                  const PluginHostCallbacks& callbacks, PluginInstance& out) noexcept override {
        if (id != "jsfx")
            return Status::invalid_argument;
        try {
            auto f = std::make_unique<JsfxBackend>();
            auto r = f->initialize(p, callbacks);
            return r == Status::ok ? PluginInstance::adopt(std::move(f), out) : r;
        } catch (...) {
            return Status::allocation_failed;
        }
    }
};
} // namespace
std::unique_ptr<PluginProvider> makeJsfxProvider() {
    return std::make_unique<JsfxProvider>();
}
} // namespace cydsp::detail
