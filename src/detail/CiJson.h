#pragma once
#include "cydsp/midi/CiProperty.h"
#include <algorithm>
#include <charconv>
#include <cstring>
namespace cydsp::detail {
inline std::span<const std::uint8_t> ciBytes(std::string_view text) noexcept {
    return {reinterpret_cast<const std::uint8_t*>(text.data()), text.size()};
}
struct CiJsonCursor {
    std::span<const std::uint8_t> bytes;
    std::size_t position = 0;
    bool take(unsigned c) noexcept {
        if (position == bytes.size() || bytes[position] != c)
            return false;
        ++position;
        return true;
    }
    void whitespace() noexcept {
        while (position < bytes.size() && (bytes[position] == ' ' || bytes[position] == '\n' ||
                                           bytes[position] == '\r' || bytes[position] == '\t'))
            ++position;
    }
    bool hex(std::uint32_t& c) noexcept {
        c = 0;
        for (unsigned i = 0; i < 4; ++i) {
            if (position == bytes.size())
                return false;
            const auto v = bytes[position++];
            const unsigned n = v >= '0' && v <= '9'   ? v - '0'
                               : v >= 'a' && v <= 'f' ? v - 'a' + 10
                               : v >= 'A' && v <= 'F' ? v - 'A' + 10
                                                      : 16;
            if (n == 16)
                return false;
            c = c * 16 + n;
        }
        return true;
    }
    bool string(std::span<char> out, std::size_t& n) noexcept {
        n = 0;
        if (!take('"'))
            return false;
        auto emit = [&](unsigned c) {
            if (n < out.size())
                out[n] = static_cast<char>(c);
            ++n;
        };
        while (position < bytes.size()) {
            unsigned c = bytes[position++];
            if (c == '"')
                return true;
            if (c < 32 || c > 127)
                return false;
            if (c == '\\') {
                if (position == bytes.size())
                    return false;
                c = bytes[position++];
                switch (c) {
                case '"':
                case '\\':
                case '/':
                    break;
                case 'b':
                    c = 8;
                    break;
                case 'f':
                    c = 12;
                    break;
                case 'n':
                    c = 10;
                    break;
                case 'r':
                    c = 13;
                    break;
                case 't':
                    c = 9;
                    break;
                case 'u': {
                    std::uint32_t scalar;
                    if (!hex(scalar))
                        return false;
                    if (scalar >= 0xd800 && scalar <= 0xdbff) {
                        std::uint32_t low;
                        if (!take('\\') || !take('u') || !hex(low) || low < 0xdc00 || low > 0xdfff)
                            return false;
                        scalar = 0x10000 + (scalar - 0xd800) * 1024 + low - 0xdc00;
                    } else if (scalar >= 0xdc00 && scalar <= 0xdfff)
                        return false;
                    if (scalar < 128)
                        emit(scalar);
                    else if (scalar < 2048) {
                        emit(0xc0 | (scalar >> 6));
                        emit(0x80 | (scalar & 63));
                    } else if (scalar < 65536) {
                        emit(0xe0 | (scalar >> 12));
                        emit(0x80 | ((scalar >> 6) & 63));
                        emit(0x80 | (scalar & 63));
                    } else {
                        emit(0xf0 | (scalar >> 18));
                        emit(0x80 | ((scalar >> 12) & 63));
                        emit(0x80 | ((scalar >> 6) & 63));
                        emit(0x80 | (scalar & 63));
                    }
                    continue;
                }
                default:
                    return false;
                }
            }
            emit(c);
        }
        return false;
    }
    bool number() noexcept {
        const auto start = position;
        (void)take('-');
        if (!take('0')) {
            if (position == bytes.size() || bytes[position] < '1' || bytes[position] > '9')
                return false;
            while (position < bytes.size() && bytes[position] >= '0' && bytes[position] <= '9')
                ++position;
        }
        if (take('.')) {
            const auto p = position;
            while (position < bytes.size() && bytes[position] >= '0' && bytes[position] <= '9')
                ++position;
            if (p == position)
                return false;
        }
        if (take('e') || take('E')) {
            if (!take('+'))
                (void)take('-');
            const auto p = position;
            while (position < bytes.size() && bytes[position] >= '0' && bytes[position] <= '9')
                ++position;
            if (p == position)
                return false;
        }
        return position != start;
    }
    bool literal(std::string_view text) noexcept {
        if (bytes.size() - position < text.size() ||
            !std::equal(text.begin(), text.end(),
                        bytes.begin() + static_cast<std::ptrdiff_t>(position)))
            return false;
        position += text.size();
        return true;
    }
    bool primitive(bool allowNull = true) noexcept {
        if (position == bytes.size())
            return false;
        if (bytes[position] == '"') {
            std::size_t n;
            return string({}, n);
        }
        return literal("true") || literal("false") || (allowNull && literal("null")) || number();
    }
    bool value(unsigned depth = 0) noexcept {
        whitespace();
        if (depth > 32 || position == bytes.size())
            return false;
        if (take('{')) {
            whitespace();
            if (take('}'))
                return true;
            do {
                std::size_t n;
                whitespace();
                if (!string({}, n))
                    return false;
                whitespace();
                if (!take(':') || !value(depth + 1))
                    return false;
                whitespace();
                if (take('}'))
                    return true;
            } while (take(','));
            return false;
        }
        if (take('[')) {
            whitespace();
            if (take(']'))
                return true;
            do {
                if (!value(depth + 1))
                    return false;
                whitespace();
                if (take(']'))
                    return true;
            } while (take(','));
            return false;
        }
        return primitive();
    }
};
inline bool ciUtf8(std::string_view input, std::size_t& position, std::uint32_t& c) noexcept {
    const auto first = static_cast<unsigned char>(input[position++]);
    if (first < 128) {
        c = first;
        return true;
    }
    const unsigned count = first >= 0xc2 && first <= 0xdf   ? 1
                           : first >= 0xe0 && first <= 0xef ? 2
                           : first >= 0xf0 && first <= 0xf4 ? 3
                                                            : 0;
    if (!count || count > input.size() - position)
        return false;
    c = first & ((1u << (6 - count)) - 1);
    for (unsigned i = 0; i < count; ++i) {
        const auto part = static_cast<unsigned char>(input[position++]);
        if ((part & 0xc0) != 0x80)
            return false;
        c = (c << 6) | (part & 63);
    }
    return c >= (count == 1   ? 128u
                 : count == 2 ? 2048u
                              : 65536u) &&
           c <= 0x10ffff && !(c >= 0xd800 && c <= 0xdfff);
}
struct CiJsonWriter {
    std::span<std::uint8_t> output;
    std::size_t position = 0;
    bool good = true;
    void character(unsigned c) noexcept {
        if (position < output.size())
            output[position] = static_cast<std::uint8_t>(c);
        else
            good = false;
        ++position;
    }
    void text(std::string_view s) noexcept {
        for (auto c : s)
            character(static_cast<unsigned char>(c));
    }
    void number(std::uint32_t value) noexcept {
        std::array<char, 16> buffer{};
        const auto r = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
        text({buffer.data(), static_cast<std::size_t>(r.ptr - buffer.data())});
    }
    void unicode(std::uint32_t v) noexcept {
        text("\\u");
        for (int shift = 12; shift >= 0; shift -= 4)
            character("0123456789abcdef"[(v >> shift) & 15]);
    }
    bool string(std::string_view utf8) noexcept {
        character('"');
        for (std::size_t p = 0; p < utf8.size();) {
            std::uint32_t c;
            if (!ciUtf8(utf8, p, c))
                return false;
            if (c == '"' || c == '\\') {
                character('\\');
                character(c);
            } else if (c < 32 || c >= 127) {
                if (c <= 0xffff)
                    unicode(c);
                else {
                    c -= 0x10000;
                    unicode(0xd800 | (c >> 10));
                    unicode(0xdc00 | (c & 1023));
                }
            } else
                character(c);
        }
        character('"');
        return true;
    }
};
} // namespace cydsp::detail
