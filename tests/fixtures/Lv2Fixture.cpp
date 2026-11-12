// Original MIT fixture; LV2 declarations: third_party/licenses/lv2.txt
#include "lv2/atom/util.h"
#include "lv2/core/lv2.h"
#include "lv2/midi/midi.h"
#include "lv2/state/state.h"
#include "lv2/ui/ui.h"
#include "lv2/urid/urid.h"
#include "lv2/worker/worker.h"
#include <array>
#include <cstring>
#include <new>
#if defined(_WIN32)
#define NOMINMAX
#include <windows.h>
#endif
namespace {
struct Effect {
    std::array<void*, 10> ports{};
    LV2_URID_Map* map = nullptr;
    LV2_Worker_Schedule* schedule = nullptr;
    std::uint32_t midi = 0, sequence = 0, key = 0, floatType = 0;
    float memory = .125f, worked = 0;
};
LV2_Handle instantiate(const LV2_Descriptor*, double, const char*, const LV2_Feature* const* f) {
    auto* e = new (std::nothrow) Effect;
    if (!e)
        return nullptr;
    for (; f && *f; ++f) {
        if (!std::strcmp((*f)->URI, LV2_URID__map))
            e->map = static_cast<LV2_URID_Map*>((*f)->data);
        if (!std::strcmp((*f)->URI, LV2_WORKER__schedule))
            e->schedule = static_cast<LV2_Worker_Schedule*>((*f)->data);
    }
    if (!e->map) {
        delete e;
        return nullptr;
    }
    e->midi = e->map->map(e->map->handle, LV2_MIDI__MidiEvent);
    e->sequence = e->map->map(e->map->handle, LV2_ATOM__Sequence);
    e->key = e->map->map(e->map->handle, "urn:cydsp:fixture:memory");
    e->floatType = e->map->map(e->map->handle, LV2_ATOM__Float);
    return e;
}
void connect(LV2_Handle h, std::uint32_t i, void* p) {
    if (i < 10)
        static_cast<Effect*>(h)->ports[i] = p;
}
void run(LV2_Handle h, std::uint32_t n) {
    auto& e = *static_cast<Effect*>(h);
    auto* gain = static_cast<float*>(e.ports[4]);
    for (std::size_t ch = 0; ch < 2; ++ch) {
        auto* in = static_cast<float*>(e.ports[ch]);
        auto* out = static_cast<float*>(e.ports[2 + ch]);
        for (std::uint32_t i = 0; i < n; ++i)
            out[i] = in[i] * (*gain) + e.memory + static_cast<float*>(e.ports[9])[i];
    }
    *static_cast<float*>(e.ports[7]) = 11;
    *static_cast<float*>(e.ports[8]) = e.worked;
    auto* out = static_cast<LV2_Atom_Sequence*>(e.ports[6]);
    auto capacity = out->atom.size;
    out->atom = {8, e.sequence};
    out->body = {0, 0};
    const auto* in = static_cast<const LV2_Atom_Sequence*>(e.ports[5]);
    LV2_ATOM_SEQUENCE_FOREACH(in, event) if (event->body.type == e.midi) {
        lv2_atom_sequence_append_event(out, capacity, event);
        if (e.schedule) {
            std::uint32_t value = 1;
            e.schedule->schedule_work(e.schedule->handle, 4, &value);
        }
    }
}
LV2_State_Status save(LV2_Handle h, LV2_State_Store_Function store, LV2_State_Handle c,
                      std::uint32_t, const LV2_Feature* const*) {
    auto& e = *static_cast<Effect*>(h);
    return store(c, e.key, &e.memory, 4, e.floatType, LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);
}
LV2_State_Status restore(LV2_Handle h, LV2_State_Retrieve_Function get, LV2_State_Handle c,
                         std::uint32_t, const LV2_Feature* const*) {
    auto& e = *static_cast<Effect*>(h);
    std::size_t n = 0;
    std::uint32_t type = 0, flags = 0;
    auto* data = get(c, e.key, &n, &type, &flags);
    if (data && n == 4 && type == e.floatType)
        std::memcpy(&e.memory, data, 4);
    return LV2_STATE_SUCCESS;
}
LV2_Worker_Status work(LV2_Handle, LV2_Worker_Respond_Function respond, LV2_Worker_Respond_Handle c,
                       std::uint32_t n, const void* b) {
    return respond(c, n, b);
}
LV2_Worker_Status response(LV2_Handle h, std::uint32_t n, const void*) {
    if (n == 4)
        static_cast<Effect*>(h)->worked += 1;
    return LV2_WORKER_SUCCESS;
}
const void* extension(const char* uri) {
    static const LV2_State_Interface s{save, restore};
    static const LV2_Worker_Interface w{work, response, nullptr};
    if (!std::strcmp(uri, LV2_STATE__interface))
        return &s;
    if (!std::strcmp(uri, LV2_WORKER__interface))
        return &w;
    return nullptr;
}
const LV2_Descriptor plugin{"urn:cydsp:lv2-fixture",
                            instantiate,
                            connect,
                            nullptr,
                            run,
                            nullptr,
                            [](LV2_Handle h) { delete static_cast<Effect*>(h); },
                            extension};
struct Ui {
    LV2UI_Write_Function write;
    LV2UI_Controller host;
#if defined(_WIN32)
    HWND child = nullptr;
#endif
};
LV2UI_Handle createUi(const LV2UI_Descriptor*, const char*, const char*, LV2UI_Write_Function write,
                      LV2UI_Controller host, LV2UI_Widget* widget, const LV2_Feature* const* fs) {
#if defined(_WIN32)
    HWND parent = nullptr;
    LV2UI_Resize* resize = nullptr;
    for (; fs && *fs; ++fs) {
        if (!std::strcmp((*fs)->URI, LV2_UI__parent))
            parent = static_cast<HWND>((*fs)->data);
        if (!std::strcmp((*fs)->URI, LV2_UI__resize))
            resize = static_cast<LV2UI_Resize*>((*fs)->data);
    }
    auto* ui = new Ui{write, host};
    if (resize)
        resize->ui_resize(resize->handle, 400, 200);
    ui->child = CreateWindowExW(0, L"STATIC", L"CyDSP LV2 fixture", WS_CHILD, 0, 0, 400, 200,
                                parent, nullptr, GetModuleHandleW(nullptr), nullptr);
    if (!ui->child) {
        delete ui;
        return nullptr;
    }
    *widget = ui->child;
    return ui;
#else
    (void)write;
    (void)host;
    (void)widget;
    (void)fs;
    return nullptr;
#endif
}
void cleanupUi(LV2UI_Handle h) {
    auto* u = static_cast<Ui*>(h);
#if defined(_WIN32)
    if (u->child)
        DestroyWindow(u->child);
#endif
    delete u;
}
int resizeUi(void* h, int w, int height) {
#if defined(_WIN32)
    return MoveWindow(static_cast<Ui*>(h)->child, 0, 0, w, height, TRUE) ? 0 : 1;
#else
    (void)h;
    (void)w;
    (void)height;
    return 1;
#endif
}
const void* uiExtension(const char* uri) {
    static const LV2UI_Resize r{nullptr, resizeUi};
    return !std::strcmp(uri, LV2_UI__resize) ? &r : nullptr;
}
const LV2UI_Descriptor editor{"urn:cydsp:lv2-fixture-ui", createUi, cleanupUi, nullptr,
                              uiExtension};
} // namespace
#if defined(_WIN32)
#define EXPORT __declspec(dllexport)
#else
#define EXPORT __attribute__((visibility("default")))
#endif
extern "C" EXPORT const LV2_Descriptor* lv2_descriptor(std::uint32_t index) {
    return index ? nullptr : &plugin;
}
extern "C" EXPORT const LV2UI_Descriptor* lv2ui_descriptor(std::uint32_t index) {
    return index ? nullptr : &editor;
}
