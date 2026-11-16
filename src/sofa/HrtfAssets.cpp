#include "cydsp/processors/AdvancedFilterDesign.h"
#include "cydsp/processors/RealFft.h"
#include "sofa/SofaDetail.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <limits>
#include <numbers>
namespace cydsp {
namespace {
using namespace sofa_detail;
template <Sample T> bool validAsset(const BasicSofaHrtf<T>& a, std::size_t limit) {
    if (!a.points || !a.taps || !isFinite(a.sampleRate) || a.sampleRate < 8000 ||
        a.sampleRate > 768000 || a.points > limit / a.taps || a.left.size() != a.points * a.taps ||
        a.right.size() != a.left.size() || a.directions.size() != a.points ||
        a.distancesMetres.size() != a.points || a.leftDelay.size() != a.points ||
        a.rightDelay.size() != a.points)
        return false;
    for (std::size_t i = 0; i < a.points; ++i) {
        auto d = a.directions[i];
        const double n = std::hypot(d.x, d.y, d.z);
        if (!isFinite(n) || n < 1e-12 || !isFinite(a.distancesMetres[i]) ||
            a.distancesMetres[i] <= 0 || !isFinite(a.leftDelay[i]) || !isFinite(a.rightDelay[i]) ||
            a.leftDelay[i] < 0 || a.rightDelay[i] < 0)
            return false;
    }
    for (auto v : a.left)
        if (!isFinite(v))
            return false;
    for (auto v : a.right)
        if (!isFinite(v))
            return false;
    return true;
}
void checked(Status s) {
    if (s != Status::ok)
        throw s;
}
template <Sample T>
Status prepare(const BasicSofaHrtf<T>& a, HrtfSet<T>& out, const SofaPreparation& o,
               SofaPreparationReport* report) noexcept {
    try {
        if (!validAsset(a, o.hrtf.maximumStorageBytes / (2 * sizeof(T))) ||
            !isFinite(o.distanceMetres) || o.distanceMetres <= 0 ||
            !isFinite(o.distanceToleranceMetres) || o.distanceToleranceMetres < 0 ||
            !isFinite(o.maximumEqualizationDb) || o.maximumEqualizationDb < 0 ||
            o.maximumEqualizationDb > 36 ||
            unsigned(o.normalization) > unsigned(HrtfLevelNormalization::frontalEnergy))
            return Status::invalid_argument;
        if (!o.diffuseFieldWeights.empty() && o.diffuseFieldWeights.size() != a.points)
            return Status::invalid_argument;
        std::vector<std::size_t> selected;
        double radius = a.distancesMetres.front();
        for (double d : a.distancesMetres)
            if (std::abs(d - o.distanceMetres) < std::abs(radius - o.distanceMetres))
                radius = d;
        for (std::size_t i = 0; i < a.points; ++i)
            if (std::abs(a.distancesMetres[i] - radius) <= o.distanceToleranceMetres)
                selected.push_back(i);
        if (selected.empty() || selected.size() > o.hrtf.maximumPoints)
            return Status::invalid_spec;
        std::size_t taps = a.taps;
        std::vector<T> left, right;
        SofaPreparationReport r;
        r.selectedPoints = selected.size();
        r.distanceMetres = radius;
        r.diffuseFieldEqualized = o.diffuseFieldEqualization;
        r.uniformDiffuseWeights = o.diffuseFieldEqualization && o.diffuseFieldWeights.empty();
        if (o.diffuseFieldEqualization) {
            if (o.equalizationTaps < 3 || o.equalizationTaps > 8193 || !(o.equalizationTaps & 1))
                return Status::invalid_argument;
            taps = a.taps + o.equalizationTaps - 1;
            if (taps > o.hrtf.maximumInputTaps)
                return Status::invalid_spec;
            const auto count =
                product(selected.size(), taps, o.hrtf.maximumStorageBytes / (2 * sizeof(T)));
            left.resize(count);
            right.resize(count);
            std::size_t n = 2;
            while (n < 2 * taps)
                n *= 2;
            RealFft<double> fft;
            checked(fft.prepare(n));
            std::vector<double> time(n), power(fft.bins()), prefix(fft.bins() + 1),
                linear(o.equalizationTaps), eq(o.equalizationTaps);
            std::vector<std::complex<double>> spectrum(fft.bins()), eqSpectrum(fft.bins());
            double total = 0;
            for (auto i : selected) {
                const double w = o.diffuseFieldWeights.empty() ? 1 : o.diffuseFieldWeights[i];
                if (!isFinite(w) || w < 0)
                    return Status::invalid_argument;
                total += w;
                for (const auto* ear : {&a.left, &a.right}) {
                    std::fill(time.begin(), time.end(), 0.);
                    std::copy_n(ear->begin() + i * a.taps, a.taps, time.begin());
                    checked(fft.forward(time, spectrum));
                    for (std::size_t k = 0; k < power.size(); ++k)
                        power[k] += w * .5 * std::norm(spectrum[k]);
                }
            }
            if (!isFinite(total) || total <= 0)
                return Status::invalid_argument;
            const double peak = *std::max_element(power.begin(), power.end()) / total;
            if (peak <= 0)
                return Status::invalid_argument;
            for (std::size_t k = 0; k < power.size(); ++k)
                prefix[k + 1] = prefix[k] + std::log(std::max(power[k] / total, peak * 1e-12));
            for (std::size_t k = 0; k < power.size(); ++k) {
                // One-third-octave smoothing of log power and bounded inversion.
                const auto lo = std::size_t(double(k) / std::exp2(1. / 6));
                const auto hi = std::min(
                    power.size(), std::max(lo + 1, std::size_t(double(k) * std::exp2(1. / 6)) + 1));
                const double logPower = (prefix[hi] - prefix[lo]) / double(hi - lo);
                const double db = std::clamp(-10 / std::log(10.) * logPower,
                                             -o.maximumEqualizationDb, o.maximumEqualizationDb);
                spectrum[k] = {std::pow(10., db / 20), 0};
            }
            checked(fft.inverse(spectrum, time));
            const auto mid = o.equalizationTaps / 2;
            for (std::size_t j = 0; j < linear.size(); ++j) {
                const auto k = (j + n - mid) % n;
                linear[j] = time[k] * (.5 - .5 * std::cos(2 * std::numbers::pi * double(j) /
                                                          double(linear.size() - 1)));
            }
            checked(minimumPhaseFir(linear, eq));
            std::fill(time.begin(), time.end(), 0.);
            std::copy(eq.begin(), eq.end(), time.begin());
            checked(fft.forward(time, eqSpectrum));
            for (std::size_t p = 0; p < selected.size(); ++p) {
                for (unsigned ear = 0; ear < 2; ++ear) {
                    const auto& source = ear ? a.right : a.left;
                    auto& target = ear ? right : left;
                    std::fill(time.begin(), time.end(), 0.);
                    std::copy_n(source.begin() + selected[p] * a.taps, a.taps, time.begin());
                    checked(fft.forward(time, spectrum));
                    for (std::size_t k = 0; k < spectrum.size(); ++k)
                        spectrum[k] *= eqSpectrum[k];
                    checked(fft.inverse(spectrum, time));
                    for (std::size_t j = 0; j < taps; ++j)
                        target[p * taps + j] = T(time[j]);
                }
            }
        }
        double gain = 1;
        if (o.normalization == HrtfLevelNormalization::frontalEnergy) {
            std::size_t front = 0;
            double best = -2;
            for (std::size_t p = 0; p < selected.size(); ++p) {
                const auto d = a.directions[selected[p]];
                const double x = d.x / std::hypot(d.x, d.y, d.z);
                if (x > best) {
                    best = x;
                    front = p;
                }
            }
            double energy = 0;
            for (unsigned ear = 0; ear < 2; ++ear) {
                const auto& raw = ear ? a.right : a.left;
                const auto& processed = ear ? right : left;
                const auto* first = o.diffuseFieldEqualization
                                        ? processed.data() + front * taps
                                        : raw.data() + selected[front] * a.taps;
                for (std::size_t j = 0; j < taps; ++j)
                    energy += double(first[j]) * double(first[j]) * .5;
            }
            if (!isFinite(energy) || energy <= 1e-30)
                return Status::invalid_argument;
            gain = 1 / std::sqrt(energy);
        }
        if (gain != 1) {
            if (left.empty()) {
                const auto count =
                    product(selected.size(), taps, o.hrtf.maximumStorageBytes / (2 * sizeof(T)));
                left.resize(count);
                right.resize(count);
                for (std::size_t p = 0; p < selected.size(); ++p) {
                    std::copy_n(a.left.begin() + selected[p] * taps, taps, left.begin() + p * taps);
                    std::copy_n(a.right.begin() + selected[p] * taps, taps,
                                right.begin() + p * taps);
                }
            }
            for (auto& v : left)
                v = T(double(v) * gain);
            for (auto& v : right)
                v = T(double(v) * gain);
        }
        std::vector<HrirPoint<T>> points;
        points.reserve(selected.size());
        for (std::size_t p = 0; p < selected.size(); ++p) {
            const auto i = selected[p];
            const auto l = left.empty() ? std::span<const T>(a.left).subspan(i * taps, taps)
                                        : std::span<const T>(left).subspan(p * taps, taps);
            const auto rr = right.empty() ? std::span<const T>(a.right).subspan(i * taps, taps)
                                          : std::span<const T>(right).subspan(p * taps, taps);
            points.push_back({a.directions[i], l, rr, a.leftDelay[i], a.rightDelay[i]});
        }
        auto config = o.hrtf;
        config.sourceSampleRate = a.sampleRate;
        const auto s = out.prepare(points, config);
        if (s != Status::ok)
            return s;
        r.inputTaps = taps;
        r.preparedTaps = out.taps();
        r.truncated = out.truncated();
        r.appliedGain = gain;
        if (report)
            *report = r;
        return Status::ok;
    } catch (Status s) {
        return s;
    } catch (const Failure& f) {
        return status(f.error);
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (...) {
        return Status::invalid_argument;
    }
}
std::uint32_t crc(std::span<const std::byte> b) {
    std::uint32_t c = ~0u;
    for (auto v : b) {
        c ^= std::to_integer<unsigned>(v);
        for (unsigned i = 0; i < 8; ++i)
            c = (c >> 1) ^ (0xedb88320u & (0u - (c & 1)));
    }
    return ~c;
}
void put(std::vector<std::byte>& out, std::uint64_t n, unsigned bytes = 8) {
    for (unsigned i = 0; i < bytes; ++i)
        out.push_back(std::byte((n >> (8 * i)) & 255));
}
struct Cursor {
    std::span<const std::byte> b;
    std::size_t offset = 0;
    std::uint64_t get(unsigned n = 8) {
        if (n > b.size() - offset)
            fail(SofaError::invalid_file);
        std::uint64_t v = 0;
        for (unsigned i = 0; i < n; ++i)
            v |= std::uint64_t(std::to_integer<unsigned>(b[offset++])) << (8 * i);
        return v;
    }
    double real() {
        return std::bit_cast<double>(get());
    }
};
template <Sample T> Status encode(const BasicSofaHrtf<T>& a, std::vector<std::byte>& out) noexcept {
    try {
        if (!validAsset(a, std::numeric_limits<std::size_t>::max() / 16))
            return Status::invalid_argument;
        std::vector<std::byte> next;
        const char magic[] = "CYHRTF01";
        for (unsigned i = 0; i < 8; ++i)
            next.push_back(std::byte(magic[i]));
        put(next, sizeof(T), 4);
        put(next, 1, 4);
        put(next, a.points);
        put(next, a.taps);
        put(next, std::bit_cast<std::uint64_t>(a.sampleRate));
        for (const auto* s : {&a.convention, &a.title, &a.database, &a.listener, &a.license}) {
            put(next, s->size());
            for (unsigned char c : *s)
                next.push_back(std::byte(c));
        }
        for (std::size_t i = 0; i < a.points; ++i)
            for (double v : {a.directions[i].x, a.directions[i].y, a.directions[i].z,
                             a.distancesMetres[i], a.leftDelay[i], a.rightDelay[i]})
                put(next, std::bit_cast<std::uint64_t>(v));
        for (const auto* ear : {&a.left, &a.right})
            for (T v : *ear) {
                if constexpr (sizeof(T) == 4)
                    put(next, std::bit_cast<std::uint32_t>(v), 4);
                else
                    put(next, std::bit_cast<std::uint64_t>(v));
            }
        put(next, crc(next), 4);
        out = std::move(next);
        return Status::ok;
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (...) {
        return Status::invalid_argument;
    }
}
template <Sample T>
Status decode(std::span<const std::byte> bytes, BasicSofaHrtf<T>& out,
              const SofaLoadOptions& o) noexcept {
    try {
        if (bytes.size() < 84 || bytes.size() > o.maximumFileBytes ||
            std::memcmp(bytes.data(), "CYHRTF01", 8))
            return Status::invalid_argument;
        Cursor checksum{bytes.last(4)};
        if (checksum.get(4) != crc(bytes.first(bytes.size() - 4)))
            return Status::invalid_argument;
        Cursor c{bytes.first(bytes.size() - 4)};
        c.offset = 8;
        const auto size = c.get(4);
        if (size != 4 && size != 8)
            return Status::unsupported_format;
        if (size > sizeof(T) && !o.allowPrecisionLoss)
            return Status::unsupported_format;
        if (c.get(4) != 1)
            return Status::incompatible_version;
        const auto points = c.get(), taps = c.get();
        if (points > SIZE_MAX || taps > SIZE_MAX)
            return Status::invalid_argument;
        BasicSofaHrtf<T> a;
        a.points = std::size_t(points);
        a.taps = std::size_t(taps);
        a.sampleRate = c.real();
        if (!a.points || !a.taps || a.points > o.maximumPoints || a.taps > o.maximumTaps)
            return Status::invalid_argument;
        const auto samples = product(a.points, a.taps, o.maximumDecodedBytes / (2 * sizeof(T)));
        Budget budget{o.maximumDecodedBytes};
        budget.consume(product(samples, 2 * sizeof(T), o.maximumDecodedBytes));
        budget.consume(product(a.points, 48, o.maximumDecodedBytes));
        for (auto* s : {&a.convention, &a.title, &a.database, &a.listener, &a.license}) {
            const auto n = c.get();
            if (n > o.maximumMetadataBytes || n > c.b.size() - c.offset)
                fail(SofaError::limit_exceeded);
            budget.consume(std::size_t(n));
            s->assign(reinterpret_cast<const char*>(c.b.data() + c.offset), std::size_t(n));
            c.offset += std::size_t(n);
        }
        const auto encoded = product(samples, 2 * std::size_t(size), bytes.size());
        const auto coordinates = product(a.points, 48, bytes.size());
        if (coordinates > c.b.size() - c.offset || encoded != c.b.size() - c.offset - coordinates)
            return Status::invalid_argument;
        a.directions.resize(a.points);
        a.distancesMetres.resize(a.points);
        a.leftDelay.resize(a.points);
        a.rightDelay.resize(a.points);
        for (std::size_t i = 0; i < a.points; ++i) {
            a.directions[i] = {c.real(), c.real(), c.real()};
            a.distancesMetres[i] = c.real();
            a.leftDelay[i] = c.real();
            a.rightDelay[i] = c.real();
        }
        for (auto* ear : {&a.left, &a.right}) {
            ear->resize(samples);
            for (auto& v : *ear) {
                const double x =
                    size == 4 ? double(std::bit_cast<float>(std::uint32_t(c.get(4)))) : c.real();
                if (!isFinite(x) || std::abs(x) > std::numeric_limits<T>::max())
                    return Status::invalid_argument;
                v = T(x);
            }
        }
        if (!validAsset(a, samples))
            return Status::invalid_argument;
        out = std::move(a);
        return Status::ok;
    } catch (const Failure& f) {
        return status(f.error);
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (...) {
        return Status::invalid_argument;
    }
}
} // namespace
Status prepareSofaHrtf(const SofaHrtf& a, HrtfSet<float>& b, const SofaPreparation& o,
                       SofaPreparationReport* r) noexcept {
    return prepare(a, b, o, r);
}
Status prepareSofaHrtf(const SofaHrtf64& a, HrtfSet<double>& b, const SofaPreparation& o,
                       SofaPreparationReport* r) noexcept {
    return prepare(a, b, o, r);
}
Status encodeHrtfAsset(const SofaHrtf& a, std::vector<std::byte>& b) noexcept {
    return encode(a, b);
}
Status encodeHrtfAsset(const SofaHrtf64& a, std::vector<std::byte>& b) noexcept {
    return encode(a, b);
}
Status decodeHrtfAsset(std::span<const std::byte> b, SofaHrtf& a,
                       const SofaLoadOptions& o) noexcept {
    return decode(b, a, o);
}
Status decodeHrtfAsset(std::span<const std::byte> b, SofaHrtf64& a,
                       const SofaLoadOptions& o) noexcept {
    return decode(b, a, o);
}
} // namespace cydsp
