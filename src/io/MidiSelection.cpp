#include "cydsp/io/MidiDevice.h"
#include <algorithm>
namespace cydsp {
Status resolveMidiPort(const MidiDevicePorts& ports, const MidiPortSelector& s,
                       std::uint32_t& result) noexcept {
    if (!s.backend.empty() && s.backend != ports.backend)
        return Status::unsupported_backend;
    const auto& list = s.input ? ports.inputs : ports.outputs;
    const auto find = [&](bool stable) {
        std::size_t count = 0;
        std::uint32_t index = 0;
        for (const auto& p : list) {
            if (stable ? p.persistentIdentity && p.persistentId == s.persistentId
                       : p.name == s.name) {
                ++count;
                index = p.index;
            }
        }
        if (count == 1) {
            result = index;
            return Status::ok;
        }
        return count > 1 ? Status::ambiguous : Status::device_unavailable;
    };
    if (!s.persistentId.empty()) {
        const auto r = find(true);
        if (r != Status::device_unavailable || !s.allowNameFallback)
            return r;
    }
    if (s.name.empty())
        return Status::invalid_argument;
    return find(false);
}
Status serializeMidiPortSelector(const MidiPortSelector& s, std::string& output) noexcept {
    if (s.backend.size() > 256 || s.persistentId.size() > 65536 || s.name.size() > 65536)
        return Status::invalid_argument;
    try {
        std::string out = s.input ? "CyDSP.MidiPort/1/i/" : "CyDSP.MidiPort/1/o/";
        constexpr char digits[] = "0123456789abcdef";
        for (const auto* field : {&s.backend, &s.persistentId, &s.name}) {
            for (unsigned char c : *field) {
                out += digits[c >> 4];
                out += digits[c & 15];
            }
            out += '/';
        }
        out += s.allowNameFallback ? '1' : '0';
        output.swap(out);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
Status parseMidiPortSelector(std::string_view text, MidiPortSelector& output) noexcept {
    if (!text.starts_with("CyDSP.MidiPort/1/") || text.size() > 263000)
        return Status::unsupported_format;
    text.remove_prefix(std::string_view("CyDSP.MidiPort/1/").size());
    if (text.size() < 3 || (text[0] != 'i' && text[0] != 'o') || text[1] != '/')
        return Status::invalid_argument;
    try {
        MidiPortSelector next;
        next.input = text[0] == 'i';
        text.remove_prefix(2);
        const auto digit = [](char c) {
            return c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : -1;
        };
        for (auto* field : {&next.backend, &next.persistentId, &next.name}) {
            const auto end = text.find('/');
            if (end == std::string_view::npos || end % 2)
                return Status::invalid_argument;
            for (std::size_t i = 0; i < end; i += 2) {
                const auto a = digit(text[i]), b = digit(text[i + 1]);
                if (a < 0 || b < 0)
                    return Status::invalid_argument;
                field->push_back(char((a << 4) | b));
            }
            text.remove_prefix(end + 1);
        }
        if (next.backend.size() > 256 || next.persistentId.size() > 65536 ||
            next.name.size() > 65536 || (text != "0" && text != "1"))
            return Status::invalid_argument;
        next.allowNameFallback = text == "1";
        output = std::move(next);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
} // namespace cydsp
