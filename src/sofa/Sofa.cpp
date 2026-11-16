#include "sofa/SofaDetail.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>
#include <numbers>
namespace cydsp {
namespace {
using namespace sofa_detail;
std::string clean(std::string s) {
    std::string out;
    for (unsigned char c : s)
        if (c > 32)
            out += char(std::tolower(c));
    return out;
}
const Array& need(const Raw& r, const char* name) {
    const auto it = r.arrays.find(name);
    if (it == r.arrays.end())
        fail(SofaError::invalid_dimensions);
    return it->second;
}
double magnitude(Vec3 v) {
    return std::hypot(v.x, v.y, v.z);
}
Vec3 scaled(Vec3 v, double a) {
    return {v.x * a, v.y * a, v.z * a};
}
Vec3 minus(Vec3 a, Vec3 b) {
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}
double dot3(Vec3 a, Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
Vec3 cross3(Vec3 a, Vec3 b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
Vec3 normalized(Vec3 v) {
    const double n = magnitude(v);
    if (!isFinite(n) || n < 1e-12)
        fail(SofaError::invalid_coordinates);
    return scaled(v, 1 / n);
}
double lengthUnit(std::string s) {
    s = clean(s);
    if (s == "metre" || s == "meter" || s == "metres" || s == "meters" || s == "m")
        return 1;
    if (s == "centimetre" || s == "centimeter" || s == "cm")
        return .01;
    if (s == "millimetre" || s == "millimeter" || s == "mm")
        return .001;
    fail(SofaError::invalid_coordinates);
}
Vec3 convert(Vec3 v, const Array& a, bool orientation = false) {
    if (!isFinite(v.x) || !isFinite(v.y) || !isFinite(v.z))
        fail(SofaError::invalid_coordinates);
    if (clean(a.type) == "cartesian" || (orientation && a.type.empty()))
        return scaled(v, orientation && a.units.empty() ? 1 : lengthUnit(a.units));
    if (clean(a.type) != "spherical")
        fail(SofaError::invalid_coordinates);
    const auto units = clean(a.units);
    const auto comma = units.find(','), last = units.rfind(',');
    if (comma == std::string::npos || last == comma)
        fail(SofaError::invalid_coordinates);
    const auto az = units.substr(0, comma), el = units.substr(comma + 1, last - comma - 1);
    double angle = 1;
    if ((az == "degree" || az == "degrees") && (el == "degree" || el == "degrees"))
        angle = std::numbers::pi / 180;
    else if (!((az == "radian" || az == "radians") && (el == "radian" || el == "radians")))
        fail(SofaError::invalid_coordinates);
    if (v.z < 0)
        fail(SofaError::invalid_coordinates);
    const double radius = v.z * lengthUnit(units.substr(last + 1));
    return {radius * std::cos(v.y * angle) * std::cos(v.x * angle),
            radius * std::cos(v.y * angle) * std::sin(v.x * angle), radius * std::sin(v.y * angle)};
}
Vec3 position(const Raw& r, const char* name, std::size_t m, std::size_t count, Vec3 fallback,
              bool required = false) {
    const auto it = r.arrays.find(name);
    if (it == r.arrays.end()) {
        if (required)
            fail(SofaError::invalid_dimensions);
        return fallback;
    }
    const auto& a = it->second;
    const bool orientation =
        std::string_view(name) == "ListenerUp" || std::string_view(name) == "ListenerView";
    if (a.shape.size() == 3 && a.shape[0] == 1 && a.shape[1] == 3 &&
        (a.shape[2] == 1 || a.shape[2] == count)) {
        const auto stride = a.shape[2], i = stride == 1 ? 0 : m;
        return convert({a.values[i], a.values[stride + i], a.values[2 * stride + i]}, a,
                       orientation);
    }
    if (a.shape.size() != 2 || a.shape[1] != 3 || (a.shape[0] != 1 && a.shape[0] != count))
        fail(SofaError::invalid_dimensions);
    const auto i = (a.shape[0] == 1 ? 0 : m) * 3;
    return convert({a.values[i], a.values[i + 1], a.values[i + 2]}, a, orientation);
}
Vec3 receiver(const Array& a, std::size_t r, std::size_t m, std::size_t count) {
    if (a.shape == std::vector<std::size_t>{2, 3}) {
        const auto i = r * 3;
        return convert({a.values[i], a.values[i + 1], a.values[i + 2]}, a);
    }
    if (a.shape.size() != 3 || a.shape[0] != 2 || a.shape[1] != 3 ||
        (a.shape[2] != 1 && a.shape[2] != count))
        fail(SofaError::invalid_dimensions);
    const auto stride = a.shape[2], i = m * (stride != 1);
    return convert({a.values[(r * 3) * stride + i], a.values[(r * 3 + 1) * stride + i],
                    a.values[(r * 3 + 2) * stride + i]},
                   a);
}
template <Sample T>
Status load(std::span<const std::byte> bytes, BasicSofaHrtf<T>& out, const SofaLoadOptions& o,
            SofaLoadReport* report) noexcept {
    SofaLoadReport result;
    try {
        if (o.backend != SofaBackend::automatic && o.backend != SofaBackend::compact &&
            o.backend != SofaBackend::complete)
            fail(SofaError::backend_unavailable);
        if (bytes.empty() || bytes.size() > o.maximumFileBytes || !o.maximumPoints ||
            !o.maximumTaps || !o.maximumDecodedBytes || !o.maximumMetadataBytes)
            fail(SofaError::limit_exceeded);
        result.backend = o.backend == SofaBackend::automatic
                             ? (sofaBackendAvailable(SofaBackend::complete) ? SofaBackend::complete
                                                                            : SofaBackend::compact)
                             : o.backend;
        if (!sofaBackendAvailable(result.backend))
            fail(SofaError::backend_unavailable);
        if constexpr (std::same_as<T, double>) {
            if (result.backend == SofaBackend::compact && !o.allowPrecisionLoss)
                fail(SofaError::precision_loss);
        }
        Raw raw;
        if (result.backend == SofaBackend::complete) {
#if CYDSP_SOFA_COMPLETE
            raw = readComplete(bytes, o);
#endif
        } else {
#if CYDSP_SOFA_COMPACT
            raw = readCompact(bytes, o);
#endif
        }
        Budget metadata{o.maximumMetadataBytes};
        for (const auto& [key, value] : raw.attributes)
            metadata.consume(value.size());
        for (const auto& [key, a] : raw.arrays) {
            metadata.consume(a.type.size());
            metadata.consume(a.units.size());
        }
        if (raw.attributes["Conventions"] != "SOFA" || raw.attributes["DataType"] != "FIR")
            fail(SofaError::unsupported_convention);
        const auto convention = raw.attributes["SOFAConventions"];
        if (convention != "SimpleFreeFieldHRIR" &&
            !(o.allowGeneralFir && convention == "GeneralFIR"))
            fail(SofaError::unsupported_convention);
        const auto& ir = need(raw, "Data.IR");
        if (ir.shape.size() != 3 || ir.shape[1] != 2 || !ir.shape[0] || !ir.shape[2])
            fail(SofaError::invalid_dimensions);
        BasicSofaHrtf<T> next;
        next.points = ir.shape[0];
        next.taps = ir.shape[2];
        if (next.points > o.maximumPoints || next.taps > o.maximumTaps)
            fail(SofaError::limit_exceeded);
        const auto count = product(next.points, next.taps, o.maximumDecodedBytes / (sizeof(T) * 2));
        Budget outputBudget{o.maximumDecodedBytes};
        outputBudget.consume(count * sizeof(T) * 2);
        outputBudget.consume(
            product(next.points, sizeof(Vec3) + 3 * sizeof(double), o.maximumDecodedBytes));
        for (const auto& [key, value] : raw.attributes)
            outputBudget.consume(key.size() + value.size());
        const auto& rate = need(raw, "Data.SamplingRate");
        if (rate.shape.size() != 1 || rate.values.empty() ||
            (rate.values.size() != 1 && rate.values.size() != next.points) ||
            clean(rate.units) != "hertz")
            fail(SofaError::invalid_sampling_rate);
        next.sampleRate = rate.values[0];
        if (!isFinite(next.sampleRate) || next.sampleRate < 8000 || next.sampleRate > 768000)
            fail(SofaError::invalid_sampling_rate);
        for (double v : rate.values)
            if (!isFinite(v) || std::abs(v - next.sampleRate) > next.sampleRate * 1e-12)
                fail(SofaError::invalid_sampling_rate);
        const auto& delay = need(raw, "Data.Delay");
        const auto delayUnit = clean(delay.units);
        if (!delayUnit.empty() && delayUnit != "sample" && delayUnit != "samples" &&
            delayUnit != "frame" && delayUnit != "frames")
            fail(SofaError::invalid_delay);
        if (delay.shape != std::vector<std::size_t>{1, 2} &&
            delay.shape != std::vector<std::size_t>{next.points, 2})
            fail(SofaError::invalid_dimensions);
        const auto& rec = need(raw, "ReceiverPosition");
        next.left.resize(count);
        next.right.resize(count);
        next.directions.reserve(next.points);
        next.distancesMetres.reserve(next.points);
        next.leftDelay.reserve(next.points);
        next.rightDelay.reserve(next.points);
        for (std::size_t m = 0; m < next.points; ++m) {
            const auto origin = position(raw, "ListenerPosition", m, next.points, {});
            const auto front = normalized(position(raw, "ListenerView", m, next.points, {1, 0, 0}));
            const auto up = normalized(position(raw, "ListenerUp", m, next.points, {0, 0, 1}));
            const auto leftAxis = normalized(cross3(up, front)), upAxis = cross3(front, leftAxis);
            const auto source = position(raw, "SourcePosition", m, next.points, {}, true);
            const auto emitter = position(raw, "EmitterPosition", m, next.points, {});
            if (magnitude(emitter) > 1e-9)
                fail(SofaError::invalid_coordinates);
            const auto delta = minus(source, origin);
            const double distance = magnitude(delta);
            const auto dir =
                normalized({dot3(delta, front), dot3(delta, leftAxis), dot3(delta, upAxis)});
            next.directions.push_back(dir);
            next.distancesMetres.push_back(distance);
            const auto a = receiver(rec, 0, m, next.points), b = receiver(rec, 1, m, next.points);
            if (!(std::max(a.y, b.y) > 1e-8 && std::min(a.y, b.y) < -1e-8))
                fail(SofaError::invalid_coordinates);
            const std::size_t l = a.y > b.y ? 0 : 1, r = 1 - l;
            result.receiversSwapped = result.receiversSwapped || l != 0;
            const auto di = (delay.shape[0] == 1 ? 0 : m) * 2;
            const double dl = delay.values[di + l], dr = delay.values[di + r];
            if (!isFinite(dl) || !isFinite(dr) || dl < 0 || dr < 0)
                fail(SofaError::invalid_delay);
            next.leftDelay.push_back(dl);
            next.rightDelay.push_back(dr);
            for (std::size_t t = 0; t < next.taps; ++t) {
                const double lv = ir.values[(m * 2 + l) * next.taps + t],
                             rv = ir.values[(m * 2 + r) * next.taps + t];
                if (!isFinite(lv) || !isFinite(rv) ||
                    std::abs(lv) > std::numeric_limits<T>::max() ||
                    std::abs(rv) > std::numeric_limits<T>::max())
                    fail(SofaError::invalid_samples);
                next.left[m * next.taps + t] = T(lv);
                next.right[m * next.taps + t] = T(rv);
            }
        }
        next.convention = convention;
        next.title = raw.attributes["Title"];
        next.database = raw.attributes["DatabaseName"];
        next.listener = raw.attributes["ListenerShortName"];
        next.license = raw.attributes["License"];
        result.sourcePrecisionBits = ir.precision;
        result.precisionReduced = raw.reduced || (ir.precision > 32 && std::same_as<T, float>);
        out = std::move(next);
        if (report)
            *report = result;
        return Status::ok;
    } catch (const Failure& f) {
        result.error = f.error;
    } catch (const std::bad_alloc&) {
        result.error = SofaError::allocation_failed;
    } catch (...) {
        result.error = SofaError::invalid_file;
    }
    if (report)
        *report = result;
    return status(result.error);
}
} // namespace
bool sofaBackendAvailable(SofaBackend b) noexcept {
    if (b == SofaBackend::compact)
        return CYDSP_SOFA_COMPACT != 0;
    if (b == SofaBackend::complete)
        return CYDSP_SOFA_COMPLETE != 0;
    return b == SofaBackend::automatic && (CYDSP_SOFA_COMPACT || CYDSP_SOFA_COMPLETE);
}
Status loadSofa(std::span<const std::byte> bytes, SofaHrtf& out, const SofaLoadOptions& o,
                SofaLoadReport* r) noexcept {
    return load(bytes, out, o, r);
}
Status loadSofa(std::span<const std::byte> bytes, SofaHrtf64& out, const SofaLoadOptions& o,
                SofaLoadReport* r) noexcept {
    return load(bytes, out, o, r);
}
} // namespace cydsp
