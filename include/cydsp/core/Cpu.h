#pragma once
namespace cydsp {
enum class Architecture { unknown, x86, x64, arm32, arm64 };
enum class CpuBackend { automatic, scalar, sse2, avx2, neon, avx512, sve };
struct CpuFeatures {
    Architecture architecture = Architecture::unknown;
    bool sse2 = false, avx2 = false, neon = false, avx512 = false, sve = false;
};
// Preparation-time query. Wide backends require CPU AND OS register-state support.
[[nodiscard]] CpuFeatures detectCpuFeatures() noexcept;
[[nodiscard]] const char* backendName(CpuBackend) noexcept;
} // namespace cydsp
