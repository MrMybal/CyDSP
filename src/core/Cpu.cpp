#include "cydsp/core/Cpu.h"
#include "detail/CpuPolicy.h"
#if defined(_M_IX86) || defined(_M_X64)
#include <intrin.h>
#elif defined(__i386__) || defined(__x86_64__)
#include <cpuid.h>
#endif
#if (defined(__arm__) || defined(__aarch64__)) && defined(__linux__)
#include <asm/hwcap.h>
#include <sys/auxv.h>
#elif defined(_M_ARM) && defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif
namespace cydsp {
CpuFeatures detectCpuFeatures() noexcept {
#if defined(_M_IX86) || defined(_M_X64) || defined(__i386__) || defined(__x86_64__)
    detail::X86Snapshot raw;
#if defined(_M_IX86) || defined(__i386__)
    raw.architecture = Architecture::x86;
#endif
#if defined(_MSC_VER)
    int leaf[4]{};
    __cpuid(leaf, 0);
    raw.maxLeaf = static_cast<std::uint32_t>(leaf[0]);
    if (raw.maxLeaf >= 1) {
        __cpuidex(leaf, 1, 0);
        raw.leaf1ecx = static_cast<std::uint32_t>(leaf[2]);
        raw.leaf1edx = static_cast<std::uint32_t>(leaf[3]);
        constexpr auto xsave = (1u << 26) | (1u << 27);
        if ((raw.leaf1ecx & xsave) == xsave)
            raw.xcr0 = _xgetbv(0);
    }
    if (raw.maxLeaf >= 7) {
        __cpuidex(leaf, 7, 0);
        raw.leaf7ebx = static_cast<std::uint32_t>(leaf[1]);
    }
#else
    unsigned eax{}, ebx{}, ecx{}, edx{};
    raw.maxLeaf = __get_cpuid_max(0, nullptr);
    if (raw.maxLeaf >= 1) {
        __cpuid_count(1, 0, eax, ebx, ecx, edx);
        raw.leaf1ecx = ecx;
        raw.leaf1edx = edx;
        constexpr auto xsave = (1u << 26) | (1u << 27);
        if ((ecx & xsave) == xsave) {
            unsigned lo{}, hi{};
            __asm__ volatile("xgetbv" : "=a"(lo), "=d"(hi) : "c"(0));
            raw.xcr0 = static_cast<std::uint64_t>(lo) | (static_cast<std::uint64_t>(hi) << 32);
        }
    }
    if (raw.maxLeaf >= 7) {
        __cpuid_count(7, 0, eax, ebx, ecx, edx);
        raw.leaf7ebx = ebx;
    }
#endif
    return detail::decodeX86(raw);
#elif defined(__aarch64__) || defined(_M_ARM64)
    CpuFeatures result{Architecture::arm64, false, false, true};
#if defined(__linux__) && defined(HWCAP_SVE)
    result.sve = (getauxval(AT_HWCAP) & HWCAP_SVE) != 0;
#endif
    return result;
#elif defined(__arm__) || defined(_M_ARM)
    bool neon = false;
#if defined(__linux__) && defined(HWCAP_NEON)
    neon = (getauxval(AT_HWCAP) & HWCAP_NEON) != 0;
#elif defined(_WIN32) && defined(PF_ARM_NEON_INSTRUCTIONS_AVAILABLE)
    neon = IsProcessorFeaturePresent(PF_ARM_NEON_INSTRUCTIONS_AVAILABLE) != 0;
#endif
    return {Architecture::arm32, false, false, neon};
#else
    return {};
#endif
}
const char* backendName(CpuBackend backend) noexcept {
    switch (backend) {
    case CpuBackend::automatic:
        return "auto";
    case CpuBackend::scalar:
        return "scalar";
    case CpuBackend::sse2:
        return "sse2";
    case CpuBackend::avx2:
        return "avx2";
    case CpuBackend::avx512:
        return "avx512";
    case CpuBackend::sve:
        return "sve";
    case CpuBackend::neon:
        return "neon";
    }
    return "unknown";
}
} // namespace cydsp
