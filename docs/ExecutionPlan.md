# Preparing a DSP processing chain

Select the sample rate, maximum block size and channel count through
`ProcessSpec`. Prepare processors and buffers before processing, then account for
each processor's latency and tail when connecting branches. See
[lifecycle and memory](Architecture.md#lifecycle-and-memory),
[module contracts](Modules.md) and the [processing example](../examples/Processing.cpp).
