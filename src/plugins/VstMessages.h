#pragma once
#include "VstSupport.h"
#include <map>
namespace cydsp::detail::vst {
class Attributes final : public IAttributeList {
    struct Value {
        int type = 0;
        int64 integer = 0;
        double real = 0;
        std::basic_string<char16> text;
        std::vector<std::uint8_t> bytes;
    };
    std::map<std::string, Value> values_;
    std::atomic<uint32> references_{1};
    static std::string key(AttrID id) {
        if (!id)
            throw std::invalid_argument("attribute id");
        std::size_t n = 0;
        while (n < 1024 && id[n])
            ++n;
        if (n == 1024)
            throw std::invalid_argument("attribute id");
        return {id, n};
    }
    tresult put(AttrID id, Value v) noexcept {
        try {
            const auto k = key(id);
            if (values_.size() >= 1024 && !values_.contains(k))
                return kOutOfMemory;
            values_.insert_or_assign(k, std::move(v));
            return kResultOk;
        } catch (...) {
            return kOutOfMemory;
        }
    }
    const Value* get(AttrID id, int type) const noexcept {
        try {
            const auto it = values_.find(key(id));
            return it != values_.end() && it->second.type == type ? &it->second : nullptr;
        } catch (...) {
            return nullptr;
        }
    }

  public:
    tresult PLUGIN_API queryInterface(const TUID id, void** out) override {
        if (!out)
            return kInvalidArgument;
        *out = nullptr;
        if (equal(id, IAttributeList_iid) || equal(id, FUnknown_iid)) {
            *out = static_cast<IAttributeList*>(this);
            addRef();
            return kResultOk;
        }
        return kNoInterface;
    }
    uint32 PLUGIN_API addRef() override {
        return ++references_;
    }
    uint32 PLUGIN_API release() override {
        const auto n = --references_;
        if (!n)
            delete this;
        return n;
    }
    tresult PLUGIN_API setInt(AttrID id, int64 value) override {
        Value v;
        v.type = 1;
        v.integer = value;
        return put(id, std::move(v));
    }
    tresult PLUGIN_API getInt(AttrID id, int64& value) override {
        const auto* v = get(id, 1);
        if (!v)
            return kResultFalse;
        value = v->integer;
        return kResultOk;
    }
    tresult PLUGIN_API setFloat(AttrID id, double value) override {
        Value v;
        v.type = 2;
        v.real = value;
        return put(id, std::move(v));
    }
    tresult PLUGIN_API getFloat(AttrID id, double& value) override {
        const auto* v = get(id, 2);
        if (!v)
            return kResultFalse;
        value = v->real;
        return kResultOk;
    }
    tresult PLUGIN_API setString(AttrID id, const char16* text) override {
        if (!text)
            return kInvalidArgument;
        try {
            std::size_t n = 0;
            while (n < 1048576 && text[n])
                ++n;
            if (n == 1048576)
                return kInvalidArgument;
            Value v;
            v.type = 3;
            v.text.assign(text, text + n);
            return put(id, std::move(v));
        } catch (...) {
            return kOutOfMemory;
        }
    }
    tresult PLUGIN_API getString(AttrID id, char16* text, uint32 bytes) override {
        if (!text || bytes < sizeof(char16))
            return kInvalidArgument;
        const auto* v = get(id, 3);
        if (!v)
            return kResultFalse;
        const auto capacity = bytes / sizeof(char16);
        const auto n = std::min(std::size_t(capacity - 1), v->text.size());
        std::copy_n(v->text.data(), n, text);
        text[n] = 0;
        return n == v->text.size() ? kResultOk : kResultFalse;
    }
    tresult PLUGIN_API setBinary(AttrID id, const void* data, uint32 bytes) override {
        if ((!data && bytes) || bytes > 16 * 1024 * 1024)
            return kInvalidArgument;
        try {
            Value v;
            v.type = 4;
            if (bytes) {
                const auto* p = static_cast<const std::uint8_t*>(data);
                v.bytes.assign(p, p + bytes);
            }
            return put(id, std::move(v));
        } catch (...) {
            return kOutOfMemory;
        }
    }
    tresult PLUGIN_API getBinary(AttrID id, const void*& data, uint32& bytes) override {
        const auto* v = get(id, 4);
        if (!v)
            return kResultFalse;
        data = v->bytes.data();
        bytes = uint32(v->bytes.size());
        return kResultOk;
    }
};
class Message final : public IMessage {
    std::atomic<uint32> references_{1};
    std::string id_;
    Attributes* attributes_ = new Attributes;

  public:
    ~Message() {
        attributes_->release();
    }
    tresult PLUGIN_API queryInterface(const TUID id, void** out) override {
        if (!out)
            return kInvalidArgument;
        *out = nullptr;
        if (equal(id, IMessage_iid) || equal(id, FUnknown_iid)) {
            *out = static_cast<IMessage*>(this);
            addRef();
            return kResultOk;
        }
        return kNoInterface;
    }
    uint32 PLUGIN_API addRef() override {
        return ++references_;
    }
    uint32 PLUGIN_API release() override {
        const auto n = --references_;
        if (!n)
            delete this;
        return n;
    }
    FIDString PLUGIN_API getMessageID() override {
        return id_.c_str();
    }
    void PLUGIN_API setMessageID(FIDString id) override {
        if (!id)
            return;
        try {
            std::size_t n = 0;
            while (n < 65536 && id[n])
                ++n;
            if (n < 65536)
                id_.assign(id, n);
        } catch (...) {
        }
    }
    IAttributeList* PLUGIN_API getAttributes() override {
        return attributes_;
    }
};
} // namespace cydsp::detail::vst
