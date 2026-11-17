#include <algorithm>
#include <array>
#include <cmath>
#include <cydsp/core/AudioBuffer.h>
#include <cydsp/spatial/Sofa.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
void require(bool value, const char* message) {
    if (!value)
        throw std::runtime_error(message);
}
void ok(cydsp::Status status, const char* message) {
    if (status != cydsp::Status::ok)
        throw std::runtime_error(std::string(message) + ": status " + std::to_string(int(status)));
}
std::vector<std::byte> read(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    require(bool(stream), "Missing measured SOFA");
    const auto size = stream.tellg();
    require(size > 0 && size < 100000000, "Unexpected measured file size");
    std::vector<std::byte> bytes(static_cast<std::size_t>(size));
    stream.seekg(0);
    stream.read(reinterpret_cast<char*>(bytes.data()), size);
    require(bool(stream), "Incomplete measured SOFA read");
    return bytes;
}
void check(const char* filename, std::size_t expectedPoints) {
    using namespace cydsp;
    const auto bytes = read(std::filesystem::path(CYDSP_HRTF_DATA_DIR) / filename);
    SofaHrtf reference;
    bool haveReference = false;
    for (auto backend : {SofaBackend::compact, SofaBackend::complete}) {
        if (!sofaBackendAvailable(backend))
            continue;
        SofaLoadOptions options;
        options.backend = backend;
        options.allowPrecisionLoss = true;
        SofaHrtf data;
        SofaLoadReport report;
        ok(loadSofa(bytes, data, options, &report), "Measured SOFA import");
        require(data.points == expectedPoints && data.taps == 256 && data.sampleRate == 48000,
                "Measured SOFA dimensions");
        require(data.convention == "SimpleFreeFieldHRIR" &&
                    data.license.find("Apache License, Version 2.0") != std::string::npos,
                "Measured SOFA rights/convention");
        if (haveReference) {
            require(data.left == reference.left && data.right == reference.right,
                    "Readers disagree on float32 samples");
            require(data.leftDelay == reference.leftDelay &&
                        data.rightDelay == reference.rightDelay,
                    "Readers disagree on arrival delays");
        }
        HrtfSet<float> bank;
        HrtfPreparation config;
        ok(hrtfPreparation(HrtfQuality::realtimeHigh, 48000, config), "Realtime HRTF preset");
        config.detectOnsets = false;
        std::vector<HrirPoint<float>> points(data.points);
        for (std::size_t i = 0; i < data.points; ++i)
            points[i] = {data.directions[i],
                         std::span<const float>(data.left).subspan(i * data.taps, data.taps),
                         std::span<const float>(data.right).subspan(i * data.taps, data.taps),
                         data.leftDelay[i], data.rightDelay[i]};
        ok(bank.prepare(points, config), "Prepare complete measured direction bank");
        require(!bank.truncated() && bank.points() == data.points && bank.taps() == 256,
                "Measured bank must retain all points/taps");
        HrirBinaural<float> renderer;
        ok(renderer.prepare({48000, 512, 1}, bank), "Prepare measured renderer");
        AudioBuffer<float> input, output;
        ok(input.resize(1, 512), "Measured input buffer");
        ok(output.resize(2, 512), "Measured output buffer");
        for (auto index : {std::size_t(0), data.points / 3, data.points - 1}) {
            require(data.leftDelay[index] == 0 && data.rightDelay[index] == 0,
                    "Unexpected source delay in original dataset");
            renderer.reset();
            ok(renderer.setDirection(data.directions[index], 0), "Exact measured direction");
            std::fill(input.block().channel(0).begin(), input.block().channel(0).end(), 0.f);
            input.block().channel(0)[0] = 1.f;
            ok(renderer.process(input.block(), output.block()), "Measured impulse rendering");
            const auto offset = renderer.latencyFrames();
            require(offset + data.taps < output.block().frames(), "Impulse buffer too short");
            for (std::size_t ear = 0; ear < 2; ++ear) {
                const auto& samples = ear == 0 ? data.left : data.right;
                for (std::size_t n = 0; n < data.taps; ++n) {
                    const double target = samples[index * data.taps + n];
                    const double actual = output.block().channel(ear)[offset + n];
                    require(std::isfinite(actual) &&
                                std::abs(actual - target) < 8e-7 * (1 + std::abs(target)),
                            "Rendered measured HRIR mismatch");
                }
            }
            ok(renderer.setDirection({.37, .61, .7}, 128), "Moving measured direction");
            ok(renderer.process(input.block(128), output.block(128)), "Moving measured render");
            for (std::size_t ear = 0; ear < 2; ++ear)
                for (auto value : output.block(128).channel(ear))
                    require(std::isfinite(value), "Nonfinite moving measured render");
        }
        // The importer must forward the optional timing policy to HrtfSet.
        config.detectOnsets = true;
        HrtfSet<float> preserved, advanced;
        ok(preserved.prepare(points, config), "Preserve detected measured onsets");
        SofaPreparation alignedOptions;
        alignedOptions.hrtf = config;
        alignedOptions.hrtf.removeCommonDelay = true;
        ok(prepareSofaHrtf(data, advanced, alignedOptions), "SOFA common delay removal");
        double expectedCommon = std::numeric_limits<double>::infinity();
        for (const auto& point : points) {
            const std::array<std::span<const float>, 2> ears{point.left, point.right};
            const std::array<double, 2> explicitDelay{point.leftDelayFrames,
                                                      point.rightDelayFrames};
            for (std::size_t e = 0; e < 2; ++e) {
                double peak = 0;
                for (float sample : ears[e])
                    peak = std::max(peak, std::abs(double(sample)));
                std::size_t onset = 0;
                if (peak > 0)
                    while (onset + 1 < ears[e].size() &&
                           std::abs(double(ears[e][onset])) < .01 * peak)
                        ++onset;
                expectedCommon = std::min(expectedCommon, explicitDelay[e] + double(onset));
            }
        }
        require(advanced.removedCommonDelayFrames() == expectedCommon &&
                    advanced.points() == preserved.points() && advanced.taps() == preserved.taps(),
                "Measured earliest common arrival reported");
        std::vector<float> oldL(preserved.taps()), oldR(oldL.size()), newL(oldL.size()),
            newR(oldL.size());
        for (auto index : {std::size_t(0), data.points / 3, data.points - 1}) {
            std::array<double, 2> oldDelay{}, newDelay{};
            ok(preserved.interpolate(data.directions[index], oldL, oldR, oldDelay),
               "Measured old timing");
            ok(advanced.interpolate(data.directions[index], newL, newR, newDelay),
               "Measured new timing");
            require(oldL == newL && oldR == newR,
                    "SOFA timing option leaves FIR coefficients intact");
            for (std::size_t ear = 0; ear < 2; ++ear)
                require(std::abs(oldDelay[ear] - newDelay[ear] - expectedCommon) < 1e-12,
                        "SOFA option preserves relative ear/direction timing");
        }
        std::cout << "PASS common arrival " << filename << ": " << expectedCommon << " frames\n";
        if (backend == SofaBackend::complete) {
            SofaHrtf64 precise;
            options.allowPrecisionLoss = false;
            ok(loadSofa(bytes, precise, options, &report), "Measured float64 import");
            require(!report.precisionReduced && report.sourcePrecisionBits == 64,
                    "Complete reader must preserve original float64");
            for (std::size_t i = 0; i < precise.left.size(); i += 257)
                require(float(precise.left[i]) == data.left[i] &&
                            float(precise.right[i]) == data.right[i],
                        "Float32 conversion disagrees with preserved float64");
        }
        if (!haveReference) {
            reference = std::move(data);
            haveReference = true;
        }
        std::cout << "PASS " << filename << " backend=" << int(backend)
                  << " points=" << expectedPoints << " taps=256\n";
    }
    require(haveReference, "No SOFA backend available for measured data");
}
} // namespace

int main() {
    try {
        check("D1_48K_24bit_256tap_FIR_SOFA.sofa", 8802);
        check("D2_48K_24bit_256tap_FIR_SOFA.sofa", 8802);
        check("H10_48K_24bit_256tap_FIR_SOFA.sofa", 2114);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL " << error.what() << '\n';
        return 1;
    }
}
