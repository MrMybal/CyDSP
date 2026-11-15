# Controlling DSP from MIDI

The application translates MIDI messages into DSP parameter changes or
synthesis events. Use the prepared parameter/event primitives when sample timing
is required; serialize changes according to each processor's ownership contract.
See [parameter and processing contracts](Modules.md) and
[architecture](Architecture.md). MIDI devices and protocols are application services.
