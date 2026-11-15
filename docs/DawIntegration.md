# Using CyDSP in a DAW

Prepare processors outside the audio callback, then pass planar audio blocks
through the public C++ API. The application owns transport, devices, file I/O,
MIDI and plugin hosting. CyAudioCore is one consumer of CyDSP. See
[library integration](Architecture.md#library-integration),
[module contracts](Modules.md) and the [processing example](../examples/Processing.cpp).
