#include "cydsp/spatial/Diffraction.h"
#include "detail/Analysis.h"
#include <algorithm>
#include <cmath>
#include <numbers>
namespace cydsp {
Status diffractionGain(const DiffractionPath& p, double f, double& gain) noexcept {
    if (!isFinite(p.sourceToEdgeMetres) || p.sourceToEdgeMetres < .001 ||
        p.sourceToEdgeMetres > 1e6 || !isFinite(p.edgeToListenerMetres) ||
        p.edgeToListenerMetres < .001 || p.edgeToListenerMetres > 1e6 ||
        !isFinite(p.edgeOffsetMetres) || std::abs(p.edgeOffsetMetres) > 1e6 ||
        !isFinite(p.soundSpeed) || p.soundSpeed < 100 || p.soundSpeed > 2000 ||
        !isFinite(p.openingWidthMetres) || p.openingWidthMetres < 0 ||
        p.openingWidthMetres > 1000 || !isFinite(p.openingAngleRadians) ||
        std::abs(p.openingAngleRadians) > std::numbers::pi / 2 || !isFinite(f) || f < 0 ||
        f > 768000)
        return Status::invalid_argument;
    // Single knife-edge Fresnel parameter. ITU-R P.526 amplitude-loss fit to
    // the scalar half-plane solution; same dimensionless geometry for acoustics.
    const double v =
        p.edgeOffsetMetres *
        std::sqrt(2 * f / p.soundSpeed * (1 / p.sourceToEdgeMetres + 1 / p.edgeToListenerMetres));
    const double lossDb = v <= -.78 ? 0 : 6.9 + 20 * std::log10(std::hypot(v - .1, 1.) + v - .1);
    double result = std::pow(10., -std::max(0., lossDb) / 20);
    if (p.openingWidthMetres > 0) {
        const double x = std::numbers::pi * f / p.soundSpeed * p.openingWidthMetres *
                         std::sin(p.openingAngleRadians);
        result *= std::abs(x) < 1e-12 ? 1 : std::abs(std::sin(x) / x);
    }
    gain = std::clamp(result, 0., 1.);
    return Status::ok;
}
template <Sample T> struct DiffractionFilter<T>::State {
    BandGain<T> bands;
    double rate = 0;
};
template <Sample T> DiffractionFilter<T>::DiffractionFilter() = default;
template <Sample T> DiffractionFilter<T>::~DiffractionFilter() = default;
template <Sample T> DiffractionFilter<T>::DiffractionFilter(DiffractionFilter&&) noexcept = default;
template <Sample T>
DiffractionFilter<T>& DiffractionFilter<T>::operator=(DiffractionFilter&&) noexcept = default;
template <Sample T> Status DiffractionFilter<T>::prepare(const ProcessSpec& spec) noexcept {
    if (!detail::validAnalysisSpec(spec))
        return Status::invalid_spec;
    try {
        auto s = std::make_unique<State>();
        s->rate = spec.sampleRate;
        auto status = s->bands.prepare(spec, 400, 3000);
        if (status != Status::ok)
            return status;
        state_.swap(s);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
template <Sample T>
Status DiffractionFilter<T>::setPath(const DiffractionPath& p, std::size_t ramp) noexcept {
    if (!state_)
        return Status::not_prepared;
    std::array<double, 3> gains{};
    const std::array<double, 3> frequencies{125, 1000, std::min(8000., state_->rate * .4)};
    for (std::size_t b = 0; b < 3; ++b) {
        auto st = diffractionGain(p, frequencies[b], gains[b]);
        if (st != Status::ok)
            return st;
    }
    return state_->bands.setGains(gains, ramp);
}
template <Sample T>
Status DiffractionFilter<T>::process(AudioBlock<const T> in, AudioBlock<T> out) noexcept {
    if (!state_)
        return Status::not_prepared;
    return state_->bands.process(in, out);
}
template <Sample T> void DiffractionFilter<T>::reset() noexcept {
    if (state_)
        state_->bands.reset();
}
template class DiffractionFilter<float>;
template class DiffractionFilter<double>;
} // namespace cydsp
