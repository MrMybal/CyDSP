# CyDSP 0.1.0 — version notes

[Français](VERSION.fr.md) · [README](README.md) · [Issue tracker](https://github.com/MrMybal/CyDSP/issues)

## Current version and availability

- **Component:** CyDSP C++20 static audio DSP library.
- **Source version:** `0.1.0`, as declared by `project(CyDSP VERSION 0.1.0)` in [CMakeLists.txt](CMakeLists.txt).
- **Stage:** in development. This identifies the current source implementation, not a stable binary release.
- **Availability:** private source repository; no published release, tag or downloadable application/library binary.
- **License:** [MIT](LICENSE), copyright © 2026 MrMybal CyberAlien. Third-party notices retain their original attribution.

## Included in the current source

- Common planar buffers, layout conversion, channel matrices and preparation/status conventions for `float` and `double`.
- Gain ramps, FIR/IIR filters, cascades, voice banks and smoothed EQ transitions.
- Complex/real FFTs, vector kernels, windows and integer/fractional delay.
- Uniform, hybrid, nonuniform and progressive convolution, shared-FFT MIMO and prepared or asynchronous IR publication.
- Rational resampling, prepared variable-rate ramps, end-of-stream draining and prepared routing graphs.
- Private scalar/SSE2/AVX2/AVX-512/NEON/SVE implementations selected according to the compiled target and CPU/OS capabilities.
- Generators, dynamics, filter design, oversampling and modulation/reverb effects.
- Polyphonic synthesis/sampling/custom voices, MPE/RPN, UMP/MIDI 2 messages, tempo and queued transport.
- Optional file/device/Stretch/Sources targets; file workers reuse CyDSP sinc/vector/buffer primitives. Stretch uses adapted MIT algorithms with CyDSP FFT/STFT integration.
- CMake static target, package installation and bundled first-party MIT license with retained upstream notices.

See [module contracts](docs/Modules.md) for the exact supported behavior. These notes describe the current source, without assigning a release date or promising future features.

## Documented limitations

- x64 and ARM64 are the primary targets. x86/ARM32 remain secondary compatibility targets.
- Native checks have run on Windows x64/x86 and Quest 3 Android ARM64/ARM32. Desktop Linux/macOS and Windows ARM64 application runtime still need their own target environments.
- AVX-512 has run on the available compatible x64 CPU. SVE compiles/links for Android ARM64 but still needs native execution on SVE hardware, including different vector lengths.
- ARM64EC is explicitly rejected. ARM32 double-precision SIMD work falls back to scalar.
- Optional audio/MIDI-device, audio-file and pitch/time modules are available; the Unreal adapter is not included. Listening quality and physical audio latency have not been established by the numerical checks.
- The sampler offers native linear reads or finite filtered mip levels, plus loop crossfades; see the quality limits in [DAW integration](docs/DawIntegration.md). Native UMP adapters, MIDI-CI and Stream Configuration are implemented; physical MIDI2 interoperability and Apple build/runtime acceptance remain unverified. See [MIDI 2](docs/Midi2.md) for exact protocol/application-policy boundaries.
- Progressive convolution spreads late work and can cost more average CPU. Performance depends on the workload, configuration and target; no universal speedup is promised.
- Mutable processor instances require the ownership and serialization rules described in [the architecture guide](docs/Architecture.md); asynchronous IR publication has its own explicit producer/consumer contract.

## Known issues and feedback

There is no published release-specific bug list. This does not imply that the library is free of bugs. Check or report problems in the [private issue tracker](https://github.com/MrMybal/CyDSP/issues), including the target ABI, compiler, processing configuration and a small reproducer. Repository authorization is required.

The limits above describe coverage and integration boundaries, rather than confirmed issue tickets. Detailed executed checks and measured cases remain separate in [Validation](docs/Validation.md) and [Measurements](docs/Measurements.md).

The expanded DSP and optional module scope, delivered synthesis/MPE/source infrastructure and remaining format/platform coverage are tracked in the [coverage inventory](docs/Coverage.md). No complete framework parity is claimed.

The portable CI property service adds ResourceList/DeviceInfo/ChannelList/JSONSchema,
custom instances, GET and full/partial SET, pagination, retained replies and
notify/full/partial subscriptions. Its prepared client adds TTL caching and
coalesced refresh with stale-GET protection. [Service contracts](docs/CiProperty.md)
record schema/host policies and physical acceptance limits. macOS/Linux desktop
tests are deferred by the owner.

November12 DAW expansion: [contracts and scope](docs/DawIntegration.md) cover DSP, synthesis/presets, playback, devices/MIDI and files/codecs. macOS/Linux desktop validation remains deferred.

Integration-report remediation adds public WAV metadata, device capabilities/identity/clocks, multiport/SysEx MIDI, PCM editing proxies and source policies. The [Plugins module](docs/Plugins.md) hosts CLAP/VST3 with native editors and a separate worker. Additional formats/application-specific modes use explicit providers; CyDAW migration and physical acceptance remain separate.
