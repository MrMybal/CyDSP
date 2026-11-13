#pragma once
#include "cydsp/core/AudioBlock.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
namespace cydsp {
enum class OrthogonalMix { hadamard, householder };
// Stateless lossless mixing, 1..64 lanes. Input is snapshotted on the stack, so
// exact/partial alias is supported. Hadamard requires a power-of-two lane count.
template <Sample T>
[[nodiscard]] Status orthogonalMix(std::span<const T> input, std::span<T> output,
                                   OrthogonalMix kind = OrthogonalMix::hadamard) noexcept {
    if (input.empty() || input.size() > 64 || output.size() != input.size() ||
        unsigned(kind) > unsigned(OrthogonalMix::householder) ||
        (kind == OrthogonalMix::hadamard && !isPowerOfTwo(input.size())))
        return Status::invalid_argument;
    std::array<double, 64> values{};
    double sum = 0;
    for (std::size_t i = 0; i < input.size(); ++i) {
        if (!isFinite(input[i]))
            return Status::invalid_argument;
        values[i] = double(input[i]);
        sum += values[i];
    }
    if (kind == OrthogonalMix::householder) {
        for (std::size_t i = 0; i < input.size(); ++i)
            output[i] = T(values[i] - 2 * sum / double(input.size()));
    } else {
        for (std::size_t step = 1; step < input.size(); step *= 2)
            for (std::size_t at = 0; at < input.size(); at += 2 * step)
                for (std::size_t j = 0; j < step; ++j) {
                    const auto a = values[at + j], b = values[at + j + step];
                    values[at + j] = a + b;
                    values[at + j + step] = a - b;
                }
        const double scale = 1 / std::sqrt(double(input.size()));
        for (std::size_t i = 0; i < input.size(); ++i)
            output[i] = T(values[i] * scale);
    }
    return Status::ok;
}
struct DiffusionStage {
    std::size_t delayFrames = 1;
    double feedback = .5;
};
// Cascade of integer-delay Schroeder all-pass sections, |feedback|<1.
template <Sample T> class Diffuser {
  public:
    Diffuser();
    ~Diffuser();
    Diffuser(Diffuser&&) noexcept;
    Diffuser& operator=(Diffuser&&) noexcept;
    [[nodiscard]] Status prepare(const ProcessSpec&, std::span<const DiffusionStage>) noexcept;
    [[nodiscard]] Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
    void reset() noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
extern template class Diffuser<float>;
extern template class Diffuser<double>;
} // namespace cydsp
