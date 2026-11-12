#include "cydsp/midi/CiProperty.h"
#include "detail/Alias.h"
#include "detail/CiJson.h"
namespace cydsp {
Status CiPropertyValue::string(std::span<char> out, std::size_t& n) const noexcept {
    n = 0;
    detail::CiJsonCursor first{json};
    std::size_t required;
    if (!first.string({}, required) || first.position != json.size())
        return Status::invalid_argument;
    n = required;
    if (out.size() < required)
        return Status::block_too_large;
    if (detail::overlaps(json.data(), json.size(), out.data(), required))
        return Status::partial_overlap;
    detail::CiJsonCursor second{json};
    return second.string(out, n) ? Status::ok : Status::invalid_argument;
}
Status CiPropertyValue::unsignedInteger(std::uint32_t& result) const noexcept {
    if (json.empty())
        return Status::invalid_argument;
    std::uint32_t number = 0;
    const auto* first = reinterpret_cast<const char*>(json.data());
    const auto parsed = std::from_chars(first, first + json.size(), number);
    if (parsed.ec != std::errc{} || parsed.ptr != first + json.size() ||
        (json.size() > 1 && json[0] == '0'))
        return Status::invalid_argument;
    result = number;
    return Status::ok;
}
Status CiPropertyValue::boolean(bool& result) const noexcept {
    const auto s = std::string_view(reinterpret_cast<const char*>(json.data()), json.size());
    if (s != "true" && s != "false")
        return Status::invalid_argument;
    result = s == "true";
    return Status::ok;
}
Status CiPropertyHeader::parse(std::span<const std::uint8_t> bytes) noexcept {
    CiPropertyHeader next;
    detail::CiJsonCursor c{bytes};
    if (bytes.size() > 4096 || !c.take('{'))
        return Status::invalid_argument;
    if (!c.take('}'))
        do {
            if (next.count_ == next.fields_.size())
                return Status::block_too_large;
            auto& f = next.fields_[next.count_];
            if (!c.string(f.key, f.size) || !f.size || f.size > f.key.size() || f.key[0] < 'a' ||
                f.key[0] > 'z' || !c.take(':'))
                return Status::invalid_argument;
            for (std::size_t i = 0; i < f.size; ++i)
                if (!((f.key[i] >= 'a' && f.key[i] <= 'z') ||
                      (f.key[i] >= 'A' && f.key[i] <= 'Z') || (f.key[i] >= '0' && f.key[i] <= '9')))
                    return Status::invalid_argument;
            const auto start = c.position;
            if (!c.primitive(false))
                return Status::invalid_argument;
            f.value.json = bytes.subspan(start, c.position - start);
            const auto key = std::string_view(f.key.data(), f.size);
            if (next.find(key))
                return Status::invalid_argument;
            ++next.count_;
            if (c.take('}'))
                break;
            if (!c.take(','))
                return Status::invalid_argument;
        } while (true);
    if (c.position != bytes.size())
        return Status::invalid_argument;
    *this = next;
    return Status::ok;
}
std::optional<CiPropertyValue> CiPropertyHeader::find(std::string_view key) const noexcept {
    for (std::size_t i = 0; i < count_; ++i)
        if (std::string_view(fields_[i].key.data(), fields_[i].size) == key)
            return fields_[i].value;
    return {};
}
std::string_view CiPropertyHeader::firstKey() const noexcept {
    return count_ ? std::string_view(fields_[0].key.data(), fields_[0].size) : std::string_view{};
}
Status validateCiPropertyJson(std::span<const std::uint8_t> bytes) noexcept {
    detail::CiJsonCursor c{bytes};
    if (!c.value())
        return Status::invalid_argument;
    c.whitespace();
    return c.position == bytes.size() ? Status::ok : Status::invalid_argument;
}
Status ciPropertyJsonString(std::string_view utf8, std::span<std::uint8_t> out,
                            std::size_t& n) noexcept {
    n = 0;
    detail::CiJsonWriter sizing{{}};
    if (!sizing.string(utf8))
        return Status::invalid_argument;
    n = sizing.position;
    if (out.size() < n)
        return Status::block_too_large;
    if (detail::overlaps(utf8.data(), utf8.size(), out.data(), n))
        return Status::partial_overlap;
    detail::CiJsonWriter writer{out};
    return writer.string(utf8) && writer.good ? Status::ok : Status::invalid_argument;
}
} // namespace cydsp
