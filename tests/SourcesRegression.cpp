#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <complex>
#include <cstdlib>
#include <cydsp/CyDSP.h>
#include <filesystem>
#include <fstream>
#include <numbers>
#include <stdexcept>
#include <string>
#include <thread>
void runRealtimeAudit(void (*)(void*) noexcept, void*);
namespace {
using namespace cydsp;
void require(bool v, const char* m) {
    if (!v)
        throw std::runtime_error(m);
}
void ok(Status s) {
    require(s == Status::ok, "sources status");
}
struct Fixture {
    std::filesystem::path root;
    Fixture() {
        std::filesystem::path base;
#if defined(_MSC_VER)
        char* configured = nullptr;
        std::size_t length = 0;
        if (_dupenv_s(&configured, &length, "CYDSP_TEST_TMP") == 0 && configured)
            base = configured;
        std::free(configured);
        if (base.empty())
            base = std::filesystem::temp_directory_path();
#else
        const auto* configured = std::getenv("CYDSP_TEST_TMP");
        base =
            configured ? std::filesystem::path(configured) : std::filesystem::temp_directory_path();
#endif
        const auto key = std::chrono::steady_clock::now().time_since_epoch().count();
        root = base / ("cydsp-sources-" + std::to_string(key));
        std::filesystem::create_directories(root);
    }
    ~Fixture() {
        std::error_code e;
        std::filesystem::remove(root / "cache.peaks", e);
        std::filesystem::remove(root / "input.wav", e);
        std::filesystem::remove(root / "record.wav", e);
        std::filesystem::remove(root, e);
    }
};
template <Sample T> void sources() {
    Fixture fixture;
    constexpr std::size_t size = 8192;
    AudioBuffer<T> input, out;
    ok(input.resize(2, size));
    ok(out.resize(2, 256));
    for (std::size_t ch = 0; ch < 2; ++ch)
        for (std::size_t i = 0; i < size; ++i)
            input.block().channel(ch)[i] =
                static_cast<T>(.3 * std::sin(2 * std::numbers::pi * (ch ? 2000 : 1000) *
                                             static_cast<double>(i) / 48000));
    AudioFileWriter writer;
    AudioFileInfo info;
    info.sampleRate = 48000;
    info.channels = 2;
    info.encoding = AudioEncoding::float64;
    ok(writer.open(fixture.root / "input.wav", AudioFileFormat::wav, info, {}, size));
    ok(writer.write(AudioBlock<const T>(input.block())));
    ok(writer.close());
    WaveformPeakCache peaks;
    ok(peaks.build(fixture.root / "input.wav", 256));
    require(peaks.levels() == 6 && peaks.peaks(0).size() == 32, "waveform pyramid shape");
    for (std::size_t ch = 0; ch < 2; ++ch) {
        const auto level = peaks.peaks(ch, peaks.levels() - 1);
        double power = 0, min = 1, max = -1;
        for (auto value : input.block().channel(ch)) {
            power += double(value) * value;
            min = std::min(min, double(value));
            max = std::max(max, double(value));
        }
        require(level.size() == 1 && std::abs(level[0].minimum - min) < 1e-7 &&
                    std::abs(level[0].maximum - max) < 1e-7 &&
                    std::abs(level[0].rms - std::sqrt(power / size)) < 1e-7,
                "independent peak/RMS aggregation");
    }
    ok(peaks.save(fixture.root / "cache.peaks"));
    WaveformPeakCache loaded;
    ok(loaded.load(fixture.root / "cache.peaks", fixture.root / "input.wav"));
    require(loaded.info()->frames == size &&
                loaded.info()->formatName == peaks.info()->formatName &&
                loaded.selectLevel(1024) == 2,
            "persistent cache identity");
    {
        std::fstream corrupt(fixture.root / "cache.peaks",
                             std::ios::binary | std::ios::in | std::ios::out);
        corrupt.seekp(12);
        corrupt.put(char(99));
    }
    require(loaded.load(fixture.root / "cache.peaks", fixture.root / "input.wav") ==
                    Status::invalid_argument &&
                loaded.info()->frames == size,
            "cache corruption rejection");
    ImpulseResponse<T> ir;
    ImpulseLoadOptions irOptions;
    irOptions.channels = 1;
    ok(ir.load(fixture.root / "input.wav", irOptions));
    require(ir.samples().channels() == 1 && ir.samples().frames() == size,
            "IR load/channel mapping");
    for (std::size_t i = 0; i < size; ++i)
        require(std::abs(double(ir.samples().channel(0)[i]) -
                         .5 * (double(input.block().channel(0)[i]) + input.block().channel(1)[i])) <
                    1e-7,
                "IR mono average oracle");
    irOptions.sampleRate = 24000;
    ok(ir.load(fixture.root / "input.wav", irOptions));
    require(ir.samples().frames() == size / 2 && ir.sampleRate() == 24000,
            "IR centered rate conversion");
    for (std::size_t i = 128; i < 512; ++i) {
        const auto expected = .15 * (std::sin(2 * std::numbers::pi * 1000 * double(i) / 24000) +
                                     std::sin(2 * std::numbers::pi * 2000 * double(i) / 24000));
        require(std::abs(double(ir.samples().channel(0)[i]) - expected) < .001,
                "IR resampling phase oracle");
    }
    irOptions.sampleRate = 0;
    irOptions.normalize = true;
    ok(ir.load(fixture.root / "input.wav", irOptions));
    double maximum = 0;
    for (auto v : ir.samples().channel(0))
        maximum = std::max(maximum, std::abs(double(v)));
    require(std::abs(maximum - 1) < 1e-6, "IR peak normalization");
    auto moved = std::move(ir);
    require(ir.sampleRate() == 0 && moved.samples().frames() == size, "IR moved-from ownership");
    ReadAheadSource<T> source;
    ok(source.prepare(fixture.root / "input.wav", 48000, 256, 8));
    ok(source.prefill(0, 1024));
    std::size_t read = 0;
    ok(source.readAt(0, out.block(), read));
    require(read == 256, "read-ahead native count");
    for (std::size_t ch = 0; ch < 2; ++ch)
        for (std::size_t i = 0; i < 256; ++i)
            require(out.block().channel(ch)[i] == input.block().channel(ch)[i],
                    "read-ahead exact native decode");
    for (std::uint64_t first : {5000u, 3u, 7000u, 1024u}) {
        ok(source.prefill(first, 2048));
        ok(source.readAt(first, out.block(), read));
        for (std::size_t i = 0; i < 256; ++i)
            require(out.block().channel(0)[i] ==
                        input.block().channel(0)[static_cast<std::size_t>(first) + i],
                    "read-ahead random seek/epochs");
    }
    // Queue seeks while the worker is active, without control-thread prefill.
    // Every returned prefix must belong to the requested epoch, including misses.
    for (std::uint64_t first : {103u, 6000u, 21u, 4097u, 1700u, 400u}) {
        std::size_t done = 0;
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
        while (done < 256 && std::chrono::steady_clock::now() < deadline) {
            const auto s =
                source.readAt(first + done, out.block().subBlock(done, 256 - done), read);
            require(s == Status::ok || s == Status::would_block, "read-ahead seek queue status");
            for (std::size_t i = 0; i < read; ++i)
                require(out.block().channel(0)[done + i] ==
                            input.block().channel(0)[static_cast<std::size_t>(first) + done + i],
                        "read-ahead no stale epoch data");
            done += read;
            if (done < 256)
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        require(done == 256, "read-ahead queued seek completes");
    }
    ok(source.prefill(4096, 1024));
    struct Audit {
        ReadAheadSource<T>& source;
        AudioBuffer<T>& out;
        bool good = true;
    };
    Audit audit{source, out};
    runRealtimeAudit(
        [](void* p) noexcept {
            auto& a = *static_cast<Audit*>(p);
            std::size_t n = 0;
            a.good = a.source.readAt(4096, a.out.block(), n) == Status::ok && n == 256;
        },
        &audit);
    require(audit.good, "read-ahead realtime memory audit");
    source.close();
    ok(source.prepare(fixture.root / "input.wav", 44100, 256, 8));
    require(source.info().frames == (size * 44100 + 47999) / 48000,
            "read-ahead converted duration");
    for (std::uint64_t first : {111u, 2048u, 4000u}) {
        ok(source.prefill(first, 2048));
        ok(source.readAt(first, out.block(), read));
        for (std::size_t i = 0; i < 256; ++i) {
            const auto reference =
                .3 * std::sin(2 * std::numbers::pi * 1000 * static_cast<double>(first + i) / 44100);
            require(std::abs(out.block().channel(0)[i] - reference) < 2e-4,
                    "centered random-access sinc time/quality");
        }
    }
    const auto last = source.info().frames - 13;
    ok(source.prefill(last, 13));
    require(source.readAt(last, out.block(), read) == Status::end_of_stream && read == 13,
            "read-ahead converted EOS count");
    for (std::size_t i = 13; i < 256; ++i)
        require(out.block().channel(0)[i] == 0, "read-ahead EOS padding");
    source.close();
    ReadAheadOptions options;
    options.outputChannels = 1;
    options.pageFrames = 256;
    options.pages = 16;
    ok(source.prepare(fixture.root / "input.wav", options));
    ok(source.prefill(2048, 1024));
    AudioBuffer<T> mono;
    ok(mono.resize(1, 256));
    ok(source.readAt(2048, mono.block(), read));
    for (std::size_t i = 0; i < 256; ++i)
        require(std::abs(double(mono.block().channel(0)[i]) -
                         .5 * (double(input.block().channel(0)[2048 + i]) +
                               input.block().channel(1)[2048 + i])) < 1e-7,
                "worker automatic mono mapping");
    source.close();
#if defined(CYDSP_TEST_IO)
    options.durationRatio = 2;
    options.pitchSemitones = 12;
    ok(source.prepare(fixture.root / "input.wav", options));
    require(source.info().frames == size * 2 && source.info().channels == 1,
            "worker stretch duration/channel contract");
    ok(source.prefill(0, 1024, 5000));
    std::vector<T> rendered(size * 2);
    std::size_t position = 0;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (position < rendered.size()) {
        const auto status = source.readAt(position, mono.block(), read);
        require(status == Status::ok || status == Status::would_block ||
                    status == Status::end_of_stream,
                "worker stretch status");
        std::copy_n(mono.block().channel(0).begin(), read, rendered.begin() + position);
        position += read;
        require(std::chrono::steady_clock::now() < deadline, "worker stretch completion timeout");
        if (!read)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    auto energy = [&](double hz) {
        std::complex<double> sum = 0;
        for (std::size_t i = 6000; i < 12000; ++i) {
            require(isFinite(rendered[i]), "worker finite output");
            sum += double(rendered[i]) *
                   std::polar(1.0, -2 * std::numbers::pi * hz * double(i) / 48000);
        }
        return std::abs(sum);
    };
    require(energy(2000) > 50 && energy(4000) > 50 && energy(2000) > energy(1000) * 10,
            "worker stretch/pitch spectral reference");
    ok(source.prefill(8192, 1024, 5000));
    ok(source.readAt(8192, mono.block(), read));
    double seekEnergy = 0;
    for (auto v : mono.block().channel(0))
        seekEnergy += double(v) * v;
    require(read == 256 && seekEnergy > .1, "worker stretch seek priming");
    source.close();
#endif
    BackgroundRecorder<T> recorder;
    ok(recorder.prepare(fixture.root / "record.wav", AudioFileFormat::wav, info, 256, 8));
    for (std::size_t i = 0; i < size; i += 256) {
        auto block = AudioBlock<const T>(input.block().subBlock(i, 256));
        for (;;) {
            const auto s = recorder.push(block);
            if (s == Status::ok)
                break;
            require(s == Status::update_queue_full, "recorder bounded backpressure");
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
    ok(recorder.finish());
    AudioFileReader reader;
    ok(reader.open(fixture.root / "record.wav", 256));
    require(reader.info()->frames == size, "recorder drain count");
    ok(reader.read(1024, out.block(), read));
    for (std::size_t i = 0; i < 256; ++i)
        require(out.block().channel(0)[i] == input.block().channel(0)[1024 + i],
                "recorder exact worker data");
    reader.close();
    // finish immediately after the final publication; it must acquire and drain it.
    for (unsigned run = 0; run < 12; ++run) {
        ok(recorder.prepare(fixture.root / "record.wav", AudioFileFormat::wav, info, 256, 2));
        ok(recorder.push(AudioBlock<const T>(input.block().subBlock(0, 256))));
        ok(recorder.finish());
        ok(reader.open(fixture.root / "record.wav", 256));
        require(reader.info()->frames == 256, "recorder immediate finish drains final block");
        reader.close();
    }
}
} // namespace
void sourcesRegression() {
    sources<float>();
    sources<double>();
}
