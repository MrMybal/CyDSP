#include "cydsp/processors/PolynomialInterpolator.h"
#include "detail/Alias.h"
#include <cmath>
namespace cydsp {
template <Sample T>
Status PolynomialInterpolator<T>::prepare(const ProcessSpec& spec, double inputRate,
                                          PolynomialInterpolation type, unsigned order,
                                          CpuBackend backend) noexcept {
    if (!spec.isValid() || !isFinite(inputRate) || inputRate <= 0 ||
        (type != PolynomialInterpolation::lagrange &&
         type != PolynomialInterpolation::catmullRom) ||
        !order || order > 8 || (type == PolynomialInterpolation::catmullRom && order != 3))
        return Status::invalid_spec;
    VectorOps ops;
    const auto r = VectorOps::create(backend, ops);
    if (r != Status::ok)
        return r;
    const auto ratio = inputRate / spec.sampleRate;
    if (!isFinite(ratio) || ratio <= 0)
        return Status::invalid_spec;
    spec_ = spec;
    ratio_ = ratio;
    ops_ = ops;
    order_ = order;
    type_ = type;
    ready_ = true;
    return Status::ok;
}
template <Sample T>
Status PolynomialInterpolator<T>::process(AudioBlock<const T> source, double first,
                                          AudioBlock<T> output) noexcept {
    if (!ready_)
        return Status::not_prepared;
    auto r = validateProcessBlocks<T>(spec_, output, output);
    if (r != Status::ok)
        return r;
    if (!source.isValid() || source.channels() != spec_.channels || !isFinite(first) || first < 0)
        return Status::invalid_argument;
    if (!output.frames())
        return Status::ok;
    const auto last = first + double(output.frames() - 1) * ratio_;
    const auto left = order_ / 2, right = order_ - left;
    if (!isFinite(last) || std::floor(first) < left ||
        std::floor(last) + right >= double(source.frames()))
        return Status::invalid_argument;
    for (std::size_t a = 0; a < source.channels(); ++a)
        for (std::size_t b = 0; b < output.channels(); ++b)
            if (detail::overlaps(source.channel(a).data(), source.frames() * sizeof(T),
                                 output.channel(b).data(), output.frames() * sizeof(T)))
                return Status::partial_overlap;
    std::array<double, 9> weights{};
    for (std::size_t f = 0; f < output.frames(); ++f) {
        const auto pos = first + double(f) * ratio_, base = std::floor(pos), t = pos - base;
        if (type_ == PolynomialInterpolation::catmullRom) {
            weights[0] = -.5 * t + t * t - .5 * t * t * t;
            weights[1] = 1 - 2.5 * t * t + 1.5 * t * t * t;
            weights[2] = .5 * t + 2 * t * t - 1.5 * t * t * t;
            weights[3] = -.5 * t * t + .5 * t * t * t;
        } else {
            const auto x = t + left;
            for (unsigned i = 0; i <= order_; ++i) {
                weights[i] = 1;
                for (unsigned j = 0; j <= order_; ++j)
                    if (i != j)
                        weights[i] *= (x - j) / (double(i) - j);
            }
        }
        const auto index = static_cast<std::size_t>(base) - left;
        for (std::size_t ch = 0; ch < output.channels(); ++ch) {
            double value = 0;
            (void)ops_.dot(source.channel(ch).subspan(index, order_ + 1),
                           std::span<const double>(weights).first(order_ + 1), value);
            output.channel(ch)[f] = static_cast<T>(value);
        }
    }
    return Status::ok;
}
template class PolynomialInterpolator<float>;
template class PolynomialInterpolator<double>;
} // namespace cydsp
