#include "cydsp/spatial/SpatialMath.h"
#include "cydsp/core/Memory.h"
#include "detail/AudioMath.h"
#include <algorithm>
#include <cmath>
#include <numbers>
namespace cydsp {
Vec3 operator+(Vec3 a, Vec3 b) noexcept {
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}
Vec3 operator-(Vec3 a, Vec3 b) noexcept {
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}
Vec3 operator*(Vec3 a, double b) noexcept {
    return {a.x * b, a.y * b, a.z * b};
}
double dot(Vec3 a, Vec3 b) noexcept {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
Vec3 cross(Vec3 a, Vec3 b) noexcept {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
double length(Vec3 a) noexcept {
    return std::hypot(a.x, a.y, a.z);
}
namespace {
bool validVector(Vec3 v, double limit) {
    return isFinite(v.x) && isFinite(v.y) && isFinite(v.z) && std::abs(v.x) <= limit &&
           std::abs(v.y) <= limit && std::abs(v.z) <= limit;
}
bool validPose(const Pose3& p) {
    Quaternion q;
    return validVector(p.position, 1e9) && normalize(p.orientation, q) == Status::ok;
}
bool validSource(const SpatialSource& s) {
    return validPose(s.pose) && validVector(s.velocity, 1e6) && isFinite(s.radiusMetres) &&
           s.radiusMetres >= 0 && s.radiusMetres <= 1e6;
}
bool validListener(const SpatialListener& l) {
    return validPose(l.pose) && validVector(l.velocity, 1e6) && isFinite(l.headRadiusMetres) &&
           l.headRadiusMetres > 0 && l.headRadiusMetres <= 1;
}
} // namespace
Status normalize(Quaternion q, Quaternion& output) noexcept {
    if (!isFinite(q.w) || !isFinite(q.x) || !isFinite(q.y) || !isFinite(q.z))
        return Status::invalid_argument;
    const double magnitude = std::hypot(std::hypot(q.w, q.x), std::hypot(q.y, q.z));
    if (magnitude < 1e-12 || magnitude > 1e12)
        return Status::invalid_argument;
    output = {q.w / magnitude, q.x / magnitude, q.y / magnitude, q.z / magnitude};
    return Status::ok;
}
Vec3 rotate(Quaternion q, Vec3 v) noexcept {
    const Vec3 axis{q.x, q.y, q.z};
    const auto twice = cross(axis, v) * 2;
    return v + twice * q.w + cross(axis, twice);
}
Status interpolatePose(const Pose3& a, const Pose3& b, double t, Pose3& output) noexcept {
    Quaternion qa, qb;
    if (!validVector(a.position, 1e9) || !validVector(b.position, 1e9) || !isFinite(t) || t < 0 ||
        t > 1 || normalize(a.orientation, qa) != Status::ok ||
        normalize(b.orientation, qb) != Status::ok)
        return Status::invalid_argument;
    double cosine = qa.w * qb.w + qa.x * qb.x + qa.y * qb.y + qa.z * qb.z;
    if (cosine < 0) {
        qb = {-qb.w, -qb.x, -qb.y, -qb.z};
        cosine = -cosine;
    }
    double x = 1 - t, y = t;
    if (cosine < .9995) {
        const auto angle = std::acos(std::clamp(cosine, -1., 1.));
        x = std::sin((1 - t) * angle) / std::sin(angle);
        y = std::sin(t * angle) / std::sin(angle);
    }
    Quaternion q{x * qa.w + y * qb.w, x * qa.x + y * qb.x, x * qa.y + y * qb.y,
                 x * qa.z + y * qb.z};
    Quaternion unit;
    (void)normalize(q, unit);
    output = {a.position + (b.position - a.position) * t, unit};
    return Status::ok;
}
Status relativeSource(const SpatialSource& source, const SpatialListener& listener,
                      RelativeSource& output) noexcept {
    if (!validSource(source) || !validListener(listener))
        return Status::invalid_argument;
    RelativeSource result;
    const auto delta = source.pose.position - listener.pose.position;
    result.distanceMetres = length(delta);
    if (result.distanceMetres > 1e-12)
        result.direction = delta * (1 / result.distanceMetres);
    Quaternion orientation;
    (void)normalize(listener.pose.orientation, orientation);
    orientation.x = -orientation.x;
    orientation.y = -orientation.y;
    orientation.z = -orientation.z;
    result.localDirection = rotate(orientation, result.direction);
    result.azimuthRadians = std::atan2(result.localDirection.y, result.localDirection.x);
    result.elevationRadians = std::asin(std::clamp(result.localDirection.z, -1., 1.));
    result.sourceRadialVelocity = dot(source.velocity, result.direction);
    result.listenerRadialVelocity = dot(listener.velocity, result.direction);
    output = result;
    return Status::ok;
}
Status relativeSources(std::span<const SpatialSource> sources,
                       std::span<const SpatialListener> listeners,
                       std::span<RelativeSource> out) noexcept {
    if (sources.empty() || sources.size() > 4096 || listeners.empty() || listeners.size() > 16 ||
        out.size() != sources.size() * listeners.size())
        return Status::invalid_argument;
    if (memory::overlaps(out.data(), out.size_bytes(), sources.data(), sources.size_bytes()) ||
        memory::overlaps(out.data(), out.size_bytes(), listeners.data(), listeners.size_bytes()))
        return Status::partial_overlap;
    for (const auto& s : sources)
        if (!validSource(s))
            return Status::invalid_argument;
    for (const auto& l : listeners)
        if (!validListener(l))
            return Status::invalid_argument;
    for (std::size_t l = 0; l < listeners.size(); ++l)
        for (std::size_t s = 0; s < sources.size(); ++s)
            (void)relativeSource(sources[s], listeners[l], out[l * sources.size() + s]);
    return Status::ok;
}
bool validMedium(const AcousticMedium& m) noexcept {
    if (!isFinite(m.speedMetresPerSecond) || m.speedMetresPerSecond < 1 ||
        m.speedMetresPerSecond > 10000 || !isFinite(m.densityKgPerCubicMetre) ||
        m.densityKgPerCubicMetre <= 0 || m.densityKgPerCubicMetre > 30000)
        return false;
    for (double a : m.absorptionDbPerMetre)
        if (!isFinite(a) || a < 0 || a > 1000)
            return false;
    return true;
}
Status atmosphericAbsorption(double f, double temperature, double humidity, double& output,
                             double pressure) noexcept {
    if (!isFinite(f) || f < 20 || f > 24000 || !isFinite(temperature) || temperature < -40 ||
        temperature > 60 || !isFinite(humidity) || humidity < 0 || humidity > 100 ||
        !isFinite(pressure) || pressure < 20000 || pressure > 200000)
        return Status::invalid_argument;
    const double t = temperature + 273.15, tr = t / 293.15, pr = pressure / 101325,
                 h = humidity * std::pow(10., -6.8346 * std::pow(273.16 / t, 1.261) + 4.6151) / pr;
    const double oxygen = pr * (24 + 40400 * h * (.02 + h) / (.391 + h)),
                 nitrogen = pr * std::pow(tr, -.5) *
                            (9 + 280 * h * std::exp(-4.17 * (std::pow(tr, -1. / 3) - 1)));
    const double square = f * f;
    output = 8.686 * square *
             (1.84e-11 * std::sqrt(tr) / pr +
              std::pow(tr, -2.5) * (.01275 * std::exp(-2239.1 / t) / (oxygen + square / oxygen) +
                                    .1068 * std::exp(-3352. / t) / (nitrogen + square / nitrogen)));
    return Status::ok;
}
Status airMedium(double temperature, double humidity, AcousticMedium& output,
                 double pressure) noexcept {
    AcousticMedium result;
    const std::array<double, 3> frequencies{250, 2000, 8000};
    for (std::size_t i = 0; i < 3; ++i) {
        auto status = atmosphericAbsorption(frequencies[i], temperature, humidity,
                                            result.absorptionDbPerMetre[i], pressure);
        if (status != Status::ok)
            return status;
    }
    result.speedMetresPerSecond = 331.3 * std::sqrt((temperature + 273.15) / 273.15);
    result.densityKgPerCubicMetre = pressure / (287.05 * (temperature + 273.15));
    output = result;
    return Status::ok;
}
AcousticMedium waterMedium() noexcept {
    return {1480, 1000, {0, 0, 0}};
}
Status acousticTravel(std::span<const MediumSegment> segments, AcousticTravel& output) noexcept {
    if (segments.size() > 1024)
        return Status::invalid_argument;
    AcousticTravel result;
    std::array<double, 3> loss{};
    for (const auto& s : segments) {
        if (!validMedium(s.medium) || !isFinite(s.lengthMetres) || s.lengthMetres < 0 ||
            s.lengthMetres > 1e12)
            return Status::invalid_argument;
        result.seconds += s.lengthMetres / s.medium.speedMetresPerSecond;
        for (std::size_t b = 0; b < 3; ++b)
            loss[b] += s.medium.absorptionDbPerMetre[b] * s.lengthMetres;
    }
    for (std::size_t b = 0; b < 3; ++b)
        result.bandGains[b] = std::pow(10., -loss[b] / 20);
    output = result;
    return Status::ok;
}
Status distanceGain(double distance, const DistanceAttenuation& p, double& output) noexcept {
    if (!isFinite(distance) || distance < 0 || distance > 1e12 || !isFinite(p.referenceMetres) ||
        p.referenceMetres <= 0 || !isFinite(p.maximumMetres) ||
        p.maximumMetres <= p.referenceMetres || !isFinite(p.rolloff) || p.rolloff < 0 ||
        p.rolloff > 8 || unsigned(p.model) > unsigned(DistanceModel::linear))
        return Status::invalid_argument;
    double d = std::max(distance, p.referenceMetres);
    if (p.clampDistance)
        d = std::min(d, p.maximumMetres);
    if (p.model == DistanceModel::inverse)
        output = p.referenceMetres / (p.referenceMetres + p.rolloff * (d - p.referenceMetres));
    else if (p.model == DistanceModel::exponential)
        output = std::pow(d / p.referenceMetres, -p.rolloff);
    else
        output = std::max(0., 1 - p.rolloff * (d - p.referenceMetres) /
                                      (p.maximumMetres - p.referenceMetres));
    return Status::ok;
}
Status dopplerRatio(const RelativeSource& source, const AcousticMedium& medium, double& output,
                    double intensity, double limit) noexcept {
    if (!validMedium(medium) || !isFinite(source.sourceRadialVelocity) ||
        !isFinite(source.listenerRadialVelocity) || std::abs(source.sourceRadialVelocity) > 1e7 ||
        std::abs(source.listenerRadialVelocity) > 1e7 || !isFinite(intensity) || intensity < 0 ||
        intensity > 4 || !isFinite(limit) || limit < 0 || limit > 96)
        return Status::invalid_argument;
    const auto c = medium.speedMetresPerSecond;
    const auto vs = std::clamp(source.sourceRadialVelocity * intensity, -.95 * c, .95 * c),
               vl = std::clamp(source.listenerRadialVelocity * intensity, -.95 * c, .95 * c);
    const double maximum = std::exp2(limit / 12);
    output = std::clamp((c + vl) / (c + vs), 1 / maximum, maximum);
    return Status::ok;
}
Status directivityGains(double cosine, const Directivity& p,
                        std::array<double, 3>& output) noexcept {
    if (!isFinite(cosine) || cosine < -1 || cosine > 1 ||
        unsigned(p.pattern) > unsigned(DirectivityPattern::custom) ||
        !isFinite(p.customOmniWeight) || p.customOmniWeight < 0 || p.customOmniWeight > 1 ||
        !isFinite(p.amount) || p.amount < 0 || p.amount > 1 || !isFinite(p.minimumGainDb) ||
        p.minimumGainDb > 0 || p.minimumGainDb < -160)
        return Status::invalid_argument;
    for (double loss : p.rearLossDb)
        if (!isFinite(loss) || loss < 0 || loss > 120)
            return Status::invalid_argument;
    constexpr std::array<double, 6> weights{1, .75, .5, .37, .25, 0};
    const double a = p.pattern == DirectivityPattern::custom ? p.customOmniWeight
                                                             : weights[std::size_t(p.pattern)];
    double value = a + (1 - a) * cosine;
    value = std::copysign(
        std::max(std::abs(value), detail::audioExp(p.minimumGainDb * (std::numbers::ln10 / 20))),
        value);
    if (!p.preservePolarity)
        value = std::abs(value);
    std::array<double, 3> result{};
    for (std::size_t b = 0; b < 3; ++b)
        result[b] =
            1 - p.amount +
            p.amount * value *
                detail::audioExp(-p.rearLossDb[b] * (1 - cosine) * (std::numbers::ln10 / 40));
    output = result;
    return Status::ok;
}
Status materialResponse(const AcousticMaterial& material, double thickness,
                        MaterialResponse& output) noexcept {
    if (!isFinite(thickness) || thickness < 0 || thickness > 1e6 ||
        !isFinite(material.referenceThicknessMetres) || material.referenceThicknessMetres < 1e-6 ||
        material.referenceThicknessMetres > 1e6 || !isFinite(material.scattering) ||
        material.scattering < 0 || material.scattering > 1)
        return Status::invalid_argument;
    MaterialResponse result;
    for (std::size_t b = 0; b < 3; ++b) {
        const auto a = material.absorption[b], loss = material.transmissionLossDb[b];
        if (!isFinite(a) || a < 0 || a > 1 || !isFinite(loss) || loss < 0 || loss > 300)
            return Status::invalid_argument;
        const double scale = thickness / material.referenceThicknessMetres;
        const double base = 1 - a;
        const double remaining = scale == 0    ? 1
                                 : scale == 1  ? base
                                 : scale == 2  ? base * base
                                 : scale == .5 ? std::sqrt(base)
                                               : std::pow(base, scale);
        const double absorption = 1 - remaining;
        const double transmission =
            std::min(1 - absorption, detail::audioExp(-loss * scale * (std::numbers::ln10 / 10)));
        result.transmission[b] = std::sqrt(transmission);
        result.reflection[b] =
            std::sqrt(std::max(0., 1 - absorption - transmission) * (1 - material.scattering));
    }
    output = result;
    return Status::ok;
}
Status evaluatePropagation(const SpatialSource& source, const SpatialListener& listener,
                           const AcousticMedium& medium, const DistanceAttenuation& attenuation,
                           const Directivity& directivity, PropagationResult& output) noexcept {
    if (!validMedium(medium))
        return Status::invalid_argument;
    PropagationResult result;
    auto status = relativeSource(source, listener, result.relative);
    if (status != Status::ok)
        return status;
    double gain = 1;
    status = distanceGain(std::max(0., result.relative.distanceMetres - source.radiusMetres),
                          attenuation, gain);
    if (status != Status::ok)
        return status;
    const MediumSegment segment{result.relative.distanceMetres, medium};
    status = acousticTravel({&segment, 1}, result.travel);
    if (status != Status::ok)
        return status;
    Quaternion orientation;
    (void)normalize(source.pose.orientation, orientation);
    const double cosine = dot(rotate(orientation, {1, 0, 0}), result.relative.direction * (-1));
    std::array<double, 3> direction{};
    status = directivityGains(std::clamp(cosine, -1., 1.), directivity, direction);
    if (status != Status::ok)
        return status;
    status = dopplerRatio(result.relative, medium, result.doppler);
    if (status != Status::ok)
        return status;
    for (std::size_t b = 0; b < 3; ++b)
        result.travel.bandGains[b] *= gain * direction[b];
    output = result;
    return Status::ok;
}
} // namespace cydsp
