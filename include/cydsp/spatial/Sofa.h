#pragma once
#include "cydsp/spatial/SpatialRendering.h"
#include <cstddef>
#include <span>
#include <string>
#include <vector>
namespace cydsp {
// Import and preparation are NON real-time. Neither API reads files.
// Both readers and their dependencies belong only to the optional CyDSP::Sofa target.
enum class SofaBackend { automatic, compact, complete };
enum class SofaError {
    none,
    invalid_file,
    unsupported_convention,
    unsupported_storage,
    invalid_dimensions,
    invalid_coordinates,
    invalid_sampling_rate,
    invalid_delay,
    invalid_samples,
    limit_exceeded,
    precision_loss,
    backend_unavailable,
    allocation_failed
};
struct SofaLoadReport {
    SofaBackend backend = SofaBackend::automatic;
    SofaError error = SofaError::none;
    unsigned sourcePrecisionBits = 0;
    bool precisionReduced = false, receiversSwapped = false;
};
using SofaInflate = Status (*)(std::span<const std::byte> input, std::span<std::byte> output,
                               void* context) noexcept;
struct SofaLoadOptions {
    SofaBackend backend = SofaBackend::automatic;
    std::size_t maximumFileBytes = 128u * 1024u * 1024u;
    std::size_t maximumDecodedBytes = 512u * 1024u * 1024u;
    std::size_t maximumPoints = 100000, maximumTaps = 32768;
    std::size_t maximumMetadataBytes = 65536;
    bool allowGeneralFir = false, allowPrecisionLoss = false;
    SofaInflate inflate = nullptr; // compact reader: null uses built-in miniz
    void* inflateContext = nullptr;
};
template <Sample T> struct BasicSofaHrtf {
    double sampleRate = 0;
    std::vector<Vec3> directions;
    std::vector<double> distancesMetres;
    std::vector<T> left, right;                // point-major, points * taps
    std::vector<double> leftDelay, rightDelay; // source-rate frames
    std::size_t points = 0, taps = 0;
    std::string convention, title, database, listener, license;
};
using SofaHrtf = BasicSofaHrtf<float>;
using SofaHrtf64 = BasicSofaHrtf<double>;
// The compact reader has float precision. The complete reader preserves float64.
// Automatic prefers the complete reader when compiled. Failure preserves out.
[[nodiscard]] Status loadSofa(std::span<const std::byte> file, SofaHrtf& out,
                              const SofaLoadOptions& options = {},
                              SofaLoadReport* report = nullptr) noexcept;
[[nodiscard]] Status loadSofa(std::span<const std::byte> file, SofaHrtf64& out,
                              const SofaLoadOptions& options = {},
                              SofaLoadReport* report = nullptr) noexcept;
[[nodiscard]] bool sofaBackendAvailable(SofaBackend backend) noexcept;
enum class HrtfLevelNormalization { none, frontalEnergy };
struct SofaPreparationReport {
    std::size_t selectedPoints = 0, inputTaps = 0, preparedTaps = 0;
    double distanceMetres = 0, appliedGain = 1;
    bool truncated = false, diffuseFieldEqualized = false, uniformDiffuseWeights = false;
};
struct SofaPreparation {
    HrtfPreparation hrtf;
    double distanceMetres = 1, distanceToleranceMetres = .001;
    bool diffuseFieldEqualization = false;
    std::span<const double> diffuseFieldWeights; // original-point solid-angle weights
    std::size_t equalizationTaps = 129;
    double maximumEqualizationDb = 12;
    HrtfLevelNormalization normalization = HrtfLevelNormalization::none;
};
// Chooses one coherent distance shell, applies optional COMMON-ear EQ/gain,
// then calls HrtfSet. Configuration sourceSampleRate is taken from the asset.
// Allocates only during preparation. Failure preserves the previous HrtfSet.
[[nodiscard]] Status prepareSofaHrtf(const SofaHrtf& asset, HrtfSet<float>& out,
                                     const SofaPreparation& options = {},
                                     SofaPreparationReport* report = nullptr) noexcept;
[[nodiscard]] Status prepareSofaHrtf(const SofaHrtf64& asset, HrtfSet<double>& out,
                                     const SofaPreparation& options = {},
                                     SofaPreparationReport* report = nullptr) noexcept;
// Portable little-endian asset cache with checksum; no HDF/SOFA reader needed.
// Link CyDSP::HrtfAssets to read/write caches or prepare already-decoded assets.
[[nodiscard]] Status encodeHrtfAsset(const SofaHrtf& asset, std::vector<std::byte>& out) noexcept;
[[nodiscard]] Status encodeHrtfAsset(const SofaHrtf64& asset, std::vector<std::byte>& out) noexcept;
[[nodiscard]] Status decodeHrtfAsset(std::span<const std::byte> file, SofaHrtf& out,
                                     const SofaLoadOptions& options = {}) noexcept;
[[nodiscard]] Status decodeHrtfAsset(std::span<const std::byte> file, SofaHrtf64& out,
                                     const SofaLoadOptions& options = {}) noexcept;
} // namespace cydsp
