#pragma once
#include "cydsp/core/Cpu.h"
#include <cstdint>
namespace cydsp::detail {
struct X86Snapshot {
    Architecture architecture = Architecture::x64;
    std::uint32_t maxLeaf = 0, leaf1ecx = 0, leaf1edx = 0, leaf7ebx = 0;
    std::uint64_t xcr0 = 0;
};
constexpr CpuFeatures decodeX86(const X86Snapshot& raw) noexcept {
    CpuFeatures result{};
    result.architecture = raw.architecture;
    if (raw.maxLeaf < 1)
        return result;
    result.sse2 = (raw.leaf1edx & (1u << 26)) != 0;
    constexpr auto avxMask = (1u << 26) | (1u << 27) | (1u << 28);
    const bool avxState = (raw.leaf1ecx & avxMask) == avxMask && (raw.xcr0 & 6u) == 6u;
    result.avx2 = raw.maxLeaf >= 7 && avxState && (raw.leaf7ebx & (1u << 5)) != 0;
    constexpr auto wide = (1u << 16) | (1u << 17) | (1u << 28) | (1u << 30) | (1u << 31);
    result.avx512 = raw.maxLeaf >= 7 && result.avx2 && avxState && (raw.xcr0 & 0xe6u) == 0xe6u &&
                    (raw.leaf7ebx & wide) == wide;
    return result;
}
struct CompiledBackends {
    bool sse2 = false, avx2 = false, neon = false, avx512 = false, sve = false;
};
constexpr bool available(CpuBackend backend, CpuFeatures cpu, CompiledBackends built) noexcept {
    const bool x86 = cpu.architecture == Architecture::x86 || cpu.architecture == Architecture::x64;
    const bool arm =
        cpu.architecture == Architecture::arm32 || cpu.architecture == Architecture::arm64;
    switch (backend) {
    case CpuBackend::scalar:
        return true;
    case CpuBackend::sse2:
        return x86 && built.sse2 && cpu.sse2;
    case CpuBackend::avx2:
        return x86 && built.avx2 && cpu.avx2;
    case CpuBackend::avx512:
        return x86 && built.avx512 && cpu.avx512;
    case CpuBackend::sve:
        return cpu.architecture == Architecture::arm64 && built.sve && cpu.sve;
    case CpuBackend::neon:
        return arm && built.neon && cpu.neon;
    default:
        return false;
    }
}
constexpr CpuBackend chooseBackend(CpuFeatures cpu, CompiledBackends built) noexcept {
    if (available(CpuBackend::avx512, cpu, built))
        return CpuBackend::avx512;
    if (available(CpuBackend::sve, cpu, built))
        return CpuBackend::sve;
    if (available(CpuBackend::avx2, cpu, built))
        return CpuBackend::avx2;
    if (available(CpuBackend::sse2, cpu, built))
        return CpuBackend::sse2;
    if (available(CpuBackend::neon, cpu, built))
        return CpuBackend::neon;
    return CpuBackend::scalar;
}
} // namespace cydsp::detail
