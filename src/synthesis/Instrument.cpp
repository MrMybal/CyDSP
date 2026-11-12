#include "cydsp/synthesis/Instrument.h"
#include <choc/text/choc_JSON.h>
#include <algorithm>
#include <fstream>
#include <stdexcept>
namespace cydsp {
namespace {
bool valid(const InstrumentPreset& p) noexcept {
    if (p.name.size() > 4096 || !p.voices || p.voices > 1024 || p.rootNote > 127 ||
        p.lowerMpeMembers > 15 || p.upperMpeMembers > 15 ||
        (p.lowerMpeMembers && p.upperMpeMembers && p.lowerMpeMembers + p.upperMpeMembers > 14) ||
        static_cast<unsigned>(p.source) > static_cast<unsigned>(InstrumentSource::wavetable) ||
        validateSynthParameters(p.parameters) != Status::ok || p.loopStart > p.loopEnd ||
        p.crossfadeFrames > (p.loopEnd - p.loopStart) / 2 ||
        p.loopEnd > std::uint64_t(std::numeric_limits<std::int64_t>::max()))
        return false;
    if (p.samplePath.native().find(std::filesystem::path::value_type{}) !=
            std::filesystem::path::string_type::npos ||
        p.samplePath.native().size() > 32767)
        return false;
    if (p.source == InstrumentSource::sample && p.samplePath.empty())
        return false;
    if (p.source == InstrumentSource::wavetable) {
        if (p.cycle.size() < 16 || p.cycle.size() > 65536 || !isPowerOfTwo(p.cycle.size()))
            return false;
        for (auto value : p.cycle)
            if (!isFinite(value))
                return false;
    }
    return true;
}
bool boundedJson(std::string_view text) noexcept {
    unsigned depth = 0;
    bool string = false, escaped = false;
    for (char c : text) {
        if (string) {
            if (escaped)
                escaped = false;
            else if (c == '\\')
                escaped = true;
            else if (c == '"')
                string = false;
        } else if (c == '"')
            string = true;
        else if (c == '{' || c == '[') {
            if (++depth > 32)
                return false;
        } else if (c == '}' || c == ']') {
            if (!depth)
                return false;
            --depth;
        }
    }
    return !depth && !string;
}
using Value = choc::value::Value;
std::int64_t integer(const Value& v, std::string_view key, std::int64_t low, std::int64_t high) {
    const auto item = v[key];
    if (!item.isInt())
        throw std::runtime_error("integer field");
    const auto n = item.get<std::int64_t>();
    if (n < low || n > high)
        throw std::runtime_error("integer range");
    return n;
}
double number(const Value& v, std::string_view key) {
    const auto item = v[key];
    if (!item.isFloat() && !item.isInt())
        throw std::runtime_error("number field");
    const auto n = item.get<double>();
    if (!isFinite(n))
        throw std::runtime_error("finite field");
    return n;
}
} // namespace
Status serializeInstrumentPreset(const InstrumentPreset& p, std::string& output) noexcept {
    if (!valid(p))
        return Status::invalid_argument;
    try {
        auto object = choc::value::createObject(
            {}, "format", "CyDSPInstrument", "version", 1, "name", p.name, "source", int(p.source),
            "voices", std::int64_t(p.voices), "mpeLower", int(p.lowerMpeMembers), "mpeUpper",
            int(p.upperMpeMembers), "rootNote", int(p.rootNote), "loopStart",
            std::int64_t(p.loopStart), "loopEnd", std::int64_t(p.loopEnd), "crossfadeFrames",
            std::int64_t(p.crossfadeFrames));
        const auto utf8 = p.samplePath.generic_u8string();
        object.setMember("sample",
                         std::string(reinterpret_cast<const char*>(utf8.data()), utf8.size()));
        const auto& s = p.parameters;
        object.setMember("parameters",
                         choc::value::createObject(
                             {}, "waveform", int(s.waveform), "gain", s.gain, "pan", s.pan,
                             "pressureToGain", s.pressureToGain, "stealFadeFrames",
                             std::int64_t(s.stealFadeFrames), "sampleInterpolation",
                             int(s.sampleInterpolation), "attack", s.envelope.attackSeconds,
                             "decay", s.envelope.decaySeconds, "sustain", s.envelope.sustain,
                             "release", s.envelope.releaseSeconds));
        auto cycle = choc::value::createEmptyArray();
        for (auto value : p.cycle)
            cycle.addArrayElement(value);
        object.setMember("cycle", std::move(cycle));
        auto text = choc::json::toString(object);
        output.swap(text);
        return Status::ok;
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (...) {
        return Status::invalid_argument;
    }
}
Status parseInstrumentPreset(std::string_view text, InstrumentPreset& output) noexcept {
    if (text.size() > 4194304)
        return Status::block_too_large;
    if (!boundedJson(text))
        return Status::invalid_argument;
    try {
        const auto object = choc::json::parse(text);
        if (!object.isObject() || object["format"].getString() != "CyDSPInstrument")
            return Status::unsupported_format;
        if (integer(object, "version", 1, INT32_MAX) != 1)
            return Status::unsupported_format;
        InstrumentPreset p;
        p.name = std::string(object["name"].getString());
        p.source = InstrumentSource(integer(object, "source", 0, 2));
        p.voices = std::size_t(integer(object, "voices", 1, 1024));
        p.lowerMpeMembers = unsigned(integer(object, "mpeLower", 0, 15));
        p.upperMpeMembers = unsigned(integer(object, "mpeUpper", 0, 15));
        p.rootNote = unsigned(integer(object, "rootNote", 0, 127));
        p.loopStart = std::uint64_t(integer(object, "loopStart", 0, INT64_MAX));
        p.loopEnd = std::uint64_t(integer(object, "loopEnd", 0, INT64_MAX));
        p.crossfadeFrames = std::uint64_t(integer(object, "crossfadeFrames", 0, INT64_MAX));
        const auto sample = object["sample"].getString();
        p.samplePath = std::filesystem::path(
            std::u8string(reinterpret_cast<const char8_t*>(sample.data()), sample.size()));
        const Value parameters(object["parameters"]);
        auto& s = p.parameters;
        s.waveform = Waveform(integer(parameters, "waveform", 0, int(Waveform::whiteNoise)));
        s.sampleInterpolation =
            SampleInterpolation(integer(parameters, "sampleInterpolation", 0, 1));
        s.gain = number(parameters, "gain");
        s.pan = number(parameters, "pan");
        s.pressureToGain = number(parameters, "pressureToGain");
        s.stealFadeFrames = std::size_t(integer(parameters, "stealFadeFrames", 0, 1048576));
        s.envelope = {number(parameters, "attack"), number(parameters, "decay"),
                      number(parameters, "sustain"), number(parameters, "release")};
        const auto cycle = object["cycle"];
        if (!cycle.isArray() || cycle.size() > 65536)
            return Status::invalid_argument;
        for (std::uint32_t i = 0; i < cycle.size(); ++i) {
            if (!cycle[i].isFloat() && !cycle[i].isInt())
                return Status::invalid_argument;
            p.cycle.push_back(cycle[i].get<double>());
        }
        if (!valid(p))
            return Status::invalid_argument;
        output = std::move(p);
        return Status::ok;
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (...) {
        return Status::invalid_argument;
    }
}
Status loadInstrumentPreset(const std::filesystem::path& path, InstrumentPreset& output) noexcept {
    try {
        std::ifstream input(path, std::ios::binary | std::ios::ate);
        if (!input)
            return Status::io_error;
        const auto n = input.tellg();
        if (n < 0 || n > 4194304)
            return Status::block_too_large;
        std::string text(std::size_t(n), '\0');
        input.seekg(0);
        input.read(text.data(), n);
        return input ? parseInstrumentPreset(text, output) : Status::io_error;
    } catch (...) {
        return Status::io_error;
    }
}
Status saveInstrumentPreset(const std::filesystem::path& path, const InstrumentPreset& p) noexcept {
    std::string text;
    const auto status = serializeInstrumentPreset(p, text);
    if (status != Status::ok)
        return status;
    try {
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        out.write(text.data(), std::streamsize(text.size()));
        out.flush();
        return out ? Status::ok : Status::io_error;
    } catch (...) {
        return Status::io_error;
    }
}
template <Sample T> struct Instrument<T>::State {
    SampleData<T> sample;
    Wavetable<T> table;
    Synthesizer<T> synth;
};
template <Sample T> Instrument<T>::Instrument() = default;
template <Sample T> Instrument<T>::~Instrument() = default;
template <Sample T> Instrument<T>::Instrument(Instrument&&) noexcept = default;
template <Sample T> Instrument<T>& Instrument<T>::operator=(Instrument&&) noexcept = default;
template <Sample T>
Status Instrument<T>::prepare(const ProcessSpec& spec, const InstrumentPreset& p,
                              const std::filesystem::path& base, CpuBackend backend,
                              std::size_t maximumFrames) noexcept {
    if (!valid(p) || !maximumFrames)
        return Status::invalid_argument;
    try {
        auto next = std::make_unique<State>();
        auto r = next->synth.prepare(spec, p.voices, backend);
        if (r != Status::ok)
            return r;
        r = next->synth.setParameters(p.parameters);
        if (r != Status::ok)
            return r;
        r = next->synth.setMpeZones(p.lowerMpeMembers, p.upperMpeMembers);
        if (r != Status::ok)
            return r;
        if (p.source == InstrumentSource::sample) {
            AudioFileReader reader;
            r = reader.open(p.samplePath.is_absolute() ? p.samplePath : base / p.samplePath);
            if (r != Status::ok)
                return r;
            const auto info = *reader.info();
            if (p.loopEnd > info.frames)
                return Status::invalid_argument;
            if (info.frames > maximumFrames || info.frames == 0)
                return Status::block_too_large;
            AudioBuffer<T> audio;
            r = audio.resize(info.channels, std::size_t(info.frames));
            if (r != Status::ok)
                return r;
            for (std::size_t at = 0; at < info.frames; at += 4096) {
                std::size_t n = 0;
                r = reader.read(at,
                                audio.block().subBlock(
                                    at, std::min<std::size_t>(4096, std::size_t(info.frames) - at)),
                                n);
                if (r != Status::ok)
                    return r;
            }
            r = next->sample.prepare(audio.block(), info.sampleRate, p.rootNote, backend);
            if (r != Status::ok)
                return r;
            r = next->sample.setLoop(std::size_t(p.loopStart), std::size_t(p.loopEnd),
                                     std::size_t(p.crossfadeFrames));
            if (r != Status::ok)
                return r;
            r = next->synth.setSample(&next->sample);
            if (r != Status::ok)
                return r;
        } else if (p.source == InstrumentSource::wavetable) {
            std::vector<T> cycle(p.cycle.size());
            std::transform(p.cycle.begin(), p.cycle.end(), cycle.begin(),
                           [](double v) { return static_cast<T>(v); });
            r = next->table.prepare(cycle, backend);
            if (r != Status::ok)
                return r;
            r = next->synth.setWavetable(&next->table);
            if (r != Status::ok)
                return r;
        }
        state_.swap(next);
        return Status::ok;
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (...) {
        return Status::io_error;
    }
}
template <Sample T>
Status Instrument<T>::process(AudioBlock<T> out, const MidiBuffer* midi, const UmpBuffer* ump,
                              bool add) noexcept {
    return state_ ? state_->synth.process(out, midi, ump, add) : Status::not_prepared;
}
template <Sample T> void Instrument<T>::reset() noexcept {
    if (state_)
        state_->synth.reset();
}
template <Sample T> Synthesizer<T>* Instrument<T>::synthesizer() noexcept {
    return state_ ? &state_->synth : nullptr;
}
template class Instrument<float>;
template class Instrument<double>;
} // namespace cydsp
