#pragma once
// Original support implementation; pinned MIT declarations:
// third_party/licenses/vst3-interfaces.txt
#include "PluginInternal.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/base/ipluginbase.h"
#include "pluginterfaces/gui/iplugview.h"
#include "pluginterfaces/gui/iplugviewcontentscalesupport.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/vst/ivsthostapplication.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"
#include <algorithm>
#include <atomic>
#include <cstring>
namespace cydsp::detail::vst {
using namespace Steinberg;
using namespace Steinberg::Vst;
inline bool equal(const TUID a, const TUID b) noexcept {
    return std::memcmp(a, b, 16) == 0;
}
template <class T> T* query(FUnknown* object, const TUID id) noexcept {
    T* out = nullptr;
    if (object && object->queryInterface(id, reinterpret_cast<void**>(&out)) == kResultOk)
        return out;
    return nullptr;
}
template <class T> void release(T*& p) noexcept {
    if (p)
        p->release();
    p = nullptr;
}
// Borrowed interface storage belongs to the containing stream/process service;
// the owner retains one reference until all foreign calls/retained references end.
#define CYDSP_VST_BORROWED(Type)                                                                   \
    tresult PLUGIN_API queryInterface(const TUID requestedId, void** out) override {               \
        if (!out)                                                                                  \
            return kInvalidArgument;                                                               \
        *out = nullptr;                                                                            \
        if (equal(requestedId, Type##_iid) || equal(requestedId, FUnknown_iid)) {                  \
            *out = static_cast<Type*>(this);                                                       \
            addRef();                                                                              \
            return kResultOk;                                                                      \
        }                                                                                          \
        return kNoInterface;                                                                       \
    }                                                                                              \
    std::atomic<uint32> references{1};                                                             \
    uint32 PLUGIN_API addRef() override {                                                          \
        return references.fetch_add(1, std::memory_order_relaxed) + 1;                             \
    }                                                                                              \
    uint32 PLUGIN_API release() override {                                                         \
        return references.fetch_sub(1, std::memory_order_acq_rel) - 1;                             \
    }
inline std::string utf8(const char16* text, std::size_t maximum = 128) {
    std::string out;
    for (std::size_t i = 0; i < maximum && text[i]; ++i) {
        auto cp = std::uint32_t(text[i]);
        if (cp >= 0xd800 && cp <= 0xdbff && i + 1 < maximum && text[i + 1] >= 0xdc00 &&
            text[i + 1] <= 0xdfff)
            cp = 0x10000 + ((cp - 0xd800) << 10) + (std::uint32_t(text[++i]) - 0xdc00);
        else if (cp >= 0xd800 && cp <= 0xdfff)
            cp = 0xfffd;
        if (cp < 0x80)
            out += char(cp);
        else if (cp < 0x800) {
            out += char(0xc0 | (cp >> 6));
            out += char(0x80 | (cp & 63));
        } else if (cp < 0x10000) {
            out += char(0xe0 | (cp >> 12));
            out += char(0x80 | ((cp >> 6) & 63));
            out += char(0x80 | (cp & 63));
        } else {
            out += char(0xf0 | (cp >> 18));
            out += char(0x80 | ((cp >> 12) & 63));
            out += char(0x80 | ((cp >> 6) & 63));
            out += char(0x80 | (cp & 63));
        }
    }
    return out;
}
template <std::size_t N> std::string string(const char (&s)[N]) {
    return {s, std::find(s, s + N, char{})};
}
inline unsigned cidIndex(unsigned i) noexcept {
#if COM_COMPATIBLE
    constexpr unsigned order[]{3, 2, 1, 0, 5, 4, 7, 6, 8, 9, 10, 11, 12, 13, 14, 15};
    return order[i];
#else
    return i;
#endif
}
inline std::string identifier(const TUID id) {
    std::string out = "vst3:";
    constexpr char digits[] = "0123456789abcdef";
    for (unsigned i = 0; i < 16; ++i) {
        const auto c = std::uint8_t(id[cidIndex(i)]);
        out += digits[c >> 4];
        out += digits[c & 15];
    }
    return out;
}
inline bool identifier(std::string_view text, TUID id) noexcept {
    if (!text.starts_with("vst3:") || text.size() != 37)
        return false;
    const auto digit = [](char c) {
        return c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : -1;
    };
    for (unsigned i = 0; i < 16; ++i) {
        const auto a = digit(text[5 + i * 2]), b = digit(text[6 + i * 2]);
        if (a < 0 || b < 0)
            return false;
        id[cidIndex(i)] = char((a << 4) | b);
    }
    return true;
}
class MemoryStream final : public IBStream {
  public:
    CYDSP_VST_BORROWED(IBStream)
    std::vector<std::uint8_t> bytes;
    std::span<const std::uint8_t> input;
    std::size_t position = 0, limit = 64 * 1024 * 1024;
    bool readonly = false, failed = false;
    MemoryStream() = default;
    explicit MemoryStream(std::span<const std::uint8_t> data) : input(data), readonly(true) {}
    tresult PLUGIN_API read(void* buffer, int32 requested, int32* actual) override {
        if (actual)
            *actual = 0;
        if (requested < 0 || (!buffer && requested))
            return kInvalidArgument;
        const auto source = readonly ? input : std::span<const std::uint8_t>(bytes);
        const auto n = position < source.size()
                           ? std::min(std::size_t(requested), source.size() - position)
                           : 0;
        if (n)
            std::memcpy(buffer, source.data() + position, n);
        position += n;
        if (actual)
            *actual = int32(n);
        return n == std::size_t(requested) ? kResultOk : kResultFalse;
    }
    tresult PLUGIN_API write(void* buffer, int32 requested, int32* actual) override {
        if (actual)
            *actual = 0;
        if (readonly || requested < 0 || (!buffer && requested) || position > limit ||
            std::size_t(requested) > limit - position) {
            failed = true;
            return kInvalidArgument;
        }
        try {
            const auto end = position + std::size_t(requested);
            if (end > bytes.size())
                bytes.resize(end);
            if (requested)
                std::memcpy(bytes.data() + position, buffer, std::size_t(requested));
            position = end;
            if (actual)
                *actual = requested;
            return kResultOk;
        } catch (...) {
            failed = true;
            return kOutOfMemory;
        }
    }
    tresult PLUGIN_API seek(int64 displacement, int32 mode, int64* actual) override {
        if (actual)
            *actual = int64(position);
        if (mode < kIBSeekSet || mode > kIBSeekEnd)
            return kInvalidArgument;
        const auto base = mode == kIBSeekSet   ? std::size_t(0)
                          : mode == kIBSeekCur ? position
                                               : (readonly ? input.size() : bytes.size());
        if (displacement < 0 && std::uint64_t(-(displacement + 1)) + 1 > base)
            return kInvalidArgument;
        if (displacement > 0 && std::uint64_t(displacement) > limit - base)
            return kInvalidArgument;
        const auto next = displacement < 0 ? base - std::size_t(-(displacement + 1)) - 1
                                           : base + std::size_t(displacement);
        if (next > limit || (readonly && next > input.size()))
            return kInvalidArgument;
        position = next;
        if (actual)
            *actual = int64(position);
        return kResultOk;
    }
    tresult PLUGIN_API tell(int64* actual) override {
        if (!actual)
            return kInvalidArgument;
        *actual = int64(position);
        return kResultOk;
    }
};
class ParameterChanges final : public IParameterChanges {
  public:
    struct Point {
        std::uint32_t id = 0, frame = 0, next = 0, sequence = 0;
        double value = 0;
    };
    class Queue final : public IParamValueQueue {
      public:
        CYDSP_VST_BORROWED(IParamValueQueue)
        ParameterChanges* owner = nullptr;
        ParamID id = 0;
        std::uint32_t head = 0, tail = 0, count = 0, lastIndex = 0, lastNode = 0;
        ParamID PLUGIN_API getParameterId() override {
            return id;
        }
        int32 PLUGIN_API getPointCount() override {
            return int32(count);
        }
        tresult PLUGIN_API getPoint(int32 index, int32& frame, ParamValue& value) override {
            if (index < 0 || std::uint32_t(index) >= count)
                return kInvalidArgument;
            auto node = std::uint32_t(index) >= lastIndex ? lastNode : head;
            auto at = std::uint32_t(index) >= lastIndex ? lastIndex : 0u;
            while (at++ < std::uint32_t(index))
                node = owner->points[node].next;
            lastIndex = std::uint32_t(index);
            lastNode = node;
            frame = int32(owner->points[node].frame);
            value = owner->points[node].value;
            return kResultOk;
        }
        tresult PLUGIN_API addPoint(int32 frame, ParamValue value, int32& index) override {
            if (frame < 0 || std::uint32_t(frame) >= owner->frames || !isFinite(value) ||
                value < 0 || value > 1 || owner->used >= owner->points.size() ||
                (count && std::uint32_t(frame) < owner->points[tail].frame))
                return kInvalidArgument;
            const auto node = owner->used++;
            owner->points[node] = {id, std::uint32_t(frame), node, node, value};
            if (count)
                owner->points[tail].next = node;
            else
                head = lastNode = node;
            tail = node;
            index = int32(count++);
            return kResultOk;
        }
    };
    CYDSP_VST_BORROWED(IParameterChanges)
    std::vector<std::unique_ptr<Queue>> queues;
    std::vector<Queue*> active;
    std::vector<Point> points;
    std::uint32_t count = 0, used = 0, frames = 1;
    void prepare(std::span<const PluginParameter> params, std::size_t events) {
        // Build a replacement before destroying queues referenced by the old
        // active list. reset() must never traverse those pointers after clear().
        std::vector<std::unique_ptr<Queue>> nextQueues;
        std::vector<Queue*> nextActive(params.size());
        std::vector<Point> nextPoints(events);
        nextQueues.reserve(params.size());
        for (const auto& p : params) {
            auto q = std::make_unique<Queue>();
            q->owner = this;
            q->id = p.id;
            nextQueues.push_back(std::move(q));
        }
        queues.swap(nextQueues);
        active.swap(nextActive);
        points.swap(nextPoints);
        count = used = 0;
        frames = 1;
    }
    void reset(std::uint32_t n) noexcept {
        for (std::uint32_t i = 0; i < count; ++i)
            active[i]->count = active[i]->lastIndex = 0;
        count = used = 0;
        frames = n;
    }
    int32 PLUGIN_API getParameterCount() override {
        return int32(count);
    }
    IParamValueQueue* PLUGIN_API getParameterData(int32 i) override {
        return i >= 0 && std::uint32_t(i) < count ? active[std::size_t(i)] : nullptr;
    }
    IParamValueQueue* PLUGIN_API addParameterData(const ParamID& id, int32& index) override {
        const auto it = std::lower_bound(queues.begin(), queues.end(), id,
                                         [](const auto& a, auto b) { return a->id < b; });
        if (it == queues.end() || (*it)->id != id)
            return nullptr;
        auto* q = it->get();
        for (std::uint32_t i = 0; i < count; ++i)
            if (active[i] == q) {
                index = int32(i);
                return q;
            }
        if (count == active.size())
            return nullptr;
        index = int32(count);
        active[count++] = q;
        return q;
    }
    Status add(std::uint32_t id, std::uint32_t frame, double value) noexcept {
        int32 index = 0;
        auto* queue = addParameterData(id, index);
        return queue && queue->addPoint(int32(frame), value, index) == kResultOk
                   ? Status::ok
                   : Status::invalid_argument;
    }
    void sortOutput() noexcept {
        std::sort(points.begin(), points.begin() + used, [](const auto& a, const auto& b) {
            return a.frame < b.frame || (a.frame == b.frame && a.sequence < b.sequence);
        });
    }
};
class EventList final : public IEventList {
  public:
    CYDSP_VST_BORROWED(IEventList)
    std::vector<Event> entries;
    PluginEventBuffer copied;
    std::uint32_t count = 0, frames = 1;
    bool output = false;
    void prepare(std::size_t n) {
        entries.resize(n);
        const auto r = copied.prepare(n, 1048576);
        if (r != Status::ok)
            throw std::bad_alloc();
    }
    void reset(std::uint32_t n) noexcept {
        count = 0;
        frames = n;
        copied.reset();
    }
    int32 PLUGIN_API getEventCount() override {
        return int32(count);
    }
    tresult PLUGIN_API getEvent(int32 i, Event& e) override {
        if (i < 0 || std::uint32_t(i) >= count)
            return kInvalidArgument;
        e = entries[std::size_t(i)];
        return kResultOk;
    }
    tresult PLUGIN_API addEvent(Event& e) override {
        if (count == entries.size() || e.sampleOffset < 0 ||
            std::uint32_t(e.sampleOffset) >= frames || e.busIndex < 0 || e.busIndex > 32767)
            return kInvalidArgument;
        Event copy = e;
        if (e.type == Event::kDataEvent) {
            if (e.data.type != DataEvent::kMidiSysEx || !e.data.bytes || e.data.size > 1048576)
                return kInvalidArgument;
            PluginEvent p;
            p.kind = PluginEventKind::sysex;
            p.port = std::int16_t(e.busIndex);
            p.frame = std::uint32_t(e.sampleOffset);
            p.bytes = {e.data.bytes, e.data.size};
            if (copied.add(p) != Status::ok)
                return kOutOfMemory;
            copy.data.bytes = copied.event(copied.size() - 1)->bytes.data();
        }
        entries[count++] = copy;
        return kResultOk;
    }
};
} // namespace cydsp::detail::vst
