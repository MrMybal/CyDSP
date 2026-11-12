#pragma once
#include "detail/CiJson.h"
#include <limits>
namespace cydsp::detail {
struct CiJsonTree {
    static constexpr std::size_t none = std::numeric_limits<std::size_t>::max();
    struct Node {
        std::span<const std::uint8_t> raw, key, replacement;
        std::size_t parent = none, child = none, next = none;
        unsigned kind = 0; // '{', '[', or primitive's initial byte.
        std::uint32_t hash = 0;
        bool replaced = false;
    };
    std::vector<Node> nodes;
    std::vector<char> first, second, path;
    std::size_t used = 0;
    Status error = Status::ok;
    Status prepare(std::size_t count, std::size_t pointer) noexcept {
        if (!count || count > 65536 || !pointer || pointer > 65536)
            return Status::invalid_argument;
        try {
            nodes.resize(count);
            first.resize(pointer);
            second.resize(pointer);
            path.resize(pointer);
            return Status::ok;
        } catch (...) {
            return Status::allocation_failed;
        }
    }
    void reset() noexcept {
        used = 0;
        error = Status::ok;
    }
    bool decode(std::span<const std::uint8_t> raw, std::span<char> output,
                std::size_t& n) noexcept {
        const auto r = CiPropertyValue{raw}.string(output, n);
        if (r != Status::ok) {
            error = r;
            return false;
        }
        return true;
    }
    bool equal(std::span<const std::uint8_t> a, std::span<const std::uint8_t> b) noexcept {
        std::size_t an = 0, bn = 0;
        return decode(a, first, an) && decode(b, second, bn) && an == bn &&
               std::equal(first.begin(), first.begin() + static_cast<std::ptrdiff_t>(an),
                          second.begin());
    }
    std::size_t value(CiJsonCursor& c, std::size_t parent, unsigned depth) noexcept {
        c.whitespace();
        if (depth > 32 || c.position == c.bytes.size() || used == nodes.size()) {
            error = used == nodes.size() ? Status::block_too_large : Status::invalid_argument;
            return none;
        }
        const auto index = used++, start = c.position;
        nodes[index] = {};
        nodes[index].parent = parent;
        nodes[index].kind = c.bytes[c.position];
        const auto kind = nodes[index].kind;
        if (kind == '{' || kind == '[') {
            ++c.position;
            c.whitespace();
            const unsigned close = kind == '{' ? '}' : ']';
            std::size_t tail = none;
            if (!c.take(close))
                do {
                    std::span<const std::uint8_t> key;
                    std::uint32_t hash = 2166136261u;
                    if (kind == '{') {
                        c.whitespace();
                        const auto keyStart = c.position;
                        std::size_t length;
                        if (!c.string({}, length))
                            return none;
                        key = c.bytes.subspan(keyStart, c.position - keyStart);
                        if (!decode(key, first, length))
                            return none;
                        for (std::size_t j = 0; j < length; ++j)
                            hash = (hash ^ static_cast<unsigned char>(first[j])) * 16777619u;
                        for (auto sibling = nodes[index].child; sibling != none;
                             sibling = nodes[sibling].next)
                            if (nodes[sibling].hash == hash && equal(key, nodes[sibling].key)) {
                                error = Status::invalid_argument;
                                return none;
                            }
                        if (error != Status::ok)
                            return none;
                        c.whitespace();
                        if (!c.take(':'))
                            return none;
                    }
                    const auto child = value(c, index, depth + 1);
                    if (child == none)
                        return none;
                    nodes[child].key = key;
                    nodes[child].hash = hash;
                    if (tail == none)
                        nodes[index].child = child;
                    else
                        nodes[tail].next = child;
                    tail = child;
                    c.whitespace();
                    if (c.take(close))
                        break;
                    if (!c.take(','))
                        return none;
                } while (true);
        } else if (!c.primitive())
            return none;
        nodes[index].raw = c.bytes.subspan(start, c.position - start);
        return index;
    }
    Status parse(std::span<const std::uint8_t> bytes, std::size_t& root) noexcept {
        CiJsonCursor c{bytes};
        root = value(c, none, 0);
        c.whitespace();
        if (root == none || c.position != bytes.size())
            return error == Status::ok ? Status::invalid_argument : error;
        return Status::ok;
    }
    bool render(std::size_t index, CiJsonWriter& w, unsigned depth = 0) const noexcept {
        if (depth > 32)
            return false;
        const auto& n = nodes[index];
        if (n.replaced) {
            w.text({reinterpret_cast<const char*>(n.replacement.data()), n.replacement.size()});
            return true;
        }
        if (n.kind != '{' && n.kind != '[') {
            w.text({reinterpret_cast<const char*>(n.raw.data()), n.raw.size()});
            return true;
        }
        w.character(n.kind);
        bool comma = false;
        for (auto child = n.child; child != none; child = nodes[child].next) {
            if (comma)
                w.character(',');
            comma = true;
            if (n.kind == '{') {
                const auto k = nodes[child].key;
                w.text({reinterpret_cast<const char*>(k.data()), k.size()});
                w.character(':');
            }
            if (!render(child, w, depth + 1))
                return false;
        }
        w.character(n.kind == '{' ? '}' : ']');
        return true;
    }
    std::size_t pointer(std::size_t root, std::span<const std::uint8_t> raw) noexcept {
        std::size_t length = 0;
        if (!decode(raw, path, length))
            return none;
        if (!length)
            return root;
        if (path[0] != '/')
            return none;
        auto current = root;
        for (std::size_t p = 1; p <= length;) {
            const auto start = p;
            while (p < length && path[p] != '/')
                ++p;
            std::size_t count = 0;
            for (std::size_t i = start; i < p; ++i) {
                char c = path[i];
                if (c == '~') {
                    if (++i == p || (path[i] != '0' && path[i] != '1'))
                        return none;
                    c = path[i] == '0' ? '~' : '/';
                }
                second[count++] = c;
            }
            const auto& node = nodes[current];
            std::size_t next = none;
            if (node.kind == '{') {
                for (auto child = node.child; child != none; child = nodes[child].next) {
                    std::size_t keySize = 0;
                    if (!decode(nodes[child].key, first, keySize))
                        return none;
                    if (keySize == count &&
                        std::equal(first.begin(),
                                   first.begin() + static_cast<std::ptrdiff_t>(count),
                                   second.begin())) {
                        next = child;
                        break;
                    }
                }
            } else if (node.kind == '[') {
                std::uint32_t index = 0;
                const auto parsed = std::from_chars(second.data(), second.data() + count, index);
                if (!count || (count > 1 && second[0] == '0') || parsed.ec != std::errc{} ||
                    parsed.ptr != second.data() + count)
                    return none;
                next = node.child;
                while (index && next != none) {
                    --index;
                    next = nodes[next].next;
                }
            }
            if (next == none)
                return none;
            current = next;
            if (p == length)
                break;
            ++p;
        }
        return current;
    }
};
} // namespace cydsp::detail
