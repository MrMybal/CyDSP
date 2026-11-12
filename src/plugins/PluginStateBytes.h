#pragma once
#include "PluginInternal.h"
#include <bit>
#include <cstring>
namespace cydsp::detail {
inline std::filesystem::path pathFromUtf8(std::string_view text) {
    return std::filesystem::path(
        std::u8string_view(reinterpret_cast<const char8_t*>(text.data()), text.size()));
}
inline std::string pathUtf8(const std::filesystem::path& path) {
    auto text = path.generic_u8string();
    return {reinterpret_cast<const char*>(text.data()), text.size()};
}
inline void store32(std::vector<std::uint8_t>& b, std::uint32_t v) {
    for (unsigned i = 0; i < 4; ++i)
        b.push_back(std::uint8_t(v >> (8 * i)));
}
inline void store64(std::vector<std::uint8_t>& b, std::uint64_t v) {
    for (unsigned i = 0; i < 8; ++i)
        b.push_back(std::uint8_t(v >> (8 * i)));
}
inline std::uint32_t load32(std::span<const std::uint8_t> b, std::size_t p) noexcept {
    std::uint32_t v = 0;
    for (unsigned i = 0; i < 4; ++i)
        v |= std::uint32_t(b[p + i]) << (8 * i);
    return v;
}
inline std::uint64_t load64(std::span<const std::uint8_t> b, std::size_t p) noexcept {
    std::uint64_t v = 0;
    for (unsigned i = 0; i < 8; ++i)
        v |= std::uint64_t(b[p + i]) << (8 * i);
    return v;
}
inline std::span<const std::uint8_t> pluginMidi(const PluginEvent& e,
                                                std::array<std::uint8_t, 3>& b) noexcept {
    if (e.kind == PluginEventKind::midi1 || e.kind == PluginEventKind::sysex)
        return e.bytes;
    if ((e.kind != PluginEventKind::noteOn && e.kind != PluginEventKind::noteOff) ||
        e.channel < 0 || e.key < 0)
        return {};
    b = {std::uint8_t((e.kind == PluginEventKind::noteOn ? 0x90 : 0x80) | e.channel),
         std::uint8_t(e.key), std::uint8_t(std::lround(e.value * 127.))};
    return b;
}
} // namespace cydsp::detail
