#include <array>
#include <cydsp/CyDSP.h>
#include <iostream>
#include <sstream>
#include <stdexcept>
namespace {
using namespace cydsp;
using Bytes = std::vector<std::uint8_t>;
void require(bool v, const char* why) {
    if (!v)
        throw std::runtime_error(why);
}
void ok(Status v) {
    require(v == Status::ok, "metadata status");
}
void le(Bytes& b, std::uint64_t v, unsigned n = 4) {
    for (unsigned i = 0; i < n; ++i)
        b.push_back(std::uint8_t(v >> (i * 8)));
}
void tag(Bytes& b, std::string_view t) {
    b.insert(b.end(), t.begin(), t.end());
}
void chunk(Bytes& b, std::string_view t, const Bytes& p) {
    tag(b, t);
    le(b, p.size());
    b.insert(b.end(), p.begin(), p.end());
    if (p.size() & 1)
        b.push_back(0);
}
Bytes string(std::string_view s) {
    Bytes b(s.begin(), s.end());
    b.push_back(0);
    return b;
}
std::uint32_t u32(std::span<const std::uint8_t> b, std::size_t at) {
    require(at <= b.size() && b.size() - at >= 4, "external parser bounds");
    return std::uint32_t(b[at]) | (std::uint32_t(b[at + 1]) << 8) |
           (std::uint32_t(b[at + 2]) << 16) | (std::uint32_t(b[at + 3]) << 24);
}
template <class F> void walk(std::span<const std::uint8_t> b, F f) {
    std::size_t at = 0;
    while (at < b.size()) {
        require(b.size() - at >= 8, "external chunk header");
        std::string name(reinterpret_cast<const char*>(b.data() + at), 4);
        const auto length = u32(b, at + 4);
        at += 8;
        require(length <= b.size() - at && std::size_t(length) + (length & 1u) <= b.size() - at,
                "external chunk/pad");
        f(name, b.subspan(at, length));
        at += length + (length & 1u);
    }
}
Bytes fixture() {
    Bytes body;
    tag(body, "WAVE");
    Bytes fmt;
    le(fmt, 1, 2);
    le(fmt, 1, 2);
    le(fmt, 48000);
    le(fmt, 96000);
    le(fmt, 2, 2);
    le(fmt, 16, 2);
    chunk(body, "fmt ", fmt);
    Bytes cue;
    le(cue, 2);
    for (auto id : {7u, 9u}) {
        le(cue, id);
        le(cue, 0);
        tag(cue, "data");
        le(cue, 0);
        le(cue, 0);
        le(cue, id == 7 ? 93 : 99);
    }
    chunk(body, "cue ", cue);
    Bytes labels;
    tag(labels, "adtl");
    Bytes label;
    le(label, 7);
    auto name = string("Door impact");
    label.insert(label.end(), name.begin(), name.end());
    chunk(labels, "labl", label);
    Bytes note;
    le(note, 7);
    auto detail = string("prise \xc3\xa9");
    note.insert(note.end(), detail.begin(), detail.end());
    chunk(labels, "note", note);
    Bytes region;
    le(region, 7);
    le(region, 55);
    tag(region, "rgn ");
    le(region, 0, 2);
    le(region, 0, 2);
    le(region, 0, 2);
    le(region, 65001, 2);
    auto txt = string("R\xc3\xa9gion");
    region.insert(region.end(), txt.begin(), txt.end());
    chunk(labels, "ltxt", region);
    chunk(labels, "zzzz", {1, 2, 3});
    chunk(body, "LIST", labels);
    Bytes other;
    tag(other, "adtl");
    Bytes label2;
    le(label2, 9);
    auto n = string("\xc3\x89"
                    "cho");
    label2.insert(label2.end(), n.begin(), n.end());
    chunk(other, "labl", label2);
    chunk(body, "LIST", other);
    Bytes info;
    tag(info, "INFO");
    chunk(info, "INAM", string("Title"));
    chunk(info, "IART", string("Artist"));
    chunk(info, "XRAW", {0xff, 0, 1});
    chunk(body, "LIST", info);
    Bytes smpl;
    for (unsigned i = 0; i < 7; ++i)
        le(smpl, i == 3 ? 60 : 0);
    le(smpl, 1);
    le(smpl, 0);
    le(smpl, 7);
    le(smpl, 1);
    le(smpl, 16);
    le(smpl, 31);
    le(smpl, 0);
    le(smpl, 2);
    chunk(body, "smpl", smpl);
    chunk(body, "Odd!", {0x42});
    Bytes audio;
    for (unsigned i = 0; i < 256; ++i)
        le(audio, i * 64, 2);
    chunk(body, "data", audio);
    Bytes file;
    tag(file, "RIFF");
    le(file, body.size());
    file.insert(file.end(), body.begin(), body.end());
    return file;
}
struct Parsed {
    std::vector<Bytes> lists;
    std::string label, note, title, artist, second;
    unsigned cueFrame = 0, regionLength = 0, loopFirst = 0, loopLast = 0;
    bool odd = false, unknown = false;
};
Parsed independently(std::span<const std::uint8_t> file) {
    require(file.size() >= 12 && u32(file, 4) == file.size() - 8, "exact RIFF declared extent");
    Parsed p;
    walk(file.subspan(12), [&](const auto& name, auto b) {
        if (name == "cue ") {
            require(u32(b, 0) == 2, "external cue count");
            p.cueFrame = u32(b, 24);
        }
        if (name == "smpl") {
            p.loopFirst = u32(b, 44);
            p.loopLast = u32(b, 48);
        }
        if (name == "Odd!")
            p.odd = b.size() == 1 && b[0] == 0x42;
        if (name != "LIST")
            return;
        p.lists.emplace_back(b.begin(), b.end());
        require(b.size() >= 4, "list type exists");
        const auto type = std::string(reinterpret_cast<const char*>(b.data()), 4);
        walk(b.subspan(4), [&](const auto& child, auto value) {
            auto text = [&](std::size_t at) {
                require(at < value.size() && value.back() == 0, "external NUL");
                return std::string(reinterpret_cast<const char*>(value.data() + at),
                                   value.size() - at - 1);
            };
            if (type == "adtl") {
                if (child == "labl") {
                    if (u32(value, 0) == 7)
                        p.label = text(4);
                    else if (u32(value, 0) == 9)
                        p.second = text(4);
                }
                if (child == "note")
                    p.note = text(4);
                if (child == "ltxt") {
                    p.regionLength = u32(value, 4);
                    require(text(20) == "R\xc3\xa9gion", "region UTF8");
                }
                if (child == "zzzz")
                    p.unknown = value.size() == 3 && value[2] == 3;
            } else if (type == "INFO") {
                if (child == "INAM")
                    p.title = text(0);
                if (child == "IART")
                    p.artist = text(0);
            }
        });
    });
    return p;
}
void extensible() {
    Bytes body;
    tag(body, "WAVE");
    Bytes fmt;
    le(fmt, 0xfffe, 2);
    le(fmt, 1, 2);
    le(fmt, 48000);
    le(fmt, 192000);
    le(fmt, 4, 2);
    le(fmt, 32, 2);
    le(fmt, 22, 2);
    le(fmt, 24, 2);
    le(fmt, 1);
    const Bytes guid{1, 0, 0, 0, 0, 0, 0x10, 0, 0x80, 0, 0, 0xaa, 0, 0x38, 0x9b, 0x71};
    fmt.insert(fmt.end(), guid.begin(), guid.end());
    chunk(body, "fmt ", fmt);
    Bytes samples;
    le(samples, 0x40000000);
    le(samples, 0xc0000000);
    chunk(body, "data", samples);
    Bytes bytes;
    tag(bytes, "RIFF");
    le(bytes, body.size());
    bytes.insert(bytes.end(), body.begin(), body.end());
    AudioFileReader reader;
    ok(reader.open(std::make_shared<std::istringstream>(std::string(bytes.begin(), bytes.end()),
                                                        std::ios::binary),
                   2));
    require(reader.info()->encoding == AudioEncoding::pcm32, "extensible container width");
    AudioBuffer<double> output;
    ok(output.resize(1, 2));
    std::size_t read = 0;
    ok(reader.read(0, output.block(), read));
    require(output.block().channel(0)[0] == .5 && output.block().channel(0)[1] == -.5,
            "24 significant bits in 32-bit container");
}
void test() {
    auto original = fixture();
    auto expected = independently(original);
    auto input = std::make_shared<std::istringstream>(
        std::string(reinterpret_cast<const char*>(original.data()), original.size()),
        std::ios::binary);
    AudioFileReader reader;
    ok(reader.open(input, 256));
    auto info = *reader.info();
    AudioMetadata metadata;
    ok(getAudioMetadata(info, metadata));
    require(metadata.markers.size() == 2 && metadata.markers[0].name == "Door impact" &&
                metadata.markers[0].frame == 93 &&
                metadata.markers[1].name == "\xc3\x89"
                                            "cho" &&
                metadata.regions.size() == 1 && metadata.regions[0].lengthFrames == 55 &&
                metadata.loops.size() == 1 && metadata.loops[0].startFrame == 16 &&
                metadata.loops[0].endFrame == 32 &&
                metadata.loops[0].mode == AudioLoopMode::alternating && metadata.tags.size() == 2,
            "public vocabulary");
    AudioBuffer<double> audio;
    ok(audio.resize(1, 256));
    std::size_t n = 0;
    ok(reader.read(0, audio.block(), n));
    auto write = [&](const AudioFileInfo& props) {
        auto output = std::make_shared<std::ostringstream>(std::ios::binary);
        AudioFileWriter w;
        ok(w.open(output, AudioFileFormat::wav, props, {}, 256));
        ok(w.write(audio.block()));
        ok(w.close());
        auto s = output->str();
        return Bytes(s.begin(), s.end());
    };
    auto unchanged = independently(write(info));
    require(unchanged.lists == expected.lists && unchanged.label == expected.label &&
                unchanged.cueFrame == 93 && unchanged.regionLength == 55 &&
                unchanged.loopFirst == 16 && unchanged.loopLast == 31 && unchanged.odd &&
                unchanged.unknown,
            "independent byte-exact LIST roundtrip");
    metadata.markers[0].name = "Edited";
    metadata.loops[0].endFrame = 48;
    metadata.tags[0].value = "Changed";
    ok(setAudioMetadata(info, metadata));
    auto edited = independently(write(info));
    require(edited.label == "Edited" && edited.loopLast == 47 && edited.title == "Changed" &&
                edited.artist == "Artist" && edited.odd && edited.unknown &&
                edited.note == expected.note && edited.second == expected.second,
            "typed edits preserve unknowns");
    std::string json;
    ok(serializeAudioMetadata(metadata, json));
    AudioMetadata decoded;
    ok(parseAudioMetadata(json, decoded));
    require(decoded.markers[0].name == "Edited" && decoded.loops[0].endFrame == 48,
            "versioned metadata");
    auto saved = decoded.markers[0].name;
    require(parseAudioMetadata(R"({"schema":"CyDSP.AudioMetadata","version":2})", decoded) !=
                    Status::ok &&
                decoded.markers[0].name == saved,
            "unknown schema transaction");
    const auto rejected = [&](Bytes bytes) {
        auto stream = std::make_shared<std::istringstream>(
            std::string(reinterpret_cast<const char*>(bytes.data()), bytes.size()),
            std::ios::binary);
        require(reader.open(stream, 256) != Status::ok && reader.info()->frames == 256,
                "malformed RIFF rejected transactionally");
    };
    auto invalid = original;
    invalid[4] = invalid[5] = invalid[6] = invalid[7] = 0xff;
    rejected(invalid);
    invalid = original;
    invalid[32] = 0;
    invalid[33] = 32;
    rejected(invalid); // Invalid frame stride used to produce zero progress.
    invalid = original;
    for (std::size_t at = 12; at + 8 <= invalid.size();) {
        const auto length = u32(invalid, at + 4);
        if (std::string(reinterpret_cast<const char*>(invalid.data() + at), 4) == "cue ") {
            invalid[at + 8] = invalid[at + 9] = invalid[at + 10] = invalid[at + 11] = 0xff;
            break;
        }
        at += 8 + length + (length & 1u);
    }
    rejected(invalid);
    // A damaged nested length must not be clamped to unrelated file data.
    auto malformed = original;
    for (std::size_t at = 0; at + 4 < malformed.size(); ++at)
        if (std::string(reinterpret_cast<const char*>(malformed.data() + at), 4) == "labl") {
            malformed[at + 4] = 0xff;
            malformed[at + 5] = 0xff;
            break;
        }
    auto bad = std::make_shared<std::istringstream>(
        std::string(reinterpret_cast<const char*>(malformed.data()), malformed.size()),
        std::ios::binary);
    require(reader.open(bad, 256) != Status::ok && reader.info()->frames == 256,
            "nested RIFF bounds/transaction");
}
} // namespace
void metadataRegression() {
    test();
    extensible();
    std::cout << "PASS integration WAV LIST/adtl/INFO/typed metadata/independent fixtures\n";
}
