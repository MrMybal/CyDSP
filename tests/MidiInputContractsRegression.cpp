#include <cydsp/CyDSP.h>
#include <iostream>
#include <stdexcept>
#include <source_location>
namespace {
using namespace cydsp;
void require(bool v, const std::source_location here = std::source_location::current()) {
    if (!v)
        throw std::runtime_error("MIDI input integration contract line " + std::to_string(here.line()));
}
void ok(Status r, const std::source_location here = std::source_location::current()) {
    require(r == Status::ok, here);
}
} // namespace
void midiInputContractsRegression() {
    MidiDevicePorts ports;
    ports.backend = "fixture";
    ports.inputs = {{8, "Same", "idA", true}, {2, "Same", "idB", true}};
    MidiPortSelector selector;
    selector.backend = "fixture";
    selector.name = "Same";
    std::uint32_t index = 99;
    require(resolveMidiPort(ports, selector, index) == Status::ambiguous && index == 99);
    selector.persistentId = "idB";
    ok(resolveMidiPort(ports, selector, index));
    require(index == 2);
    std::swap(ports.inputs[0], ports.inputs[1]);
    ports.inputs[0].index = 42;
    ok(resolveMidiPort(ports, selector, index));
    require(index == 42);
    std::string serialized;
    ok(serializeMidiPortSelector(selector, serialized));
    MidiPortSelector restored;
    ok(parseMidiPortSelector(serialized, restored));
    require(restored.name == selector.name && restored.persistentId == "idB");
    selector.persistentId = "gone";
    require(resolveMidiPort(ports, selector, index) == Status::device_unavailable);
    selector.allowNameFallback = true;
    require(resolveMidiPort(ports, selector, index) == Status::ambiguous);
    MidiInput input;
    require(input.open(0, MidiInputOptions{1, 1048577}) == Status::invalid_argument);
    MidiInputBank bank;
    ok(bank.prepare({2, 8, 16384}));
    std::vector<std::uint8_t> sysex(12000, 0x12);
    sysex[0] = 0xf0;
    sysex.back() = 0xf7;
    ok(bank.ingest(0, 1000000000, {sysex.data(), 6000}));
    const std::uint8_t realtime[]{0xf8};
    ok(bank.ingest(0, 1000000001, realtime));
    ok(bank.ingest(1, 1000020834, midiNoteOn(1, 64, 100).bytes()));
    ok(bank.ingest(0, 1000041667, {sysex.data() + 6000, 6000}));
    ok(bank.collect({1000000000, 0, 48000}, 64));
    auto a = bank.block(0), b = bank.block(1);
    require(a.midi->size() == 2 && b.midi->size() == 1);
    require(a.midi->event(0).bytes[0] == 0xf8 && a.midi->event(1).bytes.size() == 12000);
    require(a.midi->event(1).frame == 2 && b.midi->event(0).frame == 1);
    // Queued old-epoch events and partial SysEx cannot leak through reconnection.
    ok(bank.ingest(1, 10000000000, midiNoteOn(2, 80, 100).bytes()));
    ok(bank.ingest(1, 1000000000, {sysex.data(), 6000}));
    ok(bank.invalidate(1));
    ok(bank.collect({1000000000, 64, 48000}, 64));
    b = bank.block(1);
    require(b.releaseVoices && b.generation == 2 && b.midi->size() == 48);
    for (std::size_t i = 0; i < 48; ++i)
        require(b.midi->event(i).frame == 0);
    ok(bank.collect({1000000000, 128, 48000}, 64));
    require(bank.block(1).midi->size() == 0 && !bank.block(1).releaseVoices);
    std::cout << "PASS integration MIDI selection/multiport/SysEx/epochs/voice releases\n";
}
