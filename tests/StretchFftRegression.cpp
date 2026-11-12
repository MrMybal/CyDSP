#include "detail/StretchFft.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <complex>
#include <cydsp/CyDSP.h>
#include <numbers>
#include <stdexcept>
#include <thread>
void runRealtimeAudit(void (*)(void*) noexcept, void*);
namespace {
using namespace cydsp;
void require(bool v, const char* m) {
    if (!v)
        throw std::runtime_error(m);
}
void ok(Status s) {
    require(s == Status::ok, "DAW status");
}
template <Sample T> void fft() {
    for (auto backend : {CpuBackend::scalar, CpuBackend::sse2, CpuBackend::avx2, CpuBackend::avx512,
                         CpuBackend::neon, CpuBackend::sve}) {
        VectorOps ops;
        if (VectorOps::create(backend, ops) != Status::ok)
            continue;
        for (std::size_t n : {4u, 8u, 12u, 24u, 32u, 96u, 128u, 192u}) {
            detail::StretchFft<T> fft;
            fft.select(backend);
            fft.resize(n);
            std::vector<T> input(n), output(n);
            std::vector<std::complex<T>> spectrum(n / 2);
            for (std::size_t i = 0; i < n; ++i)
                input[i] = static_cast<T>(std::sin(.37 * static_cast<double>(i)) * .2);
            fft.fft(input.data(), spectrum.data());
            for (std::size_t k = 0; k < n / 2; ++k) {
                std::complex<double> reference{};
                for (std::size_t i = 0; i < n; ++i)
                    reference +=
                        static_cast<double>(input[i]) *
                        std::polar(1.0, -2 * std::numbers::pi * (static_cast<double>(k) + .5) *
                                            static_cast<double>(i) / static_cast<double>(n));
                require(std::abs(reference - static_cast<std::complex<double>>(spectrum[k])) <
                            (std::same_as<T, float> ? 2e-5 : 1e-11),
                        "modified FFT independent DFT");
            }
            fft.ifft(spectrum.data(), output.data());
            for (std::size_t i = 0; i < n; ++i)
                require(std::abs(output[i] / static_cast<T>(n) - input[i]) <
                            (std::same_as<T, float> ? 1e-6 : 1e-13),
                        "modified FFT roundtrip");
        }
    }
}
}
void stretchFftRegression() { fft<float>(); fft<double>(); }
