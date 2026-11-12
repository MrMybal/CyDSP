#include "cydsp/io/Midi2Device.h"
#include "cydsp/core/SpscQueue.h"
#include "detail/Midi2Backend.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <thread>
namespace cydsp {
namespace {
class SysExQueue {
    struct Meta {
        std::uint64_t timestamp = 0;
        std::size_t size = 0;
        unsigned group = 0;
    };
    std::vector<Meta> metadata_;
    std::vector<std::uint8_t> bytes_;
    std::size_t maximum_ = 0;
    std::atomic<std::size_t> read_{0};
    [[maybe_unused]] std::array<std::byte, 128> separation_{};
    std::atomic<std::size_t> write_{0};

  public:
    struct View {
        const std::uint8_t* bytes = nullptr;
        std::size_t size = 0;
        std::uint64_t timestamp = 0;
        unsigned group = 0;
    };
    Status prepare(std::size_t count, std::size_t maximum) noexcept {
        try {
            std::vector<Meta> metadata(count + 1);
            std::vector<std::uint8_t> bytes((count + 1) * maximum);
            metadata_.swap(metadata);
            bytes_.swap(bytes);
            maximum_ = maximum;
            return Status::ok;
        } catch (...) {
            return Status::allocation_failed;
        }
    }
    bool push(unsigned group, std::span<const std::uint8_t> bytes,
              std::uint64_t timestamp) noexcept {
        const auto w = write_.load(std::memory_order_relaxed), next = (w + 1) % metadata_.size();
        if (next == read_.load(std::memory_order_acquire))
            return false;
        std::copy(bytes.begin(), bytes.end(),
                  bytes_.begin() + static_cast<std::ptrdiff_t>(w * maximum_));
        metadata_[w] = {timestamp, bytes.size(), group};
        write_.store(next, std::memory_order_release);
        return true;
    }
    std::optional<View> peek() const noexcept {
        const auto r = read_.load(std::memory_order_relaxed);
        if (r == write_.load(std::memory_order_acquire))
            return {};
        const auto& m = metadata_[r];
        return View{bytes_.data() + r * maximum_, m.size, m.timestamp, m.group};
    }
    void discard() noexcept {
        const auto r = read_.load(std::memory_order_relaxed);
        read_.store((r + 1) % metadata_.size(), std::memory_order_release);
    }
};
} // namespace
std::uint64_t midi2TimestampNow() noexcept {
    return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
                                          std::chrono::steady_clock::now().time_since_epoch())
                                          .count());
}
Status enumerateMidi2Endpoints(std::vector<Midi2EndpointInfo>& result) noexcept {
    return detail::enumerateNativeMidi2(result);
}
struct Midi2Device::State {
    Midi2DeviceOptions options;
    detail::Midi2NativeRequest request;
    SpscQueue<Midi2TimedPacket> input, output;
    SysExQueue sysex;
    std::thread worker;
    std::atomic<bool> stop{false};
    std::atomic<std::uint32_t> error{0}, dropped{0};
    std::atomic_flag receiving = ATOMIC_FLAG_INIT;
    UmpStreamParser parser;
    std::uint64_t packetTime = 0;
    std::mutex startupMutex;
    std::condition_variable startup;
    bool ready = false;
    Status opened = Status::not_prepared;
    ~State() {
        stop.store(true, std::memory_order_release);
        if (worker.joinable())
            worker.join();
    }
    static void receive(void* ptr, std::span<const std::uint32_t> words,
                        std::uint64_t time) noexcept {
        auto& s = *static_cast<State*>(ptr);
        // Serialize possible concurrent system callbacks without ever waiting.
        if (s.receiving.test_and_set(std::memory_order_acquire)) {
            s.dropped.fetch_add(1, std::memory_order_relaxed);
            return;
        }
        if (words.empty())
            s.parser.reset();
        for (auto word : words) {
            if (!s.parser.incomplete())
                s.packetTime = time;
            const auto status = s.parser.feed(
                {&word, 1},
                [](void* p, const UmpPacket& packet) noexcept {
                    auto& a = *static_cast<State*>(p);
                    if (!a.input.push({packet, a.packetTime}))
                        a.dropped.fetch_add(1, std::memory_order_relaxed);
                },
                &s);
            if (status != Status::ok)
                s.dropped.fetch_add(1, std::memory_order_relaxed);
        }
        s.receiving.clear(std::memory_order_release);
    }
    struct Loopback final : detail::NativeMidi2 {
        State& state;
        explicit Loopback(State& s) : state(s) {}
        Status send(const Midi2TimedPacket& p) noexcept override {
            receive(&state, {p.packet.words.data(), p.packet.count},
                    p.timestampNs ? p.timestampNs : midi2TimestampNow());
            return Status::ok;
        }
        Status poll() noexcept override {
            return Status::ok;
        }
    };
    void run() noexcept {
        std::unique_ptr<detail::NativeMidi2> backend;
        Status status;
        try {
            if (request.endpoint.backend == Midi2Backend::loopback) {
                backend = std::make_unique<Loopback>(*this);
                status = Status::ok;
            }
#if !defined(_WIN32)
            else if (request.endpoint.backend == Midi2Backend::fileDescriptors)
                status = detail::createFdMidi2(request, &receive, this, backend);
#endif
            else
                status = detail::createNativeMidi2(request, &receive, this, backend);
        } catch (const std::bad_alloc&) {
            status = Status::allocation_failed;
        } catch (...) {
            status = Status::device_unavailable;
        }
        {
            std::lock_guard lock(startupMutex);
            opened = status;
            ready = true;
        }
        startup.notify_one();
        if (status != Status::ok)
            return;
        std::size_t sysexOffset = 0;
        unsigned blocked = 0; // 1 normal, 2 SysEx: retry the same partial native packet.
        std::array<bool, 16> rawSysEx{};
        while (!stop.load(std::memory_order_acquire)) {
            status = backend->poll();
            if (status != Status::ok && status != Status::would_block) {
                error.store(static_cast<std::uint32_t>(status), std::memory_order_release);
                break;
            }
            // Bound work per turn, prioritizing normal events. Timestamp ordering is FIFO per
            // producer.
            for (unsigned i = 0; i < 256 && blocked != 2; ++i) {
                const auto* p = output.peek();
                if (!p)
                    break;
                if (!backend->schedules() && p->timestampNs > midi2TimestampNow())
                    break;
                const auto ci = sysex.peek();
                if (ci && sysexOffset && p->packet.type() == 3 && p->packet.group() == ci->group)
                    break;
                status = backend->send(*p);
                if (status == Status::would_block) {
                    blocked = 1;
                    break;
                }
                if (status != Status::ok) {
                    error.store(static_cast<std::uint32_t>(status), std::memory_order_release);
                    break;
                }
                blocked = 0;
                if (p->packet.type() == 3) {
                    const auto form = (p->packet.words[0] >> 20) & 15;
                    rawSysEx[p->packet.group()] = form == 1 || form == 2;
                }
                (void)output.discard();
            }
            if (error.load(std::memory_order_acquire))
                break;
            for (unsigned i = 0; i < 32 && blocked != 1; ++i) {
                const auto p = sysex.peek();
                if (!p)
                    break;
                if (rawSysEx[p->group])
                    break;
                if (!backend->schedules() && p->timestamp > midi2TimestampNow())
                    break;
                const auto count =
                    std::min(std::size_t{6}, static_cast<std::size_t>(p->size) - sysexOffset);
                const auto form = p->size <= 6                     ? 0u
                                  : sysexOffset == 0               ? 1u
                                  : sysexOffset + count == p->size ? 3u
                                                                   : 2u;
                const auto packet = umpSysEx7(p->group, form, {p->bytes + sysexOffset, count});
                status = backend->send({packet, p->timestamp});
                if (status == Status::would_block) {
                    blocked = 2;
                    break;
                }
                if (status != Status::ok) {
                    error.store(static_cast<std::uint32_t>(status), std::memory_order_release);
                    break;
                }
                blocked = 0;
                sysexOffset += count;
                if (sysexOffset == p->size) {
                    (void)sysex.discard();
                    sysexOffset = 0;
                }
            }
            if (error.load(std::memory_order_acquire))
                break;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        // Native backend is destroyed here, on the opening worker (COM/JNI/CoreMIDI ownership).
    }
    Status start(detail::Midi2NativeRequest next, const Midi2DeviceOptions& config) noexcept {
        if (!config.incomingPackets || config.incomingPackets > 65536 || !config.outgoingPackets ||
            config.outgoingPackets > 65536 || !config.queuedSysEx || config.queuedSysEx > 128 ||
            !config.maximumSysExBytes || config.maximumSysExBytes > 65536)
            return Status::invalid_argument;
        auto status = input.prepare(config.incomingPackets);
        if (status != Status::ok)
            return status;
        status = output.prepare(config.outgoingPackets);
        if (status != Status::ok)
            return status;
        status = sysex.prepare(config.queuedSysEx, config.maximumSysExBytes);
        if (status != Status::ok)
            return status;
        options = config;
        request = std::move(next);
        try {
            worker = std::thread([this] { run(); });
            std::unique_lock lock(startupMutex);
            startup.wait(lock, [this] { return ready; });
            return opened;
        } catch (...) {
            return Status::device_unavailable;
        }
    }
};
Midi2Device::Midi2Device() = default;
Midi2Device::~Midi2Device() = default;
Midi2Device::Midi2Device(Midi2Device&&) noexcept = default;
Midi2Device& Midi2Device::operator=(Midi2Device&&) noexcept = default;
Status Midi2Device::open(const Midi2EndpointInfo& endpoint,
                         const Midi2DeviceOptions& options) noexcept {
    if (endpoint.backend == Midi2Backend::androidAmidi ||
        endpoint.backend == Midi2Backend::fileDescriptors ||
        endpoint.backend == Midi2Backend::automatic || (!endpoint.receive && !endpoint.send))
        return Status::invalid_argument;
    try {
        auto next = std::make_unique<State>();
        detail::Midi2NativeRequest request;
        request.endpoint = endpoint;
        auto result = next->start(std::move(request), options);
        if (result == Status::ok)
            state_.swap(next);
        return result;
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (...) {
        return Status::device_unavailable;
    }
}
Status Midi2Device::openLoopback(const Midi2DeviceOptions& options) noexcept {
    return open({"loopback", "CyDSP UMP loopback", Midi2Backend::loopback, true, true}, options);
}
Status Midi2Device::openFileDescriptors(int input, int output,
                                        const Midi2DeviceOptions& options) noexcept {
#if defined(_WIN32)
    (void)input;
    (void)output;
    (void)options;
    return Status::unsupported_backend;
#else
    if (input < 0 && output < 0)
        return Status::invalid_argument;
    try {
        auto next = std::make_unique<State>();
        detail::Midi2NativeRequest request;
        request.endpoint = {"fd", "Native UMP descriptors", Midi2Backend::fileDescriptors,
                            input >= 0, output >= 0};
        request.inputFd = input;
        request.outputFd = output;
        auto result = next->start(std::move(request), options);
        if (result == Status::ok)
            state_.swap(next);
        return result;
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (...) {
        return Status::device_unavailable;
    }
#endif
}
Status Midi2Device::openAndroid(void* env, void* device, int receive, int send,
                                const Midi2DeviceOptions& options) noexcept {
#if defined(__ANDROID__)
    try {
        auto next = std::make_unique<State>();
        detail::Midi2NativeRequest request;
        auto result = detail::prepareAndroidMidi2(env, device, receive, send, request);
        if (result != Status::ok)
            return result;
        result = next->start(std::move(request), options);
        if (result == Status::ok)
            state_.swap(next);
        return result;
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (...) {
        return Status::device_unavailable;
    }
#else
    (void)env;
    (void)device;
    (void)receive;
    (void)send;
    (void)options;
    return Status::unsupported_backend;
#endif
}
void Midi2Device::close() noexcept {
    state_.reset();
}
Status Midi2Device::send(const UmpPacket& packet, std::uint64_t time) noexcept {
    if (!state_)
        return Status::not_prepared;
    if (!packet.valid() ||
        time > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()))
        return Status::invalid_argument;
    if (!state_->request.endpoint.send)
        return Status::unsupported_format;
    const auto error = lastError();
    if (error != Status::ok)
        return error;
    return state_->output.push({packet, time}) ? Status::ok : Status::update_queue_full;
}
Status Midi2Device::sendSysEx7(unsigned group, std::span<const std::uint8_t> bytes,
                               std::uint64_t time) noexcept {
    if (!state_)
        return Status::not_prepared;
    if (group > 15 || bytes.size() > state_->options.maximumSysExBytes ||
        time > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) ||
        !std::all_of(bytes.begin(), bytes.end(), [](auto v) { return v < 128; }))
        return Status::invalid_argument;
    if (!state_->request.endpoint.send)
        return Status::unsupported_format;
    const auto error = lastError();
    if (error != Status::ok)
        return error;
    return state_->sysex.push(group, bytes, time) ? Status::ok : Status::update_queue_full;
}
Status Midi2Device::read(Midi2TimedPacket& packet) noexcept {
    if (!state_)
        return Status::not_prepared;
    if (state_->input.pop(packet))
        return Status::ok;
    const auto s = lastError();
    return s == Status::ok ? Status::would_block : s;
}
Status Midi2Device::lastError() const noexcept {
    return state_ ? static_cast<Status>(state_->error.load(std::memory_order_acquire))
                  : Status::not_prepared;
}
std::uint32_t Midi2Device::droppedPackets() const noexcept {
    return state_ ? state_->dropped.load(std::memory_order_relaxed) : 0;
}
Midi2Backend Midi2Device::backend() const noexcept {
    return state_ ? state_->request.endpoint.backend : Midi2Backend::automatic;
}
} // namespace cydsp
