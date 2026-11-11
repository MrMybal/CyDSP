#include "cydsp/math/VectorOps.h"
#include "detail/Alias.h"
#include "detail/CpuPolicy.h"
#include "detail/VectorTable.h"
namespace cydsp {
VectorOps::VectorOps() noexcept : table_(&detail::scalarTable()) {}
Status VectorOps::create(CpuBackend requested, VectorOps& result) noexcept {
    const auto cpu = detectCpuFeatures();
    constexpr detail::CompiledBackends built{CYDSP_HAS_SSE2 != 0, CYDSP_HAS_AVX2 != 0,
                                             CYDSP_HAS_NEON != 0, CYDSP_HAS_AVX512 != 0,
                                             CYDSP_HAS_SVE != 0};
    const auto selected =
        requested == CpuBackend::automatic ? detail::chooseBackend(cpu, built) : requested;
    if (!detail::available(selected, cpu, built))
        return Status::unsupported_backend;
    const auto* table = &detail::scalarTable();
    switch (selected) {
    case CpuBackend::scalar:
        break;
#if CYDSP_HAS_SSE2
    case CpuBackend::sse2:
        table = &detail::sse2Table();
        break;
#endif
#if CYDSP_HAS_AVX2
    case CpuBackend::avx2:
        table = &detail::avx2Table();
        break;
#endif
#if CYDSP_HAS_NEON
    case CpuBackend::neon:
        table = &detail::neonTable();
        break;
#endif
#if CYDSP_HAS_AVX512
    case CpuBackend::avx512:
        table = &detail::avx512Table();
        break;
#endif
#if CYDSP_HAS_SVE
    case CpuBackend::sve:
        table = &detail::sveTable();
        break;
#endif
    default:
        break;
    }
    result.table_ = table;
    return Status::ok;
}
CpuBackend VectorOps::backend() const noexcept {
    return table_->backend;
}
namespace {
template <class T> Status validate(std::span<const T> source, std::span<T> destination) noexcept {
    if (source.size() != destination.size())
        return Status::invalid_argument;
    if (source.data() != destination.data() &&
        detail::overlaps(source.data(), source.size_bytes(), destination.data(),
                         destination.size_bytes()))
        return Status::partial_overlap;
    return Status::ok;
}
} // namespace
Status VectorOps::copy(std::span<const float> source, std::span<float> destination) const noexcept {
    const auto status = validate(source, destination);
    if (status != Status::ok)
        return status;
    table_->f32.copy(source.data(), destination.data(), source.size());
    return Status::ok;
}
Status VectorOps::scale(std::span<const float> source, std::span<float> destination,
                        float gain) const noexcept {
    const auto status = validate(source, destination);
    if (status != Status::ok)
        return status;
    if (!isFinite(gain))
        return Status::invalid_argument;
    table_->f32.scale(source.data(), destination.data(), source.size(), gain);
    return Status::ok;
}
Status VectorOps::addScaled(std::span<const float> source, std::span<float> destination,
                            float gain) const noexcept {
    const auto status = validate(source, destination);
    if (status != Status::ok)
        return status;
    if (!isFinite(gain))
        return Status::invalid_argument;
    table_->f32.addScaled(source.data(), destination.data(), source.size(), gain);
    return Status::ok;
}
void VectorOps::fill(std::span<float> destination, float value) const noexcept {
    table_->f32.fill(destination.data(), destination.size(), value);
}
Status VectorOps::copy(std::span<const double> source,
                       std::span<double> destination) const noexcept {
    const auto status = validate(source, destination);
    if (status != Status::ok)
        return status;
    table_->f64.copy(source.data(), destination.data(), source.size());
    return Status::ok;
}
Status VectorOps::scale(std::span<const double> source, std::span<double> destination,
                        double gain) const noexcept {
    const auto status = validate(source, destination);
    if (status != Status::ok)
        return status;
    if (!isFinite(gain))
        return Status::invalid_argument;
    table_->f64.scale(source.data(), destination.data(), source.size(), gain);
    return Status::ok;
}
Status VectorOps::addScaled(std::span<const double> source, std::span<double> destination,
                            double gain) const noexcept {
    const auto status = validate(source, destination);
    if (status != Status::ok)
        return status;
    if (!isFinite(gain))
        return Status::invalid_argument;
    table_->f64.addScaled(source.data(), destination.data(), source.size(), gain);
    return Status::ok;
}
void VectorOps::fill(std::span<double> destination, double value) const noexcept {
    table_->f64.fill(destination.data(), destination.size(), value);
}
} // namespace cydsp

namespace cydsp {
Status VectorOps::multiply(std::span<const float> a, std::span<const float> b,
                           std::span<float> d) const noexcept {
    auto status = validate(a, d);
    if (status != Status::ok)
        return status;
    status = validate(b, d);
    if (status != Status::ok)
        return status;
    table_->f32.multiply(a.data(), b.data(), d.data(), a.size());
    return Status::ok;
}
Status VectorOps::complexMultiplyAccumulate(std::span<const std::complex<float>> a,
                                            std::span<const std::complex<float>> b,
                                            std::span<std::complex<float>> d) const noexcept {
    auto status = validate(a, d);
    if (status != Status::ok)
        return status;
    status = validate(b, d);
    if (status != Status::ok)
        return status;
    // C++ [complex.numbers.general] guarantees array-oriented scalar access.
    table_->f32.complexMac(reinterpret_cast<const float*>(a.data()),
                           reinterpret_cast<const float*>(b.data()),
                           reinterpret_cast<float*>(d.data()), a.size());
    return Status::ok;
}
Status VectorOps::complexButterfly(std::span<std::complex<float>> even,
                                   std::span<std::complex<float>> odd,
                                   std::span<const std::complex<float>> w,
                                   bool conjugate) const noexcept {
    if (even.size() != odd.size() || even.size() != w.size())
        return Status::invalid_argument;
    if (detail::overlaps(even.data(), even.size_bytes(), odd.data(), odd.size_bytes()) ||
        detail::overlaps(even.data(), even.size_bytes(), w.data(), w.size_bytes()) ||
        detail::overlaps(odd.data(), odd.size_bytes(), w.data(), w.size_bytes()))
        return Status::partial_overlap;
    table_->f32.butterfly(reinterpret_cast<float*>(even.data()),
                          reinterpret_cast<float*>(odd.data()),
                          reinterpret_cast<const float*>(w.data()), even.size(), conjugate);
    return Status::ok;
}
Status VectorOps::multiply(std::span<const double> a, std::span<const double> b,
                           std::span<double> d) const noexcept {
    auto status = validate(a, d);
    if (status != Status::ok)
        return status;
    status = validate(b, d);
    if (status != Status::ok)
        return status;
    table_->f64.multiply(a.data(), b.data(), d.data(), a.size());
    return Status::ok;
}
Status VectorOps::complexMultiplyAccumulate(std::span<const std::complex<double>> a,
                                            std::span<const std::complex<double>> b,
                                            std::span<std::complex<double>> d) const noexcept {
    auto status = validate(a, d);
    if (status != Status::ok)
        return status;
    status = validate(b, d);
    if (status != Status::ok)
        return status;
    // C++ [complex.numbers.general] guarantees array-oriented scalar access.
    table_->f64.complexMac(reinterpret_cast<const double*>(a.data()),
                           reinterpret_cast<const double*>(b.data()),
                           reinterpret_cast<double*>(d.data()), a.size());
    return Status::ok;
}
Status VectorOps::complexButterfly(std::span<std::complex<double>> even,
                                   std::span<std::complex<double>> odd,
                                   std::span<const std::complex<double>> w,
                                   bool conjugate) const noexcept {
    if (even.size() != odd.size() || even.size() != w.size())
        return Status::invalid_argument;
    if (detail::overlaps(even.data(), even.size_bytes(), odd.data(), odd.size_bytes()) ||
        detail::overlaps(even.data(), even.size_bytes(), w.data(), w.size_bytes()) ||
        detail::overlaps(odd.data(), odd.size_bytes(), w.data(), w.size_bytes()))
        return Status::partial_overlap;
    table_->f64.butterfly(reinterpret_cast<double*>(even.data()),
                          reinterpret_cast<double*>(odd.data()),
                          reinterpret_cast<const double*>(w.data()), even.size(), conjugate);
    return Status::ok;
}
} // namespace cydsp

namespace cydsp {
Status VectorOps::dot(std::span<const float> a, std::span<const float> b,
                      float& result) const noexcept {
    if (a.size() != b.size())
        return Status::invalid_argument;
    result = table_->f32.dot(a.data(), b.data(), a.size());
    return Status::ok;
}
Status VectorOps::dot(std::span<const double> a, std::span<const double> b,
                      double& result) const noexcept {
    if (a.size() != b.size())
        return Status::invalid_argument;
    result = table_->f64.dot(a.data(), b.data(), a.size());
    return Status::ok;
}
} // namespace cydsp

namespace cydsp {
Status VectorOps::dot(std::span<const float> a, std::span<const double> b,
                      double& result) const noexcept {
    if (a.size() != b.size())
        return Status::invalid_argument;
    result = table_->dotMixed(a.data(), b.data(), a.size());
    return Status::ok;
}
} // namespace cydsp
