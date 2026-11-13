#pragma once
#include "cydsp/core/Types.h"
#include <array>
#include <span>
namespace cydsp {
// Right-handed metres: +X forward, +Y left, +Z up. Angles are radians.
struct Vec3 {
    double x = 0, y = 0, z = 0;
};
struct Quaternion {
    double w = 1, x = 0, y = 0, z = 0;
};
struct Pose3 {
    Vec3 position;
    Quaternion orientation;
};
struct SpatialSource {
    Pose3 pose;
    Vec3 velocity;
    double radiusMetres = 0;
};
struct SpatialListener {
    Pose3 pose;
    Vec3 velocity;
    double headRadiusMetres = .0875;
};
struct RelativeSource {
    Vec3 direction{1, 0, 0}, localDirection{1, 0, 0};
    double distanceMetres = 0, azimuthRadians = 0, elevationRadians = 0;
    double sourceRadialVelocity = 0, listenerRadialVelocity = 0;
};
[[nodiscard]] Vec3 operator+(Vec3, Vec3) noexcept;
[[nodiscard]] Vec3 operator-(Vec3, Vec3) noexcept;
[[nodiscard]] Vec3 operator*(Vec3, double) noexcept;
[[nodiscard]] double dot(Vec3, Vec3) noexcept;
[[nodiscard]] Vec3 cross(Vec3, Vec3) noexcept;
[[nodiscard]] double length(Vec3) noexcept;
[[nodiscard]] Status normalize(Quaternion, Quaternion&) noexcept;
[[nodiscard]] Vec3 rotate(Quaternion unitQuaternion, Vec3) noexcept;
[[nodiscard]] Status interpolatePose(const Pose3&, const Pose3&, double fraction, Pose3&) noexcept;
[[nodiscard]] Status relativeSource(const SpatialSource&, const SpatialListener&,
                                    RelativeSource&) noexcept;
// Row-major [listener * sources.size() + source]. Whole-batch preflight.
[[nodiscard]] Status relativeSources(std::span<const SpatialSource>,
                                     std::span<const SpatialListener>,
                                     std::span<RelativeSource>) noexcept;
struct AcousticMedium {
    double speedMetresPerSecond = 343, densityKgPerCubicMetre = 1.2;
    std::array<double, 3> absorptionDbPerMetre{0, 0, 0}; // Representative 250/2000/8000 Hz.
};
[[nodiscard]] bool validMedium(const AcousticMedium&) noexcept;
[[nodiscard]] Status atmosphericAbsorption(double frequencyHz, double temperatureCelsius,
                                           double relativeHumidityPercent, double& dbPerMetre,
                                           double pressurePascals = 101325) noexcept;
[[nodiscard]] Status airMedium(double temperatureCelsius, double relativeHumidityPercent,
                               AcousticMedium&, double pressurePascals = 101325) noexcept;
// Explicit reference preset; water temperature/salinity/absorption remain host data.
[[nodiscard]] AcousticMedium waterMedium() noexcept;
struct MediumSegment {
    double lengthMetres = 0;
    AcousticMedium medium;
};
struct AcousticTravel {
    double seconds = 0;
    std::array<double, 3> bandGains{1, 1, 1};
};
[[nodiscard]] Status acousticTravel(std::span<const MediumSegment>, AcousticTravel&) noexcept;
enum class DistanceModel { inverse, exponential, linear };
struct DistanceAttenuation {
    DistanceModel model = DistanceModel::inverse;
    double referenceMetres = 1, maximumMetres = 100, rolloff = 1;
    bool clampDistance = true;
};
[[nodiscard]] Status distanceGain(double distanceMetres, const DistanceAttenuation&,
                                  double& gain) noexcept;
[[nodiscard]] Status dopplerRatio(const RelativeSource&, const AcousticMedium&, double& ratio,
                                  double intensity = 1, double maximumSemitones = 24) noexcept;
enum class DirectivityPattern {
    omni,
    subcardioid,
    cardioid,
    supercardioid,
    hypercardioid,
    figureEight,
    custom
};
struct Directivity {
    DirectivityPattern pattern = DirectivityPattern::omni;
    double customOmniWeight = .5, amount = 1, minimumGainDb = -96;
    std::array<double, 3> rearLossDb{0, 0, 0};
    bool preservePolarity = false;
};
[[nodiscard]] Status directivityGains(double forwardCosine, const Directivity&,
                                      std::array<double, 3>&) noexcept;
struct AcousticMaterial {
    std::array<double, 3> absorption{.1, .2, .3}, transmissionLossDb{20, 30, 40};
    double referenceThicknessMetres = .2, scattering = .2;
};
struct MaterialResponse {
    std::array<double, 3> transmission{}, reflection{};
};
[[nodiscard]] Status materialResponse(const AcousticMaterial&, double thicknessMetres,
                                      MaterialResponse&) noexcept;
struct PropagationResult {
    RelativeSource relative;
    AcousticTravel travel;
    double doppler = 1;
};
[[nodiscard]] Status evaluatePropagation(const SpatialSource&, const SpatialListener&,
                                         const AcousticMedium&, const DistanceAttenuation&,
                                         const Directivity&, PropagationResult&) noexcept;
} // namespace cydsp
