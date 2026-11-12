#include "TestPaths.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cydsp/CyDSP.h>
#include <fstream>
#include <iostream>
#include <numbers>
#include <source_location>
#include <stdexcept>
#include <thread>
#if defined(_WIN32)
// clang-format off
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <psapi.h>
// clang-format on
#endif
namespace {
using namespace cydsp;
void require(bool v, const char* why) {
    if (!v)
        throw std::runtime_error(why);
}
void ok(Status r, const std::source_location at = std::source_location::current()) {
    if (r != Status::ok)
        throw std::runtime_error("PCM proxy status " + std::to_string(unsigned(r)) + " line " +
                                 std::to_string(at.line()));
}
void test() {
    const auto folder = cydsp::testSupport::temporaryDirectory();
    const auto prefix = "cydsp-proxy-" +
                        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    const auto source = folder / (prefix + ".wav"), cache = folder / (prefix + "-cache.wav");
    struct Cleanup {
        std::filesystem::path a, b;
        ~Cleanup() {
            std::error_code ec;
            std::filesystem::remove(a, ec);
            std::filesystem::remove(b, ec);
        }
    } cleanup{source, cache};
    AudioFileInfo info;
    info.sampleRate = 48000;
    info.channels = 2;
    info.encoding = AudioEncoding::float64;
    AudioBuffer<double> block;
    ok(block.resize(2, 512));
    AudioFileWriter writer;
    ok(writer.open(source, AudioFileFormat::wav, info, {}, 512));
    constexpr std::size_t total = 512 * 256;
    for (std::size_t at = 0; at < total; at += 512) {
        for (std::size_t i = 0; i < 512; ++i)
            for (std::size_t c = 0; c < 2; ++c)
                block.block().channel(c)[i] = std::sin(double(at + i) * .013 + double(c));
        ok(writer.write(block.block()));
    }
    ok(writer.close());
    EditingPcmProxy proxy;
    PcmProxyOptions options;
    options.blockFrames = 512;
    PcmProxyOptions tiny = options;
    tiny.maximumWorkingBufferBytes = 8;
    require(proxy.build(source, cache, tiny) == Status::block_too_large &&
                !std::filesystem::exists(cache),
            "copy-buffer budget");
    ok(proxy.build(source, cache, options));
    require(proxy.sourceInfo()->frames == total &&
                proxy.pcmInfo()->encoding == AudioEncoding::float64,
            "proxy shape/precision");
    require(proxy.statistics().workingBufferBytes == 8192 + 2 * sizeof(double*) &&
                proxy.statistics().framesCopied == total,
            "bounded proxy scratch");
    auto start = std::chrono::steady_clock::now();
    std::size_t n = 0;
    ok(proxy.read(total - 512, block.block(), n));
    const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(
                             std::chrono::steady_clock::now() - start)
                             .count();
    for (std::size_t i = 0; i < 512; ++i)
        for (std::size_t c = 0; c < 2; ++c)
            require(block.block().channel(c)[i] ==
                        std::sin(double(total - 512 + i) * .013 + double(c)),
                    "exact direct end seek");
    require(proxy.statistics().framesRead == 512 && proxy.statistics().readCalls == 1,
            "seek reads one block, no prefix replay");
    EditingPcmProxy reopened;
    ok(reopened.open(source, cache, 512));
    ok(reopened.read(12345, block.block(), n));
    require(block.block().channel(0)[0] == std::sin(12345 * .013), "persistent cache seek");
    require(proxy.build(source, cache, options) == Status::invalid_argument && proxy.sourceInfo(),
            "never overwrite existing cache");
    // Any source stamp/size change invalidates reuse, preserving the old open state.
    std::ofstream append(source, std::ios::binary | std::ios::app);
    append.put(0);
    append.close();
    require(reopened.open(source, cache, 512) != Status::ok &&
                reopened.sourceInfo()->frames == total,
            "stale cache transaction");
    std::cout << "PCM proxy end-seek ns=" << elapsed << " requestedFrames=512 totalFrames=" << total
              << " sampleScratchBytes=8192\n";
}
#if defined(_WIN32)
std::uint64_t privateMemory() noexcept {
    PROCESS_MEMORY_COUNTERS_EX counters{};
    counters.cb = sizeof(counters);
    return K32GetProcessMemoryInfo(GetCurrentProcess(),
                                   reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters),
                                   sizeof(counters))
               ? std::uint64_t(counters.PrivateUsage)
               : 0;
}
void nativeProxy() {
    const auto formats = audioFileFormats();
    for (auto format : {AudioFileFormat::m4a, AudioFileFormat::wma}) {
        const auto capability = std::find_if(formats.begin(), formats.end(),
                                             [&](const auto& c) { return c.format == format; });
        if (capability == formats.end() || !capability->write)
            continue;
        const auto name =
            "cydsp-long-proxy-" +
            std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        const auto source = cydsp::testSupport::temporaryDirectory() /
                            (name + (format == AudioFileFormat::m4a ? ".m4a" : ".wma"));
        const auto cache = cydsp::testSupport::temporaryDirectory() / (name + "-pcm.wav");
        struct Cleanup {
            std::filesystem::path source, cache;
            ~Cleanup() {
                std::error_code ec;
                std::filesystem::remove(source, ec);
                std::filesystem::remove(cache, ec);
            }
        } cleanup{source, cache};
        AudioFileInfo info;
        info.sampleRate = 48000;
        info.channels = 2;
        info.encoding = AudioEncoding::pcm16;
        AudioBuffer<double> data, reference;
        ok(data.resize(2, 1024));
        ok(reference.resize(2, 512));
        AudioFileWriter writer;
        ok(writer.open(source, format, info, "192", 1024));
        constexpr std::size_t total = 48000 * 60;
        for (std::size_t first = 0; first < total; first += 1024) {
            const auto frames = std::min(std::size_t(1024), total - first);
            for (std::size_t i = 0; i < frames; ++i)
                for (std::size_t ch = 0; ch < 2; ++ch)
                    data.block().channel(ch)[i] =
                        .3 * std::sin(2 * std::numbers::pi * (ch ? 1700 : 1000) *
                                      double(first + i) / 48000);
            ok(writer.write(data.block(frames)));
        }
        ok(writer.close());
        AudioFileReader native;
        ok(native.open(source, 1024));
        const auto frames = native.info()->frames;
        require(frames > total - 8192, "long native decoded duration");
        EditingPcmProxy proxy;
        PcmProxyOptions options;
        options.blockFrames = 1024;
        options.maximumWorkingBufferBytes = 65536;
        const auto baseline = privateMemory();
        std::atomic<std::uint32_t> stop{0};
        std::atomic<std::uint64_t> peak{baseline};
        std::thread sampler([&] {
            while (!stop.load(std::memory_order_acquire)) {
                const auto memory = privateMemory();
                auto old = peak.load(std::memory_order_relaxed);
                while (memory > old &&
                       !peak.compare_exchange_weak(old, memory, std::memory_order_relaxed)) {
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });
        const auto built = proxy.build(source, cache, options);
        stop.store(1, std::memory_order_release);
        sampler.join();
        ok(built);
        std::size_t count = 0;
        const auto first = frames - 16384;
        const auto start = std::chrono::steady_clock::now();
        ok(native.read(first, reference.block(), count));
        const auto nativeNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                  std::chrono::steady_clock::now() - start)
                                  .count();
        const auto cached = std::chrono::steady_clock::now();
        ok(proxy.read(first, data.block(512), count));
        const auto proxyNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                 std::chrono::steady_clock::now() - cached)
                                 .count();
        require(count == 512 && proxy.statistics().framesRead == 512 &&
                    proxy.statistics().readCalls == 1,
                "long seek only reads requested PCM block");
        for (std::size_t ch = 0; ch < 2; ++ch)
            for (std::size_t i = 0; i < 512; ++i)
                require(std::abs(data.block().channel(ch)[i] - reference.block().channel(ch)[i]) <
                            1e-8,
                        "native decoded PCM exact proxy");
        const auto high = peak.load(std::memory_order_relaxed);
        std::cout << "Long codec proxy " << (format == AudioFileFormat::m4a ? "AAC/M4A" : "WMA")
                  << " durationSeconds=60 decodedFrames=" << frames
                  << " directEndSeekNs=" << nativeNs << " proxyEndSeekNs=" << proxyNs
                  << " copyBufferBytes=" << proxy.statistics().workingBufferBytes
                  << " pcmBytes=" << proxy.statistics().pcmBytes
                  << " sampledProcessPrivateIncreaseBytes="
                  << (high > baseline ? high - baseline : 0) << "\n";
    }
}
#endif
} // namespace
void pcmProxyRegression() {
    test();
#if defined(_WIN32)
    nativeProxy();
#endif
    std::cout << "PASS integration PCM editing proxy/exact random seek/bounds\n";
}
