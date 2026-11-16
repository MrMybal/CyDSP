#include "cydsp/core/AudioBuffer.h"
#include "cydsp/spatial/SpatialRendering.h"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <vector>
namespace cydsp {
Status sphericalFibonacci(std::span<Vec3> directions, std::span<double> weights) noexcept {
    const auto n = directions.size();
    if (!n || n > 100000 || (!weights.empty() && weights.size() != n))
        return Status::invalid_argument;
    // Validate aliasing before modifying either caller-owned span.
    if (!weights.empty()) {
        const auto a = reinterpret_cast<std::uintptr_t>(directions.data());
        const auto b = reinterpret_cast<std::uintptr_t>(weights.data());
        if (a <= b ? b - a < directions.size_bytes() : a - b < weights.size_bytes())
            return Status::partial_overlap;
    }
    const double golden = std::numbers::pi * (3 - std::sqrt(5.));
    for (std::size_t i = 0; i < n; ++i) {
        const double z = 1 - 2 * (double(i) + .5) / double(n),
                     r = std::sqrt(std::max(0., 1 - z * z));
        const double phi = golden * double(i);
        directions[i] = {r * std::cos(phi), r * std::sin(phi), z};
    }
    std::fill(weights.begin(), weights.end(), 4 * std::numbers::pi / double(n));
    return Status::ok;
}
template <Sample T> struct HrirBank<T>::State {
    std::vector<T> left, right;
    std::vector<HrirPoint<T>> views;
    std::size_t taps = 0;
    double rate = 0;
};
template <Sample T> HrirBank<T>::HrirBank() = default;
template <Sample T> HrirBank<T>::~HrirBank() = default;
template <Sample T> HrirBank<T>::HrirBank(HrirBank&&) noexcept = default;
template <Sample T> HrirBank<T>& HrirBank<T>::operator=(HrirBank&&) noexcept = default;
template <Sample T> std::span<const HrirPoint<T>> HrirBank<T>::points() const noexcept {
    return state_ ? std::span<const HrirPoint<T>>(state_->views) : std::span<const HrirPoint<T>>{};
}
template <Sample T> double HrirBank<T>::sampleRate() const noexcept {
    return state_ ? state_->rate : 0;
}
template <Sample T> std::size_t HrirBank<T>::taps() const noexcept {
    return state_ ? state_->taps : 0;
}
template <Sample T>
Status analyticHrirs(const AnalyticHrirOptions& c, std::span<const Vec3> dirs, HrirBank<T>& out,
                     AnalyticHrirReport* report) noexcept {
    if (dirs.empty() || dirs.size() > 100000 || !c.taps || c.taps > 65536 ||
        !isFinite(c.distanceMetres) || c.distanceMetres < .02 || c.distanceMetres > 100000)
        return Status::invalid_spec;
    const auto perPoint = sizeof(HrirPoint<T>) + 2 * c.taps * sizeof(T);
    if (dirs.size() > c.maximumStorageBytes / perPoint)
        return Status::invalid_spec;
    for (auto d : dirs) {
        const double n = std::hypot(d.x, d.y, d.z);
        if (!isFinite(n) || n < 1e-10 || n > 1e12)
            return Status::invalid_argument;
    }
    try {
        AnalyticBinaural<T> model;
        auto status = model.prepare({c.sampleRate, 64, 1}, c.headRadiusMetres, c.soundSpeed);
        if (status != Status::ok)
            return status;
        auto next = std::make_unique<typename HrirBank<T>::State>();
        next->left.resize(dirs.size() * c.taps);
        next->right.resize(dirs.size() * c.taps);
        next->views.resize(dirs.size());
        next->rate = c.sampleRate;
        next->taps = c.taps;
        AudioBuffer<T> input, output;
        status = input.resize(1, 64);
        if (status != Status::ok)
            return status;
        status = output.resize(2, 64);
        if (status != Status::ok)
            return status;
        AnalyticHrirReport r{dirs.size(), c.taps, 0};
        for (std::size_t p = 0; p < dirs.size(); ++p) {
            const auto d = dirs[p];
            const double n = std::hypot(d.x, d.y, d.z);
            const Vec3 unit{d.x / n, d.y / n, d.z / n};
            const BinauralEmitter emitter{unit * c.distanceMetres, 1, 0};
            model.reset();
            status = model.setEmitters({&emitter, 1}, 0);
            if (status != Status::ok)
                return status;
            std::fill(input.block().channel(0).begin(), input.block().channel(0).end(), T{});
            input.block().channel(0)[0] = T{1};
            std::size_t offset = 0;
            while (offset < c.taps + 64) {
                const auto count = std::min<std::size_t>(64, c.taps + 64 - offset);
                status = model.process(input.block(count), output.block(count));
                if (status != Status::ok)
                    return status;
                for (std::size_t j = 0; j < count; ++j) {
                    const auto i = offset + j;
                    if (i < c.taps) {
                        next->left[p * c.taps + i] = output.block(count).channel(0)[j];
                        next->right[p * c.taps + i] = output.block(count).channel(1)[j];
                    } else
                        for (unsigned ear = 0; ear < 2; ++ear)
                            r.maximumTailSample =
                                std::max(r.maximumTailSample,
                                         std::abs(double(output.block(count).channel(ear)[j])));
                }
                input.block().channel(0)[0] = T{};
                offset += count;
            }
            next->views[p] = {unit,
                              {next->left.data() + p * c.taps, c.taps},
                              {next->right.data() + p * c.taps, c.taps},
                              0,
                              0};
        }
        out.state_.swap(next);
        if (report)
            *report = r;
        return Status::ok;
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (...) {
        return Status::invalid_argument;
    }
}
template class HrirBank<float>;
template class HrirBank<double>;
template Status analyticHrirs<float>(const AnalyticHrirOptions&, std::span<const Vec3>,
                                     HrirBank<float>&, AnalyticHrirReport*) noexcept;
template Status analyticHrirs<double>(const AnalyticHrirOptions&, std::span<const Vec3>,
                                      HrirBank<double>&, AnalyticHrirReport*) noexcept;
} // namespace cydsp
