#include <cmath>
#include <cydsp/CyDSP.h>
#include <fstream>
#include <iostream>
#include <numbers>
#include <source_location>
#include <sstream>
#include <stdexcept>
namespace {
using namespace cydsp;
void require(bool v, const char* why) {
    if (!v)
        throw std::runtime_error(why);
}
void ok(Status r, std::source_location at = std::source_location::current()) {
    if (r != Status::ok)
        throw std::runtime_error("DAW file status " + std::to_string(int(r)) + " at " +
                                 std::to_string(at.line()));
}
void containers() {
    AudioBuffer<double> source, out;
    ok(source.resize(2, 64));
    ok(out.resize(2, 64));
    for (std::size_t c = 0; c < 2; ++c)
        for (std::size_t i = 0; i < 64; ++i)
            source.block().channel(c)[i] = (double(i) - 32) / 32 * (c ? .5 : 1);
    for (auto f : {AudioFileFormat::aiff, AudioFileFormat::aifc, AudioFileFormat::caf})
        for (auto e : {AudioEncoding::pcm8, AudioEncoding::pcm16, AudioEncoding::pcm24,
                       AudioEncoding::pcm32, AudioEncoding::float32, AudioEncoding::float64})
            for (bool little : {false, true}) {
                if (f == AudioFileFormat::aiff && (little || e >= AudioEncoding::float32))
                    continue;
                if (f == AudioFileFormat::aifc && little && e >= AudioEncoding::float32)
                    continue;
                AudioFileInfo info;
                info.sampleRate = 96000;
                info.channels = 2;
                info.encoding = e;
                if (f != AudioFileFormat::caf)
                    info.metadataJson =
                        R"([{"type":"MARK","hex":"000200010000001001410002000000200142"},{"type":"INST","hex":"3c00007f017f0000000100010002000000000000"}])";
                else
                    info.metadataJson =
                        R"([{"type":"mark","hex":"000000000000000100000000402800000000000000000007000000000000000000000000"}])";
                auto memory = std::make_shared<std::stringstream>(std::ios::in | std::ios::out |
                                                                  std::ios::binary);
                AudioFileWriter writer;
                ok(writer.open(memory, f, info,
                               f == AudioFileFormat::aiff ? ""
                               : little                   ? "little-endian"
                                                          : "big-endian",
                               64));
                ok(writer.write(source.block()));
                ok(writer.close());
                const auto bytes = memory->str();
                require(bytes.substr(0, 4) == (f == AudioFileFormat::caf ? "caff" : "FORM"),
                        "container magic");
                if (f == AudioFileFormat::caf)
                    require(bytes.substr(8, 4) == "desc", "CAF description first");
                if (f == AudioFileFormat::aifc)
                    require(bytes.substr(8, 4) == "AIFC" && bytes.substr(12, 4) == "FVER",
                            "AIFC version header");
                AudioFileReader reader;
                ok(reader.open(std::static_pointer_cast<std::istream>(memory), 64));
                require(reader.info()->frames == 64 && reader.info()->sampleRate == 96000 &&
                            reader.info()->encoding == e,
                        "PCM shape/encoding");
                require(reader.info()->metadataJson.find(f == AudioFileFormat::caf
                                                             ? "4028000000000000"
                                                             : "000100010002") != std::string::npos,
                        "marker/loop chunks retained");
                std::size_t n = 0;
                ok(reader.read(0, out.block(), n));
                require(n == 64, "PCM count");
                const auto tolerance = e == AudioEncoding::pcm8    ? 1.0 / 128
                                       : e == AudioEncoding::pcm16 ? 1.0 / 32768
                                       : e == AudioEncoding::pcm24 ? 1.0 / 8388608
                                                                   : 1e-7;
                for (std::size_t c = 0; c < 2; ++c)
                    for (std::size_t i = 0; i < 64; ++i)
                        require(std::abs(source.block().channel(c)[i] -
                                         out.block().channel(c)[i]) <= tolerance,
                                "PCM endian/float/integer reference");
                // Re-exporting the reader metadata preserves the exact marker/loop payload.
                auto again = std::make_shared<std::stringstream>(std::ios::in | std::ios::out |
                                                                 std::ios::binary);
                auto saved = *reader.info();
                ok(writer.open(again, f, saved, {}, 64));
                ok(writer.write(out.block()));
                ok(writer.close());
                AudioFileReader second;
                ok(second.open(std::static_pointer_cast<std::istream>(again), 64));
                require(second.info()->metadataJson == reader.info()->metadataJson,
                        "native metadata exact roundtrip");
            }
    // Independently constructed little-endian CAF PCM16, not the CyDSP writer.
    std::string bytes;
    auto put = [&](std::uint64_t v, unsigned n) {
        while (n--)
            bytes += char(v >> (n * 8));
    };
    bytes += "caff";
    put(1, 2);
    put(0, 2);
    bytes += "desc";
    put(32, 8);
    put(0x40e7700000000000ULL, 8);
    bytes += "lpcm";
    put(2, 4);
    put(2, 4);
    put(1, 4);
    put(1, 4);
    put(16, 4);
    bytes += "data";
    put(10, 8);
    put(0, 4);
    bytes.append("\x00\x80\x00\x00\xff\x7f", 6);
    AudioFileReader independent;
    ok(independent.open(std::make_shared<std::istringstream>(bytes), 3));
    AudioBuffer<double> mono;
    ok(mono.resize(1, 3));
    std::size_t n = 0;
    ok(independent.read(0, mono.block(), n));
    require(independent.info()->sampleRate == 48000 && mono.block().channel(0)[0] == -1 &&
                mono.block().channel(0)[2] == 32767.0 / 32768,
            "independent CAF fixture");
    bytes[40] = 0; // bytesPerPacket no longer matches the packed frame.
    // A truncated payload is always rejected without replacing the previous reader.
    require(
        independent.open(std::make_shared<std::istringstream>(bytes.substr(0, bytes.size() - 1)),
                         3) != Status::ok &&
            independent.info()->frames == 3,
        "truncated PCM transaction");
    // WAV cue/smpl are preserved using the existing private CHOC metadata schema.
    AudioFileInfo info;
    info.channels = 2;
    info.sampleRate = 48000;
    info.encoding = AudioEncoding::pcm16;
    info.metadataJson =
        R"([{"type":"cue ","cues":[{"ID":7,"position":12,"dataChunkID":"data","chunkStart":0,"blockStart":0,"sampleOffset":12}]},{"type":"smpl","midiUnityNote":60,"loops":[{"ID":7,"loopType":0,"startByte":16,"endByte":31,"fractionalOffset":0,"playCount":0}]}])";
    auto wav = std::make_shared<std::stringstream>(std::ios::in | std::ios::out | std::ios::binary);
    AudioFileWriter w;
    ok(w.open(wav, AudioFileFormat::wav, info, {}, 64));
    ok(w.write(source.block()));
    ok(w.close());
    AudioFileReader r;
    ok(r.open(std::static_pointer_cast<std::istream>(wav), 64));
    require(r.info()->metadataJson.find("smpl") != std::string::npos &&
                r.info()->metadataJson.find("31") != std::string::npos,
            "WAV sampler loop preservation");
}
void mp3Fixture() {
    // Original MPEG-1 Layer III silence: empty granules/zero Huffman values,
    // no bit reservoir, 128kbps /44.1kHz stereo. No third-party audio asset.
    std::string data;
    for (unsigned i = 0; i < 8; ++i) {
        data.append("\xff\xfb\x90\x00", 4);
        data.append(413, '\0');
    }
    AudioFileReader reader;
    ok(reader.open(std::make_shared<std::istringstream>(data), 1152));
    require(reader.info()->formatName == "MP3" && reader.info()->channels == 2 &&
                reader.info()->sampleRate == 44100 && reader.info()->frames > 0,
            "MP3 successful fixture info");
    AudioBuffer<float> out;
    ok(out.resize(2, 1152));
    std::size_t n = 0;
    ok(reader.read(0, out.block(), n));
    require(n == 1152, "MP3 success count");
    for (std::size_t c = 0; c < 2; ++c)
        for (auto v : out.block().channel(c))
            require(v == 0, "MP3 silence decoding");
}
void nativeLossy() {
    const auto formats = audioFileFormats();
    for (auto format :
         {AudioFileFormat::mp3, AudioFileFormat::m4a, AudioFileFormat::aac, AudioFileFormat::wma}) {
        auto capability = std::find_if(formats.begin(), formats.end(),
                                       [&](const auto& f) { return f.format == format; });
        if (capability == formats.end() || !capability->write)
            continue;
        const auto extension = format == AudioFileFormat::mp3   ? ".mp3"
                               : format == AudioFileFormat::m4a ? ".m4a"
                               : format == AudioFileFormat::aac ? ".aac"
                                                                : ".wma";
        const auto path = std::filesystem::temp_directory_path() /
                          ("cydsp-native-" +
                           std::to_string(reinterpret_cast<std::uintptr_t>(&formats)) + extension);
        struct Cleanup {
            std::filesystem::path p;
            ~Cleanup() {
                std::error_code ec;
                std::filesystem::remove(p, ec);
            }
        } cleanup{path};
        AudioFileWriter writer;
        AudioFileInfo info;
        info.sampleRate = 48000;
        info.channels = 2;
        info.encoding = AudioEncoding::pcm16;
        ok(writer.open(path, format, info, "192", 1024));
        AudioBuffer<float> block;
        ok(block.resize(2, 1024));
        constexpr std::size_t frames = 24576;
        for (std::size_t at = 0; at < frames; at += 1024) {
            for (std::size_t i = 0; i < 1024; ++i)
                for (std::size_t c = 0; c < 2; ++c)
                    block.block().channel(c)[i] =
                        float(.3 * std::sin(2 * std::numbers::pi * 1000 * double(at + i) / 48000));
            ok(writer.write(block.block()));
        }
        require(writer.framesWritten() == frames, "native encoder input count");
        ok(writer.close());
        AudioFileReader reader;
        ok(reader.open(path, 8192));
        require(reader.info()->frames >= 16384 && reader.info()->channels == 2,
                "native lossy readback");
        AudioBuffer<double> out;
        ok(out.resize(2, 8192));
        std::size_t n = 0;
        ok(reader.read(4096, out.block(), n));
        double sin = 0, cos = 0, energy = 0;
        for (std::size_t i = 0; i < n; ++i) {
            const auto x = out.block().channel(0)[i];
            require(std::isfinite(x), "lossy finite");
            const auto phase = 2 * std::numbers::pi * 1000 * double(i) / 48000;
            sin += x * std::sin(phase);
            cos += x * std::cos(phase);
            energy += x * x;
        }
        const auto amplitude = 2 * std::hypot(sin, cos) / double(n);
        require(amplitude > .25 && amplitude < .33 && energy / double(n) > .03,
                "native codec tone quality");
        const auto reference = out.block().channel(0)[500];
        ok(reader.read(0, out.block().subBlock(0, 512), n));
        ok(reader.read(4096, out.block(), n));
        require(std::abs(reference - out.block().channel(0)[500]) < 1e-8,
                "native codec exact decoded seek");
        std::cout << "PASS platform codec " << extension << " encode/decode/tone/seek\n";
    }
}
} // namespace
void dawFilesRegression() {
    containers();
    mp3Fixture();
    nativeLossy();
    std::cout << "PASS DAW AIFC/CAF/markers/loops/MP3 fixture\n";
}
