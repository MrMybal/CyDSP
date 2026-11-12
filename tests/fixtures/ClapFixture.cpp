// Original MIT fixture. CLAP declarations: third_party/licenses/clap.txt
#include <algorithm>
#include <array>
#include <chrono>
#include <clap/clap.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <thread>
#include <type_traits>
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
namespace {
const char* const features[]{CLAP_PLUGIN_FEATURE_AUDIO_EFFECT, CLAP_PLUGIN_FEATURE_STEREO, nullptr};
const clap_plugin_descriptor_t descriptor{CLAP_VERSION,
                                          "org.cydsp.fixture",
                                          "CyDSP ABI fixture",
                                          "CyDSP",
                                          "",
                                          "",
                                          "",
                                          "1",
                                          "Original regression fixture",
                                          features};
struct Fixture {
    clap_plugin_t plugin{};
    const clap_host_t* host = nullptr;
    double gain = .5;
    bool active = false, running = false, editor = false;
    std::uint32_t width = 320, height = 160;
#if defined(_WIN32)
    HWND child = nullptr;
#endif
    static Fixture& self(const clap_plugin_t* p) noexcept {
        return *static_cast<Fixture*>(p->plugin_data);
    }
    bool thread(bool audio) const noexcept {
        const auto* t = static_cast<const clap_host_thread_check_t*>(
            host->get_extension(host, CLAP_EXT_THREAD_CHECK));
        return t && (audio ? t->is_audio_thread(host) : t->is_main_thread(host));
    }
    template <class T> clap_process_status process(const clap_process_t* call) noexcept {
        if (!active || !running || !thread(true) || call->audio_inputs_count != 2 ||
            call->audio_outputs_count != 1)
            return CLAP_PROCESS_ERROR;
        const auto count = call->in_events->size(call->in_events);
        std::uint32_t next = 0;
        for (std::uint32_t frame = 0; frame < call->frames_count; ++frame) {
            while (next < count) {
                const auto* e = call->in_events->get(call->in_events, next);
                if (e->time > frame)
                    break;
                if (e->type == CLAP_EVENT_NOTE_ON) {
                    const auto& note = *reinterpret_cast<const clap_event_note_t*>(e);
                    if (note.key == 127 && note.channel == 15)
                        std::_Exit(86);
                }
                if (e->type == CLAP_EVENT_MIDI_SYSEX) {
                    const auto& sysex = *reinterpret_cast<const clap_event_midi_sysex_t*>(e);
                    if (sysex.size == 4 && sysex.buffer[1] == 0x7d && sysex.buffer[2] == 0x44)
                        std::this_thread::sleep_for(std::chrono::milliseconds(150));
                }
                if (e->type == CLAP_EVENT_PARAM_VALUE)
                    gain = reinterpret_cast<const clap_event_param_value_t*>(e)->value;
                if (e->type == CLAP_EVENT_NOTE_ON || e->type == CLAP_EVENT_MIDI ||
                    e->type == CLAP_EVENT_MIDI_SYSEX || e->type == CLAP_EVENT_MIDI2 ||
                    e->type == CLAP_EVENT_PARAM_GESTURE_BEGIN ||
                    e->type == CLAP_EVENT_PARAM_GESTURE_END)
                    if (!call->out_events->try_push(call->out_events, e))
                        return CLAP_PROCESS_ERROR;
                ++next;
            }
            for (unsigned ch = 0; ch < 2; ++ch) {
                T* out = nullptr;
                const T* input = nullptr;
                const T* side = nullptr;
                if constexpr (std::is_same_v<T, float>) {
                    out = call->audio_outputs[0].data32[ch];
                    input = call->audio_inputs[0].data32[ch];
                    side = call->audio_inputs[1].data32[0];
                } else {
                    out = call->audio_outputs[0].data64[ch];
                    input = call->audio_inputs[0].data64[ch];
                    side = call->audio_inputs[1].data64[0];
                }
                out[frame] = T(input[frame] * gain + side[frame]);
            }
        }
        return CLAP_PROCESS_CONTINUE;
    }
    Fixture(const clap_host_t* h) : host(h) {
        plugin = {&descriptor,
                  this,
                  [](const clap_plugin_t* p) noexcept { return self(p).thread(false); },
                  [](const clap_plugin_t* p) noexcept { delete &self(p); },
                  [](const clap_plugin_t* p, double, std::uint32_t, std::uint32_t) noexcept {
                      auto& s = self(p);
                      s.active = s.thread(false);
                      if (s.active)
                          s.host->request_callback(s.host);
                      return s.active;
                  },
                  [](const clap_plugin_t* p) noexcept { self(p).active = false; },
                  [](const clap_plugin_t* p) noexcept {
                      auto& s = self(p);
                      s.running = s.active && s.thread(true);
                      return s.running;
                  },
                  [](const clap_plugin_t* p) noexcept { self(p).running = false; },
                  [](const clap_plugin_t*) noexcept {},
                  [](const clap_plugin_t* p, const clap_process_t* call) noexcept {
                      auto& s = self(p);
                      return call->audio_outputs[0].data64 ? s.process<double>(call)
                                                           : s.process<float>(call);
                  },
                  &extension,
                  [](const clap_plugin_t* p) noexcept {
                      auto& s = self(p);
                      if (!s.thread(false))
                          return;
                      const auto* state = static_cast<const clap_host_state_t*>(
                          s.host->get_extension(s.host, CLAP_EXT_STATE));
                      if (state)
                          state->mark_dirty(s.host);
                      const auto* gui = static_cast<const clap_host_gui_t*>(
                          s.host->get_extension(s.host, CLAP_EXT_GUI));
                      if (s.editor && gui && gui->request_resize(s.host, 400, 200)) {
                          s.width = 400;
                          s.height = 200;
#if defined(_WIN32)
                          if (s.child)
                              SetWindowPos(s.child, nullptr, 0, 0, 400, 200,
                                           SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
#endif
                      }
                  }};
    }
    ~Fixture() {
#if defined(_WIN32)
        if (child)
            DestroyWindow(child);
#endif
    }
    static const void* CLAP_ABI extension(const clap_plugin_t*, const char* id) noexcept;
};
const void* CLAP_ABI Fixture::extension(const clap_plugin_t*, const char* id) noexcept {
    static const clap_plugin_audio_ports_t ports{
        [](const clap_plugin_t*, bool input) noexcept { return input ? 2u : 1u; },
        [](const clap_plugin_t*, std::uint32_t i, bool input,
           clap_audio_port_info_t* out) noexcept {
            if (!out || i >= (input ? 2u : 1u))
                return false;
            *out = {};
            out->id = i;
            out->channel_count = i ? 1 : 2;
            out->flags = CLAP_AUDIO_PORT_SUPPORTS_64BITS | (i ? 0 : CLAP_AUDIO_PORT_IS_MAIN);
            out->in_place_pair = CLAP_INVALID_ID;
            std::snprintf(out->name, sizeof(out->name), "%s", i ? "Sidechain" : "Main");
            out->port_type = i ? CLAP_PORT_MONO : CLAP_PORT_STEREO;
            return true;
        }};
    static const clap_plugin_note_ports_t notes{
        [](const clap_plugin_t*, bool) noexcept { return 1u; },
        [](const clap_plugin_t*, std::uint32_t i, bool, clap_note_port_info_t* out) noexcept {
            if (i || !out)
                return false;
            *out = {};
            out->id = 0;
            out->supported_dialects = 15;
            out->preferred_dialect = CLAP_NOTE_DIALECT_CLAP;
            std::copy_n("Events", 7, out->name);
            return true;
        }};
    static const clap_plugin_params_t params{
        [](const clap_plugin_t*) noexcept { return 1u; },
        [](const clap_plugin_t*, std::uint32_t i, clap_param_info_t* out) noexcept {
            if (i || !out)
                return false;
            *out = {};
            out->id = 7;
            out->flags = CLAP_PARAM_IS_AUTOMATABLE;
            out->min_value = 0;
            out->max_value = 2;
            out->default_value = .5;
            std::copy_n("Gain", 5, out->name);
            return true;
        },
        [](const clap_plugin_t* p, clap_id id, double* value) noexcept {
            if (id != 7 || !value)
                return false;
            *value = self(p).gain;
            return true;
        },
        [](const clap_plugin_t*, clap_id id, double value, char* text,
           std::uint32_t length) noexcept {
            if (id != 7 || !text || !length)
                return false;
            std::snprintf(text, length, "%.3f", value);
            return true;
        },
        [](const clap_plugin_t*, clap_id, const char*, double*) noexcept { return false; },
        [](const clap_plugin_t* p, const clap_input_events_t* in,
           const clap_output_events_t*) noexcept {
            for (std::uint32_t i = 0; i < in->size(in); ++i) {
                const auto* e = in->get(in, i);
                if (e->type == CLAP_EVENT_NOTE_ON) {
                    const auto& note = *reinterpret_cast<const clap_event_note_t*>(e);
                    if (note.key == 127 && note.channel == 15)
                        std::_Exit(86);
                }
                if (e->type == CLAP_EVENT_MIDI_SYSEX) {
                    const auto& sysex = *reinterpret_cast<const clap_event_midi_sysex_t*>(e);
                    if (sysex.size == 4 && sysex.buffer[1] == 0x7d && sysex.buffer[2] == 0x44)
                        std::this_thread::sleep_for(std::chrono::milliseconds(150));
                }
                if (e->type == CLAP_EVENT_PARAM_VALUE)
                    self(p).gain = reinterpret_cast<const clap_event_param_value_t*>(e)->value;
            }
        }};
    static const clap_plugin_state_t state{
        [](const clap_plugin_t* p, const clap_ostream_t* out) noexcept {
            return out->write(out, &self(p).gain, sizeof(double)) == sizeof(double);
        },
        [](const clap_plugin_t* p, const clap_istream_t* in) noexcept {
            double v = 0;
            std::size_t have = 0;
            while (have < sizeof(double)) {
                const auto n =
                    in->read(in, reinterpret_cast<char*>(&v) + have, sizeof(double) - have);
                if (n <= 0)
                    return false;
                have += std::size_t(n);
            }
            if (!std::isfinite(v) || v < 0 || v > 2)
                return false;
            self(p).gain = v;
            return true;
        }};
    static const clap_plugin_latency_t latency{[](const clap_plugin_t*) noexcept { return 17u; }};
    static const clap_plugin_tail_t tail{[](const clap_plugin_t*) noexcept { return 64u; }};
    static const clap_plugin_gui_t gui{
        [](const clap_plugin_t*, const char* api, bool floating) noexcept {
#if defined(_WIN32)
            return !floating && api && std::strcmp(api, CLAP_WINDOW_API_WIN32) == 0;
#else
            (void)api;
            (void)floating;
            return false;
#endif
        },
        [](const clap_plugin_t*, const char**, bool*) noexcept { return false; },
        [](const clap_plugin_t* p, const char*, bool) noexcept {
            return self(p).editor = self(p).thread(false);
        },
        [](const clap_plugin_t* p) noexcept {
            auto& s = self(p);
#if defined(_WIN32)
            if (s.child) {
                DestroyWindow(s.child);
                s.child = nullptr;
            }
#endif
            s.editor = false;
        },
        [](const clap_plugin_t*, double scale) noexcept { return scale > 0; },
        [](const clap_plugin_t* p, std::uint32_t* w, std::uint32_t* h) noexcept {
            *w = self(p).width;
            *h = self(p).height;
            return true;
        },
        [](const clap_plugin_t*) noexcept { return true; },
        [](const clap_plugin_t*, clap_gui_resize_hints_t*) noexcept { return false; },
        [](const clap_plugin_t*, std::uint32_t* w, std::uint32_t* h) noexcept {
            *w = std::max(100u, *w);
            *h = std::max(80u, *h);
            return true;
        },
        [](const clap_plugin_t* p, std::uint32_t w, std::uint32_t h) noexcept {
            auto& s = self(p);
            s.width = w;
            s.height = h;
#if defined(_WIN32)
            if (s.child)
                SetWindowPos(s.child, nullptr, 0, 0, int(w), int(h),
                             SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
#endif
            return true;
        },
        [](const clap_plugin_t* p, const clap_window_t* w) noexcept {
#if defined(_WIN32)
            auto& s = self(p);
            s.child = CreateWindowExW(0, L"STATIC", L"CyDSP original fixture", WS_CHILD, 0, 0,
                                      int(s.width), int(s.height), static_cast<HWND>(w->win32),
                                      nullptr, GetModuleHandleW(nullptr), nullptr);
            return s.child != nullptr;
#else
            (void)p;
            (void)w;
            return false;
#endif
        },
        [](const clap_plugin_t*, const clap_window_t*) noexcept { return false; },
        [](const clap_plugin_t*, const char*) noexcept {},
        [](const clap_plugin_t* p) noexcept {
#if defined(_WIN32)
            if (self(p).child)
                ShowWindow(self(p).child, SW_SHOWNA);
#else
            (void)p;
#endif
            return true;
        },
        [](const clap_plugin_t* p) noexcept {
#if defined(_WIN32)
            if (self(p).child)
                ShowWindow(self(p).child, SW_HIDE);
#else
            (void)p;
#endif
            return true;
        }};
    if (std::strcmp(id, CLAP_EXT_AUDIO_PORTS) == 0)
        return &ports;
    if (std::strcmp(id, CLAP_EXT_NOTE_PORTS) == 0)
        return &notes;
    if (std::strcmp(id, CLAP_EXT_PARAMS) == 0)
        return &params;
    if (std::strcmp(id, CLAP_EXT_STATE) == 0)
        return &state;
    if (std::strcmp(id, CLAP_EXT_LATENCY) == 0)
        return &latency;
    if (std::strcmp(id, CLAP_EXT_TAIL) == 0)
        return &tail;
    if (std::strcmp(id, CLAP_EXT_GUI) == 0)
        return &gui;
    return nullptr;
}
const clap_plugin_factory_t factory{[](const clap_plugin_factory_t*) noexcept { return 1u; },
                                    [](const clap_plugin_factory_t*, std::uint32_t i) noexcept {
                                        return i ? nullptr : &descriptor;
                                    },
                                    [](const clap_plugin_factory_t*, const clap_host_t* host,
                                       const char* id) noexcept -> const clap_plugin_t* {
                                        if (!id || std::strcmp(id, descriptor.id))
                                            return nullptr;
                                        auto* f = new (std::nothrow) Fixture(host);
                                        return f ? &f->plugin : nullptr;
                                    }};
} // namespace
extern "C" CLAP_EXPORT const clap_plugin_entry_t clap_entry{
    CLAP_VERSION, [](const char*) noexcept { return true; }, []() noexcept {},
    [](const char* id) noexcept -> const void* {
        return id && std::strcmp(id, CLAP_PLUGIN_FACTORY_ID) == 0 ? &factory : nullptr;
    }};
