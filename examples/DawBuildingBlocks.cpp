#include <cydsp/CyDSP.h>

// Original CyDSP example. No device is opened or file read by compilation.
// Control thread: prepare/prefill before callbacks. Stop callbacks before destruction.
struct DawBuildingBlocks {
    cydsp::ReadAheadSource<float> file;
    cydsp::AudioTransport<float> transport;
    cydsp::Synthesizer<float> synth;
    cydsp::MidiEventCollector collector;
    cydsp::MidiBuffer midi;
    cydsp::UmpBuffer ump;

    cydsp::Status prepare(const std::filesystem::path& path) {
        cydsp::ReadAheadOptions options;
        options.outputRate = 48000;
        options.outputChannels = 2;
        options.pageFrames = 2048;
        options.pages = 8;
        options.durationRatio = 1;
        options.pitchSemitones = 0;
        auto status = file.prepare(path, options);
        if (status != cydsp::Status::ok)
            return status;
        const cydsp::ProcessSpec spec{48000, 512, file.info().channels};
        status = transport.prepare(spec, file);
        if (status != cydsp::Status::ok)
            return status;
        status = synth.prepare(spec, 64);
        if (status != cydsp::Status::ok)
            return status;
        status = collector.prepare();
        if (status != cydsp::Status::ok)
            return status;
        status = midi.prepare(256, 4096);
        if (status != cydsp::Status::ok)
            return status;
        status = ump.prepare(256);
        if (status != cydsp::Status::ok)
            return status;
        return file.prefill(0, 4096); // May wait; control thread only.
    }
    // One control producer calls enqueue; the audio callback consumes commands.
    cydsp::Status play() noexcept {
        return transport.enqueue({cydsp::TransportAction::play});
    }
    // Device/control producer calls collector.pushMidi/pushUmp with steady-clock ns.
    // Audio owner supplies the matching callback anchor; lifecycle changes require
    // callbacks idle. Retain events on output saturation before calling again.
    cydsp::Status process(cydsp::AudioBlock<float> output, cydsp::AudioClockAnchor clock) noexcept {
        auto status =
            collector.collect(clock, static_cast<std::uint32_t>(output.frames()), midi, ump);
        if (status != cydsp::Status::ok)
            return status;
        status = transport.process(output, cydsp::UnderrunPolicy::silenceAndAdvance);
        if (status == cydsp::Status::ok)
            status = synth.process(output, &midi, &ump, true);
        midi.reset();
        ump.reset();
        return status;
    }
};
