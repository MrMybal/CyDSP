#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <complex>
#include <cydsp/CyDSP.h>
#include <numbers>
#include <source_location>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
void runRealtimeAudit(void (*)(void*) noexcept, void*);
namespace {
using namespace cydsp;
void require(bool v, const char* m) {
    if (!v)
        throw std::runtime_error(m);
}
void ok(Status s, std::source_location at = std::source_location::current()) {
    if (s != Status::ok)
        throw std::runtime_error("IO status " + std::to_string(static_cast<unsigned>(s)) +
                                 " at line " + std::to_string(at.line()));
}
template <Sample T> void clear(AudioBlock<T> b) {
    for (std::size_t ch = 0; ch < b.channels(); ++ch)
        std::fill(b.channel(ch).begin(), b.channel(ch).end(), T{});
}
void files() {
    const std::size_t frames = 16384;
    AudioBuffer<double> source;
    ok(source.resize(2, frames));
    for (std::size_t ch = 0; ch < 2; ++ch)
        for (std::size_t i = 0; i < frames; ++i)
            source.block().channel(ch)[i] =
                .3 * std::sin(2 * std::numbers::pi * (ch ? 1500 : 1000) * static_cast<double>(i) /
                              48000);
    const auto capabilities = audioFileFormats();
    require(capabilities.size() >= 7, "format capabilities");
    for (const auto format : {AudioFileFormat::wav, AudioFileFormat::flac,
                              AudioFileFormat::oggVorbis, AudioFileFormat::aiff}) {
        auto stream =
            std::make_shared<std::stringstream>(std::ios::in | std::ios::out | std::ios::binary);
        AudioFileWriter writer;
        AudioFileInfo info;
        info.sampleRate = 48000;
        info.channels = 2;
        info.encoding =
            format == AudioFileFormat::oggVorbis ? AudioEncoding::float32 : AudioEncoding::pcm24;
        ok(writer.open(stream, format, info, {}, 257));
        for (std::size_t i = 0; i < frames; i += 257)
            ok(writer.write(AudioBlock<const double>(
                source.block().subBlock(i, std::min<std::size_t>(257, frames - i)))));
        require(writer.framesWritten() == frames, "writer exact frame count");
        ok(writer.close());
        const auto encoded = stream->str();
        require(encoded.size() > 100, "encoded file nonempty");
        AudioFileReader reader;
        ok(reader.open(std::make_shared<std::istringstream>(encoded, std::ios::binary), 300));
        require(reader.info() && reader.info()->channels == 2 &&
                    reader.info()->sampleRate == 48000 && reader.info()->frames == frames,
                "audio header properties");
        AudioBuffer<double> out;
        ok(out.resize(2, 300));
        double squared = 0, maximum = 0;
        std::size_t count = 0;
        for (std::size_t i = 0; i < frames; i += 300) {
            const auto n = std::min<std::size_t>(300, frames - i);
            const auto s = reader.read(i, out.block(), count);
            require(s == (n == 300 ? Status::ok : Status::end_of_stream) && count == n,
                    "reader EOS count");
            for (std::size_t ch = 0; ch < 2; ++ch) {
                for (std::size_t j = 0; j < n; ++j) {
                    const auto error =
                        out.block().channel(ch)[j] - source.block().channel(ch)[i + j];
                    maximum = std::max(maximum, std::abs(error));
                    squared += error * error;
                }
                for (std::size_t j = n; j < 300; ++j)
                    require(out.block().channel(ch)[j] == 0, "EOS zero padding");
            }
        }
        require(format == AudioFileFormat::oggVorbis ? std::sqrt(squared / (2 * frames)) < .015
                                                     : maximum < 3e-7,
                "audio roundtrip quality");
        ok(reader.read(700, out.block(19), count));
        require(count == 19, "random-access seek");
        const auto original = reader.info()->frames;
        require(reader.open(std::make_shared<std::istringstream>("not audio")) ==
                        Status::unsupported_format &&
                    reader.info()->frames == original,
                "failed reader preserves state");
        auto moved = std::move(reader);
        require(reader.info() == nullptr && moved.info()->frames == frames, "reader moved-from");
    }
    // Hand-built WAV PCM16 fixture independent of CHOC's writer.
    const std::array<unsigned char, 50> wav{
        'R', 'I', 'F', 'F', 42,  0,   0, 0,    'W',  'A', 'V', 'E',  'f',  'm', 't',  ' ', 16,
        0,   0,   0,   1,   0,   1,   0, 0x80, 0xbb, 0,   0,   0,    0x77, 1,   0,    2,   0,
        16,  0,   'd', 'a', 't', 'a', 6, 0,    0,    0,   0,   0x80, 0,    0,   0xff, 0x7f};
    AudioFileReader reader;
    ok(reader.open(std::make_shared<std::istringstream>(
        std::string(reinterpret_cast<const char*>(wav.data()), wav.size()))));
    AudioBuffer<float> samples;
    ok(samples.resize(1, 3));
    std::size_t count = 0;
    ok(reader.read(0, samples.block(), count));
    require(samples.block().channel(0)[0] == -1 && samples.block().channel(0)[1] == 0 &&
                std::abs(samples.block().channel(0)[2] - 32767.0f / 32768) < 1e-7,
            "independent PCM16 fixture");
    const std::array<unsigned char, 60> aiff{
        'F',  'O',  'R', 'M', 0,  0, 0, 52, 'A', 'I',  'F', 'F', 'C', 'O',  'M',
        'M',  0,    0,   0,   18, 0, 1, 0,  0,   0,    3,   0,   16,  0x40, 0x0e,
        0xbb, 0x80, 0,   0,   0,  0, 0, 0,  'S', 'S',  'N', 'D', 0,   0,    0,
        14,   0,    0,   0,   0,  0, 0, 0,  0,   0x80, 0,   0,   0,   0x7f, 0xff};
    ok(reader.open(std::make_shared<std::istringstream>(
        std::string(reinterpret_cast<const char*>(aiff.data()), aiff.size()))));
    ok(reader.read(0, samples.block(), count));
    require(reader.info()->sampleRate == 48000 && samples.block().channel(0)[0] == -1 &&
                samples.block().channel(0)[2] == 32767.0f / 32768,
            "independent AIFF PCM/rate fixture");
    AudioFileWriter unsupported;
    auto memory = std::make_shared<std::ostringstream>();
    AudioFileInfo info;
    info.sampleRate = 48000;
    info.channels = 2;
    require(unsupported.open(memory, AudioFileFormat::mp3, info) == Status::unsupported_format &&
                memory->str().empty(),
            "MP3 export explicitly unavailable");
}
template <Sample T> void stretch() {
    constexpr std::size_t block = 256;
    const ProcessSpec spec{48000, block, 1};
    AudioBuffer<T> in, out;
    ok(in.resize(1, block));
    ok(out.resize(1, block));
    TimeStretch<T> engine;
    ok(engine.prepare(spec, block));
    ok(engine.setPitchSemitones(12));
    std::vector<double> signal;
    signal.reserve(48000);
    for (std::size_t start = 0; start < 48000; start += block) {
        for (std::size_t j = 0; j < block; ++j)
            in.block().channel(0)[j] = static_cast<T>(
                .3 * std::sin(2 * std::numbers::pi * 440 * static_cast<double>(start + j) / 48000));
        ok(engine.process(in.block(), out.block()));
        if (start > 12000)
            for (auto x : out.block().channel(0)) {
                require(isFinite(x), "stretch finite");
                signal.push_back(x);
            }
    }
    auto energy = [&](double hz) {
        std::complex<double> sum{};
        for (std::size_t i = 0; i < signal.size(); ++i)
            sum += signal[i] *
                   std::polar(1.0, -2 * std::numbers::pi * hz * static_cast<double>(i) / 48000);
        return std::abs(sum);
    };
    require(energy(880) > energy(440) * 20 && energy(880) > 100, "pitch octave spectral reference");
    // Own wrapper rejects aliases and preserves state, including nonzero view offsets.
    require(engine.process(in.block(), in.block()) == Status::partial_overlap,
            "stretch overlap rejection");
    struct Audit {
        TimeStretch<T>& engine;
        AudioBuffer<T>& in;
        AudioBuffer<T>& out;
        bool success = true;
    };
    Audit audit{engine, in, out};
    runRealtimeAudit(
        [](void* p) noexcept {
            auto& a = *static_cast<Audit*>(p);
            for (unsigned i = 0; i < 24; ++i)
                a.success =
                    a.engine.process(a.in.block(), a.out.block()) == Status::ok && a.success;
            a.engine.reset();
            a.success = a.engine.setPitchSemitones(-5) == Status::ok && a.success;
            a.success = a.engine.setFormant(1.2, true, 200) == Status::ok && a.success;
            for (unsigned i = 0; i < 32; ++i)
                a.success =
                    a.engine.process(a.in.block(), a.out.block()) == Status::ok && a.success;
            for (unsigned i = 0; i < 64; ++i) {
                const auto s = a.engine.flush(a.out.block());
                a.success = (s == Status::ok || s == Status::end_of_stream) && a.success;
            }
            a.engine.reset();
        },
        &audit);
    require(audit.success, "stretch realtime statuses");
    // Duration ratio and zero input tail are driven by independent input/output sizes.
    ok(engine.prepare(spec, 512));
    AudioBuffer<T> longer;
    ok(longer.resize(1, 512));
    ok(engine.process(in.block(), longer.block()));
    require(engine.inputLatency() > 0 && engine.outputLatency() > 0, "stretch reported latency");
    ok(engine.prepare({48000, 32768, 1}, 512));
    AudioBuffer<T> history;
    ok(history.resize(1, engine.inputLatency() + engine.outputLatency()));
    for (std::size_t i = 0; i < history.block().frames(); ++i)
        history.block().channel(0)[i] = static_cast<T>(.3 * std::sin(.02 * double(i)));
    struct PrimeAudit {
        TimeStretch<T>* engine;
        AudioBuffer<T>* history;
        AudioBuffer<T>* in;
        AudioBuffer<T>* out;
        Status result = Status::ok;
    } prime{&engine, &history, &in, &longer};
    runRealtimeAudit(
        [](void* ptr) noexcept {
            auto& p = *static_cast<PrimeAudit*>(ptr);
            p.result = p.engine->prime(p.history->block());
            if (p.result == Status::ok)
                p.result = p.engine->process(p.in->block(), p.out->block());
        },
        &prime);
    ok(prime.result);
}
void virtualDevice() {
    AudioDeviceContext context;
    ok(context.prepare(true));
    std::vector<AudioDeviceInfo> devices;
    ok(context.enumerate(devices));
    require(!devices.empty(), "null audio enumeration");
    struct Callback {
        std::atomic<unsigned> calls{0};
        std::atomic<bool> valid{true};
    };
    Callback state;
    AudioDevice device;
    AudioDeviceConfig config;
    config.maxCallbackFrames = 37;
    config.periodFrames = 256;
    auto callback = [](void* p, AudioBlock<const float> in, AudioBlock<float> out,
                       std::uint64_t) noexcept {
        auto& s = *static_cast<Callback*>(p);
        if (in.channels() != 0 || out.channels() != 2 || out.frames() > 37)
            s.valid.store(false);
        for (std::size_t ch = 0; ch < out.channels(); ++ch)
            for (auto x : out.channel(ch))
                if (x != 0)
                    s.valid.store(false);
        s.calls.fetch_add(1);
        return Status::ok;
    };
    ok(device.open(context, config, callback, &state));
    ok(device.start());
    for (unsigned i = 0; i < 100 && state.calls.load() < 20; ++i)
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    ok(device.stop());
    require(state.calls.load() >= 20 && state.valid.load() && device.callbackErrors() == 0 &&
                device.sampleRate() == 48000,
            "virtual device callback contract");
    config.outputId = "invalid";
    require(device.open(context, config, callback, &state) == Status::invalid_argument,
            "device ID validation");
    device.close();
    MidiDevicePorts ports;
    const auto status = enumerateMidiPorts(ports);
    require(status == Status::ok || status == Status::unsupported_backend ||
                status == Status::device_unavailable,
            "MIDI device enumeration status");
}
} // namespace
void ioRegression() {
    files();
    stretch<float>();
    stretch<double>();
    virtualDevice();
}
