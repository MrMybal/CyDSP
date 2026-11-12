// Original adapter, pinned MIT interfaces: third_party/licenses/clap.txt
#include "ClapEvents.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <mutex>
#if !defined(_WIN32)
#include <poll.h>
#endif
namespace cydsp::detail {
namespace {
std::recursive_mutex& moduleMutex() {
    static std::recursive_mutex m;
    return m;
}
struct ClapModule {
    SharedModule library;
    const clap_plugin_entry_t* entry = nullptr;
    const clap_plugin_factory_t* factory = nullptr;
    bool initialized = false;
    ~ClapModule() {
        if (initialized) {
            std::lock_guard lock(moduleMutex());
            entry->deinit();
        }
    }
};
struct ModuleRecord {
    std::filesystem::path path;
    std::weak_ptr<ClapModule> module;
};
Status loadModule(const std::filesystem::path& path, std::shared_ptr<ClapModule>& out) {
    std::error_code error;
    const auto canonical = std::filesystem::canonical(path, error);
    if (error || !canonical.is_absolute())
        return Status::io_error;
    static std::vector<ModuleRecord> modules;
    std::unique_lock lock(moduleMutex());
    for (auto it = modules.begin(); it != modules.end();) {
        if (auto m = it->module.lock()) {
            if (it->path == canonical) {
                out = std::move(m);
                return Status::ok;
            }
            ++it;
        } else
            it = modules.erase(it);
    }
    auto next = std::make_shared<ClapModule>();
    auto r = next->library.open(canonical);
    if (r == Status::ok) {
        next->entry = static_cast<const clap_plugin_entry_t*>(next->library.symbol("clap_entry"));
        const auto* e = next->entry;
        if (!e || !clap_version_is_compatible(e->clap_version) || !e->init || !e->deinit ||
            !e->get_factory)
            r = Status::incompatible_version;
        else {
            const auto utf8 = canonical.u8string();
            next->initialized = e->init(reinterpret_cast<const char*>(utf8.c_str()));
            if (!next->initialized)
                r = Status::io_error;
            else {
                next->factory = static_cast<const clap_plugin_factory_t*>(
                    e->get_factory(CLAP_PLUGIN_FACTORY_ID));
                if (!next->factory || !next->factory->create_plugin ||
                    !next->factory->get_plugin_count || !next->factory->get_plugin_descriptor)
                    r = Status::unsupported_format;
            }
        }
    }
    if (r == Status::ok) {
        modules.push_back({canonical, next});
        out = next;
    }
    lock.unlock(); // Last reference/deinit may acquire the registry mutex.
    return r;
}
std::string text(const char* p) {
    if (!p)
        return {};
    std::size_t n = 0;
    while (n < 65536 && p[n])
        ++n;
    if (n == 65536)
        throw std::length_error("plugin string limit");
    return {p, n};
}
template <std::size_t N> std::string textFixed(const char (&p)[N]) {
    return {p, std::find(p, p + N, char{})};
}
PluginDescriptor descriptor(const clap_plugin_descriptor_t* d) {
    if (!d || !clap_version_is_compatible(d->clap_version))
        throw std::invalid_argument("CLAP version");
    PluginDescriptor result{PluginFormat::clap, text(d->id),      text(d->name),
                            text(d->vendor),    text(d->version), text(d->description)};
    if (result.id.empty() || result.name.empty())
        throw std::invalid_argument("CLAP descriptor");
    return result;
}
std::uint64_t milliseconds() noexcept {
    const auto n = std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::steady_clock::now().time_since_epoch())
                       .count();
    return n > 0 ? std::uint64_t(n) : 0;
}
class ClapBackend final : public PluginBackend {
    std::shared_ptr<ClapModule> module_;
    const clap_plugin_t* plugin_ = nullptr;
    clap_host_t host_{};
    PluginDescription description_;
    PluginHostCallbacks callbacks_;
    std::thread::id main_ = std::this_thread::get_id();
    std::atomic<std::uint32_t> requests_{0};
    const clap_plugin_audio_ports_t* audio_ = nullptr;
    const clap_plugin_note_ports_t* notes_ = nullptr;
    const clap_plugin_params_t* params_ = nullptr;
    const clap_plugin_state_t* state_ = nullptr;
    const clap_plugin_gui_t* gui_ = nullptr;
    const clap_plugin_latency_t* latency_ = nullptr;
    const clap_plugin_tail_t* tail_ = nullptr;
    const clap_plugin_render_t* render_ = nullptr;
    const clap_plugin_timer_support_t* timers_ = nullptr;
    const clap_plugin_posix_fd_support_t* fds_ = nullptr;
    struct ParameterCookie {
        std::uint32_t id;
        void* cookie;
    };
    std::vector<ParameterCookie> cookies_;
    std::vector<clap_audio_buffer_t> inputs_, outputs_;
    std::vector<float*> pointers32_;
    std::vector<double*> pointers64_;
    std::vector<ClapEventStorage> inputEvents_;
    std::uint32_t inputCount_ = 0, frames_ = 0;
    PluginEventBuffer* outputEvents_ = nullptr;
    clap_input_events_t in_{};
    clap_output_events_t out_{};
    struct Timer {
        clap_id id = 0;
        std::uint32_t period = 0;
        std::uint64_t next = 0;
    };
    std::array<Timer, 64> timerSlots_{};
    struct Fd {
        int fd = -1;
        std::uint32_t flags = 0;
    };
    std::array<Fd, 64> fdSlots_{};
    clap_id nextTimer_ = 1;
    bool active_ = false, editor_ = false, floating_ = false, cocoa_ = false;
    static ClapBackend& self(const clap_host_t* h) noexcept {
        return *static_cast<ClapBackend*>(h->host_data);
    }
    void request(std::uint32_t flag) noexcept {
        requests_.fetch_or(flag, std::memory_order_relaxed);
    }
    bool onMain() const noexcept {
        return main_ == std::this_thread::get_id() && currentAudioPlugin != this;
    }
    template <class T> const T* extension(const char* id) noexcept {
        return static_cast<const T*>(plugin_->get_extension(plugin_, id));
    }
    static const void* CLAP_ABI getExtension(const clap_host_t*, const char* id) noexcept;
    Status convert(const PluginEventBuffer* in) noexcept {
        inputCount_ = 0;
        if (!in)
            return Status::ok;
        if (in->size() > inputEvents_.size())
            return Status::block_too_large;
        for (std::size_t i = 0; i < in->size(); ++i) {
            const auto& e = *in->event(i);
            void* cookie = nullptr;
            if (e.kind == PluginEventKind::parameterValue ||
                e.kind == PluginEventKind::parameterModulation ||
                e.kind == PluginEventKind::gestureBegin || e.kind == PluginEventKind::gestureEnd) {
                const auto found = std::lower_bound(cookies_.begin(), cookies_.end(), e.id,
                                                    [](auto a, auto b) { return a.id < b; });
                if (found == cookies_.end() || found->id != e.id)
                    return Status::invalid_argument;
                cookie = found->cookie;
                const auto& p = *std::lower_bound(description_.parameters.begin(),
                                                  description_.parameters.end(), e.id,
                                                  [](const auto& a, auto b) { return a.id < b; });
                if (e.kind == PluginEventKind::parameterValue &&
                    (p.readOnly || e.value < p.minimum || e.value > p.maximum))
                    return Status::invalid_argument;
                if (e.kind == PluginEventKind::parameterModulation && !p.modulatable)
                    return Status::unsupported_format;
                const auto targets = e.kind == PluginEventKind::parameterModulation
                                         ? p.modulationTargets
                                         : p.automationTargets;
                if ((e.noteId >= 0 && !(targets & parameterNoteId)) ||
                    (e.port >= 0 && !(targets & parameterPort)) ||
                    (e.channel >= 0 && !(targets & parameterChannel)) ||
                    (e.key >= 0 && !(targets & parameterKey)))
                    return Status::unsupported_format;
            }
            std::uint32_t dialect = 0;
            switch (e.kind) {
            case PluginEventKind::noteEnd:
                return Status::invalid_argument;
            case PluginEventKind::noteOn:
            case PluginEventKind::noteOff:
            case PluginEventKind::noteChoke:
            case PluginEventKind::noteExpression:
                dialect = CLAP_NOTE_DIALECT_CLAP;
                break;
            case PluginEventKind::midi1:
            case PluginEventKind::sysex:
                dialect = CLAP_NOTE_DIALECT_MIDI | CLAP_NOTE_DIALECT_MIDI_MPE;
                break;
            case PluginEventKind::ump:
                dialect = CLAP_NOTE_DIALECT_MIDI2;
                break;
            default:
                break;
            }
            if (dialect) {
                if (e.port >= 0) {
                    if (std::size_t(e.port) >= description_.noteInputs.size() ||
                        !(description_.noteInputs[std::size_t(e.port)].dialects & dialect))
                        return Status::unsupported_format;
                } else if (description_.noteInputs.empty())
                    return Status::unsupported_format;
            }
            encodeClapEvent(e, inputEvents_[inputCount_++], cookie);
        }
        return Status::ok;
    }
    template <Sample T> Status processing(const PluginProcess<T>& p) noexcept {
        const auto converted = convert(p.inputEvents);
        if (converted != Status::ok)
            return converted;
        outputEvents_ = p.outputEvents;
        frames_ = p.frames;
        std::size_t offset = 0;
        const auto buffers = [&](const auto& blocks, auto& native) {
            for (std::size_t i = 0; i < blocks.size(); ++i) {
                auto& b = native[i];
                b = {};
                b.channel_count = std::uint32_t(blocks[i].channels());
                for (std::size_t ch = 0; ch < blocks[i].channels(); ++ch) {
                    if constexpr (std::same_as<T, float>)
                        pointers32_[offset + ch] = const_cast<T*>(blocks[i].channel(ch).data());
                    else
                        pointers64_[offset + ch] = const_cast<T*>(blocks[i].channel(ch).data());
                }
                if constexpr (std::same_as<T, float>)
                    b.data32 = pointers32_.data() + offset;
                else
                    b.data64 = pointers64_.data() + offset;
                offset += blocks[i].channels();
            }
        };
        buffers(p.inputs, inputs_);
        buffers(p.outputs, outputs_);
        clap_event_transport_t transport{};
        if (p.transport)
            transport = clapTransport(*p.transport);
        clap_process_t call{p.steadyFrame,
                            p.frames,
                            p.transport ? &transport : nullptr,
                            inputs_.data(),
                            outputs_.data(),
                            std::uint32_t(inputs_.size()),
                            std::uint32_t(outputs_.size()),
                            &in_,
                            &out_};
        const auto r = plugin_->process(plugin_, &call);
        outputEvents_ = nullptr;
        return r >= CLAP_PROCESS_CONTINUE && r <= CLAP_PROCESS_SLEEP ? Status::ok
                                                                     : Status::io_error;
    }

  public:
    explicit ClapBackend(std::shared_ptr<ClapModule> module, const PluginHostCallbacks& cb)
        : module_(std::move(module)), callbacks_(cb) {
        host_ = {CLAP_VERSION,
                 this,
                 "CyDSP",
                 "MrMybal CyberAlien",
                 "https://github.com/MrMybal/CyDSP",
                 "0.1.0",
                 &getExtension,
                 [](const clap_host_t* h) noexcept { self(h).request(pluginRestart); },
                 [](const clap_host_t* h) noexcept { self(h).request(pluginProcess); },
                 [](const clap_host_t* h) noexcept { self(h).request(pluginMainThread); }};
        in_ = {this,
               [](const clap_input_events_t* l) noexcept {
                   return static_cast<ClapBackend*>(l->ctx)->inputCount_;
               },
               [](const clap_input_events_t* l,
                  std::uint32_t i) noexcept -> const clap_event_header_t* {
                   auto& s = *static_cast<ClapBackend*>(l->ctx);
                   return i < s.inputCount_ ? &s.inputEvents_[i].note.header : nullptr;
               }};
        out_ = {this, [](const clap_output_events_t* l, const clap_event_header_t* e) noexcept {
                    auto& s = *static_cast<ClapBackend*>(l->ctx);
                    return s.outputEvents_ &&
                           decodeClapEvent(e, *s.outputEvents_, s.frames_) == Status::ok;
                }};
    }
    ~ClapBackend() override {
        if (plugin_) {
            closeEditor();
            if (active_)
                plugin_->deactivate(plugin_);
            plugin_->destroy(plugin_);
        }
    }
    Status initialize(std::string_view id) {
        if (id.empty() || id.size() > 65536 || id.find(char{}) != std::string_view::npos)
            return Status::invalid_argument;
        const std::string identifier(id);
        plugin_ = module_->factory->create_plugin(module_->factory, &host_, identifier.c_str());
        if (!plugin_)
            return Status::unsupported_format;
        if (!plugin_->destroy) {
            plugin_ = nullptr;
            return Status::unsupported_format;
        }
        if (!plugin_->init || !plugin_->activate || !plugin_->deactivate ||
            !plugin_->start_processing || !plugin_->stop_processing || !plugin_->reset ||
            !plugin_->process || !plugin_->get_extension || !plugin_->on_main_thread)
            return Status::unsupported_format;
        if (!plugin_->init(plugin_))
            return Status::io_error;
        audio_ = extension<clap_plugin_audio_ports_t>(CLAP_EXT_AUDIO_PORTS);
        notes_ = extension<clap_plugin_note_ports_t>(CLAP_EXT_NOTE_PORTS);
        params_ = extension<clap_plugin_params_t>(CLAP_EXT_PARAMS);
        state_ = extension<clap_plugin_state_t>(CLAP_EXT_STATE);
        gui_ = extension<clap_plugin_gui_t>(CLAP_EXT_GUI);
        latency_ = extension<clap_plugin_latency_t>(CLAP_EXT_LATENCY);
        tail_ = extension<clap_plugin_tail_t>(CLAP_EXT_TAIL);
        render_ = extension<clap_plugin_render_t>(CLAP_EXT_RENDER);
        timers_ = extension<clap_plugin_timer_support_t>(CLAP_EXT_TIMER_SUPPORT);
        fds_ = extension<clap_plugin_posix_fd_support_t>(CLAP_EXT_POSIX_FD_SUPPORT);
        inputEvents_.resize(1024);
        return refresh();
    }
    const PluginDescription& description() const noexcept override {
        return description_;
    }
    Status refresh() noexcept override {
        if (active_)
            return Status::transition_in_progress;
        try {
            PluginDescription next;
            next.plugin = descriptor(plugin_->desc);
            next.state = state_ && state_->save && state_->load;
            next.editor = gui_ && gui_->create && gui_->destroy;
            next.float64 = true;
            std::size_t channels = 0;
            for (bool input : {true, false}) {
                auto& list = input ? next.inputs : next.outputs;
                const auto count =
                    audio_ && audio_->count && audio_->get ? audio_->count(plugin_, input) : 0;
                if (count > 64)
                    return Status::block_too_large;
                for (std::uint32_t i = 0; i < count; ++i) {
                    clap_audio_port_info_t port{};
                    if (!audio_->get(plugin_, i, input, &port) || !port.channel_count ||
                        port.channel_count > 256)
                        return Status::unsupported_format;
                    channels += port.channel_count;
                    list.push_back({port.id, port.channel_count, port.in_place_pair,
                                    textFixed(port.name), text(port.port_type),
                                    (port.flags & CLAP_AUDIO_PORT_IS_MAIN) != 0,
                                    (port.flags & CLAP_AUDIO_PORT_SUPPORTS_64BITS) != 0});
                    next.float64 = next.float64 && list.back().supports64;
                }
                auto& noteList = input ? next.noteInputs : next.noteOutputs;
                const auto noteCount =
                    notes_ && notes_->count && notes_->get ? notes_->count(plugin_, input) : 0;
                if (noteCount > 64)
                    return Status::block_too_large;
                for (std::uint32_t i = 0; i < noteCount; ++i) {
                    clap_note_port_info_t port{};
                    if (!notes_->get(plugin_, i, input, &port))
                        return Status::unsupported_format;
                    noteList.push_back({port.id, port.supported_dialects, port.preferred_dialect,
                                        textFixed(port.name)});
                }
            }
            std::vector<ParameterCookie> cookies;
            const auto count =
                params_ && params_->count && params_->get_info ? params_->count(plugin_) : 0;
            if (count > 65536)
                return Status::block_too_large;
            for (std::uint32_t i = 0; i < count; ++i) {
                clap_param_info_t p{};
                if (!params_->get_info(plugin_, i, &p) || p.id == CLAP_INVALID_ID ||
                    !isFinite(p.min_value) || !isFinite(p.max_value) ||
                    !isFinite(p.default_value) || p.min_value > p.max_value ||
                    p.default_value < p.min_value || p.default_value > p.max_value)
                    return Status::unsupported_format;
                next.parameters.push_back({p.id, textFixed(p.name), textFixed(p.module),
                                           p.min_value, p.max_value, p.default_value,
                                           (p.flags & CLAP_PARAM_IS_STEPPED) != 0,
                                           (p.flags & CLAP_PARAM_IS_READONLY) != 0,
                                           (p.flags & CLAP_PARAM_IS_AUTOMATABLE) != 0,
                                           (p.flags & CLAP_PARAM_IS_MODULATABLE) != 0,
                                           (p.flags & CLAP_PARAM_IS_HIDDEN) != 0});
                auto& parameter = next.parameters.back();
                parameter.automationTargets =
                    ((p.flags & CLAP_PARAM_IS_AUTOMATABLE_PER_NOTE_ID) ? parameterNoteId : 0u) |
                    ((p.flags & CLAP_PARAM_IS_AUTOMATABLE_PER_PORT) ? parameterPort : 0u) |
                    ((p.flags & CLAP_PARAM_IS_AUTOMATABLE_PER_CHANNEL) ? parameterChannel : 0u) |
                    ((p.flags & CLAP_PARAM_IS_AUTOMATABLE_PER_KEY) ? parameterKey : 0u);
                parameter.modulationTargets =
                    ((p.flags & CLAP_PARAM_IS_MODULATABLE_PER_NOTE_ID) ? parameterNoteId : 0u) |
                    ((p.flags & CLAP_PARAM_IS_MODULATABLE_PER_PORT) ? parameterPort : 0u) |
                    ((p.flags & CLAP_PARAM_IS_MODULATABLE_PER_CHANNEL) ? parameterChannel : 0u) |
                    ((p.flags & CLAP_PARAM_IS_MODULATABLE_PER_KEY) ? parameterKey : 0u);
                cookies.push_back({p.id, p.cookie});
            }
            std::sort(next.parameters.begin(), next.parameters.end(),
                      [](const auto& a, const auto& b) { return a.id < b.id; });
            std::sort(cookies.begin(), cookies.end(), [](auto a, auto b) { return a.id < b.id; });
            for (std::size_t i = 1; i < cookies.size(); ++i)
                if (cookies[i - 1].id == cookies[i].id)
                    return Status::unsupported_format;
            std::vector<clap_audio_buffer_t> ins(next.inputs.size()), outs(next.outputs.size());
            std::vector<float*> p32(channels);
            std::vector<double*> p64(channels);
            description_ = std::move(next);
            cookies_.swap(cookies);
            inputs_.swap(ins);
            outputs_.swap(outs);
            pointers32_.swap(p32);
            pointers64_.swap(p64);
            return Status::ok;
        } catch (const std::bad_alloc&) {
            return Status::allocation_failed;
        } catch (...) {
            return Status::unsupported_format;
        }
    }
    Status activate(const PluginPrepareOptions& o) noexcept override {
        try {
            inputEvents_.resize(o.maximumEvents);
        } catch (...) {
            return Status::allocation_failed;
        }
        if (render_ && render_->set &&
            !render_->set(plugin_, o.offline ? CLAP_RENDER_OFFLINE : CLAP_RENDER_REALTIME))
            return Status::unsupported_format;
        active_ = plugin_->activate(plugin_, o.sampleRate, o.minimumFrames, o.maximumFrames);
        return active_ ? Status::ok : Status::invalid_spec;
    }
    Status deactivate() noexcept override {
        if (active_)
            plugin_->deactivate(plugin_);
        active_ = false;
        return Status::ok;
    }
    Status start() noexcept override {
        return plugin_->start_processing(plugin_) ? Status::ok : Status::io_error;
    }
    void stop() noexcept override {
        plugin_->stop_processing(plugin_);
    }
    Status reset() noexcept override {
        plugin_->reset(plugin_);
        return Status::ok;
    }
    Status process(const PluginProcess<float>& p) noexcept override {
        return processing(p);
    }
    Status process(const PluginProcess<double>& p) noexcept override {
        return processing(p);
    }
    PluginTiming timing() const noexcept override {
        PluginTiming t;
        t.latencyKnown = active_ && latency_ && latency_->get;
        t.tailKnown = tail_ && tail_->get;
        if (t.latencyKnown)
            t.latencyFrames = latency_->get(plugin_);
        if (t.tailKnown) {
            t.tailFrames = tail_->get(plugin_);
            t.infiniteTail = t.tailFrames >= 0x7fffffffu;
        }
        return t;
    }
    Status parameterValue(std::uint32_t id, double& value) noexcept override {
        if (!params_ || !params_->get_value)
            return Status::unsupported_format;
        double v = 0;
        if (!params_->get_value(plugin_, id, &v) || !isFinite(v))
            return Status::invalid_argument;
        value = v;
        return Status::ok;
    }
    Status parameterText(std::uint32_t id, double v, std::string& out) noexcept override {
        if (!params_ || !params_->value_to_text)
            return Status::unsupported_format;
        std::array<char, 4096> buffer{};
        if (!params_->value_to_text(plugin_, id, v, buffer.data(), std::uint32_t(buffer.size())))
            return Status::invalid_argument;
        try {
            const auto end = std::find(buffer.begin(), buffer.end(), char{});
            if (end == buffer.end())
                return Status::invalid_argument;
            out.assign(buffer.data(), std::size_t(end - buffer.begin()));
            return Status::ok;
        } catch (...) {
            return Status::allocation_failed;
        }
    }
    Status flush(const PluginEventBuffer& in, PluginEventBuffer& out) noexcept override {
        if (!params_ || !params_->flush)
            return Status::unsupported_format;
        const auto r = convert(&in);
        if (r != Status::ok)
            return r;
        outputEvents_ = &out;
        frames_ = std::numeric_limits<std::uint32_t>::max();
        params_->flush(plugin_, &in_, &out_);
        outputEvents_ = nullptr;
        return Status::ok;
    }
    Status saveState(std::vector<std::uint8_t>& result, std::size_t limit) noexcept override {
        if (!state_ || !state_->save)
            return Status::unsupported_format;
        struct Output {
            std::vector<std::uint8_t> bytes;
            std::size_t limit;
            bool failed = false;
        } output{{}, limit};
        clap_ostream_t stream{&output,
                              [](const clap_ostream_t* s, const void* data,
                                 std::uint64_t size) noexcept -> std::int64_t {
                                  auto& o = *static_cast<Output*>(s->ctx);
                                  if (!data || size > o.limit - o.bytes.size()) {
                                      o.failed = true;
                                      return -1;
                                  }
                                  try {
                                      const auto* p = static_cast<const std::uint8_t*>(data);
                                      o.bytes.insert(o.bytes.end(), p, p + std::size_t(size));
                                      return std::int64_t(size);
                                  } catch (...) {
                                      o.failed = true;
                                      return -1;
                                  }
                              }};
        if (!state_->save(plugin_, &stream) || output.failed)
            return Status::io_error;
        result.swap(output.bytes);
        return Status::ok;
    }
    Status loadState(std::span<const std::uint8_t> data) noexcept override {
        if (!state_ || !state_->load)
            return Status::unsupported_format;
        struct Input {
            std::span<const std::uint8_t> bytes;
            std::size_t at = 0;
        } input{data};
        clap_istream_t stream{
            &input,
            [](const clap_istream_t* s, void* target, std::uint64_t size) noexcept -> std::int64_t {
                auto& in = *static_cast<Input*>(s->ctx);
                if (!target)
                    return -1;
                const auto n = std::size_t(std::min<std::uint64_t>(size, in.bytes.size() - in.at));
                if (n)
                    std::memcpy(target, in.bytes.data() + in.at, n);
                in.at += n;
                return std::int64_t(n);
            }};
        return state_->load(plugin_, &stream) ? Status::ok : Status::io_error;
    }
    Status openEditor(const PluginWindow& window, PluginEditorSize& size) noexcept override {
        if (!gui_ || !gui_->is_api_supported || !gui_->create || !gui_->destroy ||
            !gui_->get_size || !gui_->show || !gui_->hide)
            return Status::unsupported_format;
        const char* api = nullptr;
        switch (window.api) {
        case PluginWindowApi::win32:
            api = CLAP_WINDOW_API_WIN32;
            break;
        case PluginWindowApi::cocoa:
            api = CLAP_WINDOW_API_COCOA;
            break;
        case PluginWindowApi::x11:
            api = CLAP_WINDOW_API_X11;
            break;
        case PluginWindowApi::wayland:
            api = CLAP_WINDOW_API_WAYLAND;
            break;
        }
        if (!api || (!window.floating && window.api == PluginWindowApi::wayland) ||
            !gui_->is_api_supported(plugin_, api, window.floating))
            return Status::unsupported_backend;
        if (!gui_->create(plugin_, api, window.floating))
            return Status::io_error;
        editor_ = true;
        floating_ = window.floating;
        cocoa_ = window.api == PluginWindowApi::cocoa;
        if (!cocoa_ && gui_->set_scale)
            (void)gui_->set_scale(plugin_, window.scale);
        clap_window_t parent{};
        parent.api = api;
        if (window.api == PluginWindowApi::x11)
            parent.x11 = static_cast<unsigned long>(window.handle);
        else
            parent.ptr = reinterpret_cast<void*>(window.handle);
        if (!window.floating && (!gui_->set_parent || !gui_->set_parent(plugin_, &parent))) {
            closeEditor();
            return Status::io_error;
        }
        if (window.floating && window.handle && gui_->set_transient &&
            !gui_->set_transient(plugin_, &parent)) {
            closeEditor();
            return Status::io_error;
        }
        PluginEditorSize actual;
        if (!gui_->get_size(plugin_, &actual.width, &actual.height) || !actual.width ||
            !actual.height || actual.width > 32768 || actual.height > 32768 ||
            !gui_->show(plugin_)) {
            closeEditor();
            return Status::io_error;
        }
        size = actual;
        return Status::ok;
    }
    Status resizeEditor(PluginEditorSize& size) noexcept override {
        if (!editor_ || floating_ || !gui_->can_resize || !gui_->can_resize(plugin_) ||
            !gui_->adjust_size || !gui_->set_size)
            return Status::unsupported_format;
        auto adjusted = size;
        if (!gui_->adjust_size(plugin_, &adjusted.width, &adjusted.height) || !adjusted.width ||
            !adjusted.height || adjusted.width > 32768 || adjusted.height > 32768 ||
            !gui_->set_size(plugin_, adjusted.width, adjusted.height))
            return Status::io_error;
        size = adjusted;
        return Status::ok;
    }
    Status scaleEditor(double scale) noexcept override {
        if (cocoa_)
            return Status::unsupported_format;
        return gui_->set_scale && gui_->set_scale(plugin_, scale) ? Status::ok
                                                                  : Status::unsupported_format;
    }
    Status showEditor(bool show) noexcept override {
        return (show ? gui_->show(plugin_) : gui_->hide(plugin_)) ? Status::ok : Status::io_error;
    }
    void closeEditor() noexcept override {
        if (editor_) {
            (void)gui_->hide(plugin_);
            gui_->destroy(plugin_);
            editor_ = false;
        }
    }
    std::uint32_t pumpMainThread() noexcept override {
        auto flags = requests_.exchange(0, std::memory_order_acq_rel);
        if (flags & pluginMainThread)
            plugin_->on_main_thread(plugin_);
        if (flags & pluginEditorClosed)
            closeEditor();
        else if (editor_) {
            if (flags & pluginEditorShow)
                (void)gui_->show(plugin_);
            if (flags & pluginEditorHide)
                (void)gui_->hide(plugin_);
        }
        const auto now = milliseconds();
        if (timers_ && timers_->on_timer)
            for (auto& t : timerSlots_)
                if (t.id && now >= t.next) {
                    const auto id = t.id;
                    t.next = now + t.period;
                    timers_->on_timer(plugin_, id);
                }
#if !defined(_WIN32)
        if (fds_ && fds_->on_fd) {
            std::array<pollfd, 64> fds{};
            std::size_t count = 0;
            for (const auto& fd : fdSlots_)
                if (fd.fd >= 0)
                    fds[count++] = {fd.fd,
                                    short(((fd.flags & CLAP_POSIX_FD_READ) ? POLLIN : 0) |
                                          ((fd.flags & CLAP_POSIX_FD_WRITE) ? POLLOUT : 0)),
                                    0};
            if (::poll(fds.data(), static_cast<nfds_t>(count), 0) > 0)
                for (std::size_t i = 0; i < count; ++i)
                    if (fds[i].revents) {
                        std::uint32_t ready = 0;
                        if (fds[i].revents & POLLIN)
                            ready |= CLAP_POSIX_FD_READ;
                        if (fds[i].revents & POLLOUT)
                            ready |= CLAP_POSIX_FD_WRITE;
                        if (fds[i].revents & (POLLERR | POLLHUP | POLLNVAL))
                            ready |= CLAP_POSIX_FD_ERROR;
                        bool registered = false;
                        for (const auto& fd : fdSlots_)
                            if (fd.fd == fds[i].fd)
                                registered = true;
                        if (registered)
                            fds_->on_fd(plugin_, fds[i].fd, ready);
                    }
        }
#endif
        // Requests raised by callbacks/timers remain pending for the next pump.
        if (callbacks_.requests && flags)
            callbacks_.requests(callbacks_.context, flags);
        return flags;
    }
};
const void* CLAP_ABI ClapBackend::getExtension(const clap_host_t*, const char* id) noexcept {
    if (!id)
        return nullptr;
    static const clap_host_thread_check_t thread{
        [](const clap_host_t* h) noexcept { return self(h).onMain(); },
        [](const clap_host_t* h) noexcept { return currentAudioPlugin == &self(h); }};
    static const clap_host_latency_t latency{
        [](const clap_host_t* h) noexcept { self(h).request(pluginLatencyChanged); }};
    static const clap_host_tail_t tail{
        [](const clap_host_t* h) noexcept { self(h).request(pluginTailChanged); }};
    static const clap_host_state_t state{
        [](const clap_host_t* h) noexcept { self(h).request(pluginStateDirty); }};
    static const clap_host_params_t params{
        [](const clap_host_t* h, std::uint32_t) noexcept {
            self(h).request(pluginParametersChanged);
        },
        [](const clap_host_t* h, clap_id, std::uint32_t) noexcept {
            self(h).request(pluginParametersChanged);
        },
        [](const clap_host_t* h) noexcept { self(h).request(pluginProcess); }};
    static const clap_host_audio_ports_t audio{
        [](const clap_host_t*, std::uint32_t flags) noexcept {
            return flags && !(flags & ~std::uint32_t(63));
        },
        [](const clap_host_t* h, std::uint32_t) noexcept { self(h).request(pluginPortsChanged); }};
    static const clap_host_note_ports_t notes{
        [](const clap_host_t*) noexcept {
            return std::uint32_t(CLAP_NOTE_DIALECT_CLAP | CLAP_NOTE_DIALECT_MIDI |
                                 CLAP_NOTE_DIALECT_MIDI_MPE | CLAP_NOTE_DIALECT_MIDI2);
        },
        [](const clap_host_t* h, std::uint32_t) noexcept { self(h).request(pluginPortsChanged); }};
    static const clap_host_gui_t gui{
        [](const clap_host_t* h) noexcept { self(h).request(pluginEditorResize); },
        [](const clap_host_t* h, std::uint32_t width, std::uint32_t height) noexcept {
            auto& s = self(h);
            if (!s.onMain() || !s.editor_ || s.floating_ || !width || !height || width > 32768 ||
                height > 32768 || !s.callbacks_.resizeEditor)
                return false;
            return s.callbacks_.resizeEditor(s.callbacks_.context, {width, height});
        },
        [](const clap_host_t* h) noexcept {
            self(h).request(pluginEditorShow);
            return true;
        },
        [](const clap_host_t* h) noexcept {
            self(h).request(pluginEditorHide);
            return true;
        },
        [](const clap_host_t* h, bool) noexcept { self(h).request(pluginEditorClosed); }};
    static const clap_host_timer_support_t timers{
        [](const clap_host_t* h, std::uint32_t period, clap_id* id) noexcept {
            auto& s = self(h);
            if (!s.onMain() || !period || !id || !s.nextTimer_ || s.nextTimer_ == CLAP_INVALID_ID)
                return false;
            for (auto& timer : s.timerSlots_)
                if (!timer.id) {
                    timer = {s.nextTimer_++, std::max(period, 1u), milliseconds() + period};
                    *id = timer.id;
                    return true;
                }
            return false;
        },
        [](const clap_host_t* h, clap_id id) noexcept {
            auto& s = self(h);
            if (!s.onMain() || !id)
                return false;
            for (auto& timer : s.timerSlots_)
                if (timer.id == id) {
                    timer = {};
                    return true;
                }
            return false;
        }};
#if !defined(_WIN32)
    static const clap_host_posix_fd_support_t fds{
        [](const clap_host_t* h, int fd, std::uint32_t flags) noexcept {
            auto& s = self(h);
            if (!s.onMain() || fd < 0 || !flags || (flags & ~7u))
                return false;
            for (const auto& slot : s.fdSlots_)
                if (slot.fd == fd)
                    return false;
            for (auto& slot : s.fdSlots_)
                if (slot.fd < 0) {
                    slot = {fd, flags};
                    return true;
                }
            return false;
        },
        [](const clap_host_t* h, int fd, std::uint32_t flags) noexcept {
            auto& s = self(h);
            if (!s.onMain() || !flags || (flags & ~7u))
                return false;
            for (auto& slot : s.fdSlots_)
                if (slot.fd == fd) {
                    slot.flags = flags;
                    return true;
                }
            return false;
        },
        [](const clap_host_t* h, int fd) noexcept {
            auto& s = self(h);
            if (!s.onMain())
                return false;
            for (auto& slot : s.fdSlots_)
                if (slot.fd == fd) {
                    slot = {};
                    return true;
                }
            return false;
        }};
    if (std::strcmp(id, CLAP_EXT_POSIX_FD_SUPPORT) == 0)
        return &fds;
#endif
    if (std::strcmp(id, CLAP_EXT_THREAD_CHECK) == 0)
        return &thread;
    if (std::strcmp(id, CLAP_EXT_LATENCY) == 0)
        return &latency;
    if (std::strcmp(id, CLAP_EXT_TAIL) == 0)
        return &tail;
    if (std::strcmp(id, CLAP_EXT_STATE) == 0)
        return &state;
    if (std::strcmp(id, CLAP_EXT_PARAMS) == 0)
        return &params;
    if (std::strcmp(id, CLAP_EXT_AUDIO_PORTS) == 0)
        return &audio;
    if (std::strcmp(id, CLAP_EXT_NOTE_PORTS) == 0)
        return &notes;
    if (std::strcmp(id, CLAP_EXT_GUI) == 0)
        return &gui;
    if (std::strcmp(id, CLAP_EXT_TIMER_SUPPORT) == 0)
        return &timers;
    return nullptr;
}
class ClapProvider final : public PluginProvider {
  public:
    Status scan(const std::filesystem::path& path,
                std::vector<PluginDescriptor>& result) noexcept override {
        try {
            std::shared_ptr<ClapModule> module;
            const auto r = loadModule(path, module);
            if (r != Status::ok)
                return r;
            const auto count = module->factory->get_plugin_count(module->factory);
            if (count > 4096)
                return Status::block_too_large;
            std::vector<PluginDescriptor> next;
            next.reserve(count);
            for (std::uint32_t i = 0; i < count; ++i)
                next.push_back(
                    descriptor(module->factory->get_plugin_descriptor(module->factory, i)));
            result.swap(next);
            return Status::ok;
        } catch (const std::bad_alloc&) {
            return Status::allocation_failed;
        } catch (...) {
            return Status::unsupported_format;
        }
    }
    Status create(const std::filesystem::path& path, std::string_view id,
                  const PluginHostCallbacks& cb, PluginInstance& out) noexcept override {
        try {
            std::shared_ptr<ClapModule> module;
            auto r = loadModule(path, module);
            if (r != Status::ok)
                return r;
            auto backend = std::make_unique<ClapBackend>(std::move(module), cb);
            r = backend->initialize(id);
            return r == Status::ok ? PluginInstance::adopt(std::move(backend), out) : r;
        } catch (const std::bad_alloc&) {
            return Status::allocation_failed;
        } catch (...) {
            return Status::unsupported_format;
        }
    }
};
} // namespace
std::unique_ptr<PluginProvider> makeClapProvider() {
    return std::make_unique<ClapProvider>();
}
} // namespace cydsp::detail
