# Native MIDI 2 and MIDI-CI

CyDSP owns the UMP transport adapter and MIDI-CI state machines. No JUCE code is
used. Link `CyDSP::CyDSP` for `MidiCiDevice` and `UmpEndpointNegotiator`; add
`CyDSP::Devices` for `Midi2Device`. The [compile-only example](../examples/Midi2Session.cpp)
connects their public APIs. Compiling it opens no device.

## Native transports

| Backend | Interface and requirements | Validation in this workspace |
| --- | --- | --- |
| Windows | Current Windows MIDI Services native COM UMP transport, bidirectional endpoint discovery via SetupAPI; compatible service/transport must already be installed | MSVC x64/x86 and Clang x64 build/link/run; this Windows 10 host has no service, so physical discovery returns device_unavailable |
| Apple | CoreMIDI event lists/protocol-aware ports, macOS 11+/iOS 14+ | Source implemented; no Apple SDK/host available to compile or execute it |
| Linux | Nonblocking native kernel `/dev/snd/umpC*D*` character devices | The same POSIX descriptor/framing code executes on Quest sockets; no desktop ALSA UMP hardware exercised |
| Android | Runtime-loaded AMidi UMP, API 33+, host Java MidiManager device and permissions | Four NDK ABIs build; ARM64/ARM32 core, queues and POSIX bridge execute on Quest; no Java AMidi device opened |
| Explicit descriptors | Caller-owned nonblocking UMP character devices or SIGPIPE-protected sockets; CyDSP duplicates descriptors | Fragmented input, output saturation, byte order, descriptor ownership and disconnect exercised on Quest |
| Explicit loopback | Bounded local validation transport, never selected as an automatic hardware fallback | Concurrent notes/SysEx, queue saturation and a two-endpoint CI roundtrip execute on Windows and Quest |

The Windows adapter is based on the native ABI at Microsoft/MIDI revision
`76a7fd4730416e634e70de555fdc1333a243d850`, recorded with its exact MIT files in
the manifest. No service, driver, Windows App SDK or system MIDI configuration is
installed by CyDSP. Older preview service contracts are not established compatible.
CoreMIDI enumerates sources and destinations separately; open separate receiving
and sending instances and route CI replies to the corresponding output. An Apple
build needs the platform SDK and Blocks support. Linux needs a kernel/device driver
exposing UMP nodes and permission to open them. Legacy RtMidi remains a separate
MIDI 1 API. No conversion to WinMM is presented as native MIDI 2.

Android enumeration and permission handling belong to the application. On API33+,
use `MidiManager.getDevicesForTransport(MidiManager.TRANSPORT_UNIVERSAL_MIDI_PACKETS)`
and `openDevice`, then call `openAndroid(JNIEnv*, jobject, receivePort, sendPort)`
from native host code. A device output port is CyDSP input, and a device input port
is CyDSP output; -1 disables a direction. The device must expose a UMP default
protocol, including MIDI 1 over UMP. CyDSP retains a global Java reference and opens
AMidi ports on its worker. Close CyDSP before closing the Java MidiDevice or JVM.
No APK is required or installed by the validation runner.

## Ownership, timing and queue policy

`open`, re-open, moves, `close` and destruction are serialized control-thread
operations after producers/consumer are idle. Failed open leaves the old connection
intact. Opening prepares all queues and starts a dedicated worker. Closing joins
that worker and discards unsent jobs; it is not a drain acknowledgement.

`send` has exactly one producer, usually the audio thread. `sendSysEx7` has a
separate single producer, usually the MIDI-CI control thread. `read` has exactly
one consumer. These three methods do no native I/O, locking, allocation or freeing.
The two producer queues are independent; a whole SysEx payload is copied atomically
and fragmented on the worker. Real-time code must handle update_queue_full without
spinning. Success acknowledges queuing, not physical delivery. Native errors are
available through lastError; incoming overflow/malformed packets increment
`droppedPackets`. The OS backend itself is not claimed allocation-free.

Timestamps are nanoseconds in the same steady-clock domain as `midi2TimestampNow`;
zero means immediate. Each producer is FIFO, and future timestamps should be
monotonic within that producer. Windows converts to QPC ticks, Apple converts to
host ticks, Android uses monotonic nanoseconds, and descriptor/loopback backends
schedule on the worker. A 1ms worker poll is not a hard real-time timing guarantee.
The host maps received times to its audio clock and queues sample-offset events.
JR capability flags do not provide audio-clock synchronization or interpret JR ticks.

UMP words are CPU-native uint32 values in the public API and Linux descriptor
transport. Serialized UMP bytes and the Android API use big-endian words. Do not
pass USB transfer bytes directly to openFileDescriptors: a host USB adapter must
convert them. Raw SysEx7 and CI jobs sharing a group cannot interleave their SysEx
fragments; voice packets may occur between fragments. A raw start must eventually
be followed by its end or the host must close the stream. All raw fragments for one
group belong to the same producer. CI processing for multiple groups needs one
MidiCiDevice per group.

## MIDI-CI services

`prepare` creates bounded peer, profile, transaction and payload storage; it may
allocate and obtain a random session seed. All subsequent processing, poll and
reset retain that memory. The owner serializes CI calls and supplies monotonic
milliseconds. `CiCallbacks::send` must enqueue/copy the borrowed payload and return
Status; it must not synchronously recurse into this device. Other callback views
are borrowed only until return. Do not reprepare, move or destroy CI inside a
callback; replying through sendProperty from property() is supported. Application callbacks must retain the same
allocation/no-wait contract if run in a real-time context. CI is normally handled
on a control thread, with musical events passed separately to audio.

Delivered wire/state-machine services:

- Seven-bit framing, independent MUID encoding, reserved IDs and maximum-message
  validation. SysEx payload APIs exclude F0/F7; parse accepts complete wire SysEx too.
- Discovery inquiry/reply, peer cache, endpoint product-instance ID reply,
  broadcast invalidation and MUID collision recovery with a new session ID.
- Profile lists and profile on/off requests/reports. Local profiles are prepared
  up front; remote lists/reports call profile() and raw messages also call message().
  The host performs the actual profile-specific instrument/controller behavior.
- Property Exchange capabilities, GET/SET/subscription/Notify framing, bounded
  interleaved assembly, unknown chunk counts, shortened final transfers, abort,
  request-ID correlation and 3-second request/inter-chunk expiration. Progress on
  long outgoing requests refreshes the reply deadline.
- Retained outgoing property transfers that resume from poll when transport queues
  are full, with bounded work per turn and lastSendError reporting. sendProperty
  copies header/data before returning. lastSendError is latched until reset.
  requestProperty allocates IDs for inquiries;
  replies reuse the received ID. Call poll regularly even if no input arrives.
- Mcoded7 binary encoding/decoding with independent bit-order and partial-group
  validation. Data handed to sendProperty is already seven-bit encoded.
- Unsupported/deprecated requests receive NAK. Process Inquiry capabilities report
  zero optional MIDI message-report features; unimplemented features are not advertised.

The default manufacturer 0x7D is for development/non-commercial use: an application
shipping hardware/software must configure its appropriate identity. CyDSP's MIT
license does not assign a manufacturer identity. maximumSysExBytes in CI discovery
includes F0/F7, whereas the native queued payload limit excludes them.

[The property service](CiProperty.md) supplies ResourceList/DeviceInfo/ChannelList/
JSONSchema, custom instances, GET/full/partial SET, pagination, retained replies,
notify/full/partial subscriptions and a prepared initiator/cache. The core
interprets PE Notify 100/144/408 for keepalive/cancellation/timeout. Applications
can use these helpers or keep their own property callback. Resource/schema/access
definitions and preset/DSP mutation remain host policy; zlib remains unsupported.
Specialized
Profile Details and Profile Specific Data pass through message() for host handling.
CiProfileDispatcher applies explicitly registered profile behaviors at an audio boundary; unknown instrument semantics remain host policy.

## UMP endpoint negotiation

`UmpEndpointNegotiator` handles UMP 1.1 endpoint information discovery and Stream
Configuration request/notification, selecting MIDI 1 or MIDI 2 only after the
peer's confirmation. It validates capabilities, supports timeout and reports
unsupported requests. Its minimal endpoint declares zero Function Blocks. It does
not own endpoint metadata. UmpEndpointService adds naming, identity and Function Block discovery/notifications with retained replies.
UmpExtensions adds typed Flex/Stream helpers and bounded text assembly. See [DAW integration contracts](DawIntegration.md) for clock collection, reconnect epochs, controllers and profile dispatch.

OS-managed endpoints normally negotiate their protocol through the operating
system. Do not run a second negotiator indiscriminately on such a connection.
Legacy MIDI-CI Protocol Negotiation is deprecated in MIDI-CI 1.2 and receives NAK;
current protocol selection uses the UMP Stream mechanism.

## Evidence and primary references

The new regressions include independent wire constants and Mcoded7 vectors,
malformed/out-of-order PE chunks, collisions, profiles, 4096-byte replies under
backpressure, expiration, confirmation-before-switch and no-allocation audits.
They include a complete CI discovery through two explicit native loopbacks.
The final digest/build/runtime evidence is in [Validation](Validation.md).
Physical device interoperability and hardware timestamps still need acceptance
with actual MIDI 2 equipment. No physical MIDI port, microphone or speaker is
opened by these tests. Apple source has no compile/runtime acceptance in this session.

- [MIDI-CI 1.2 specification](https://amei-music.github.io/midi2.0-docs/amei-pdf/M2-101-UM_v1-2_MIDI-CI_Specification.pdf).
- [Common Property Exchange Rules 1.1](https://amei.or.jp/midistandardcommittee/MIDI2.0/MIDI2.0-DOCS/M2-103-UM_v1-1_Common_Rules_for_MIDI-CI_Property_Exchange.pdf).
- [UMP and MIDI 2 Protocol 1.1.2](https://amei-music.github.io/midi2.0-docs/amei-pdf/M2-104-UM_v1-1-2_UMP_and_MIDI_2-0_Protocol_Specification.pdf).
- [Windows MIDI Services](https://microsoft.github.io/MIDI/).
- [Linux kernel UMP design](https://www.kernel.org/doc/html/next/sound/designs/midi-2.0.html).
- [Android AMidi reference](https://developer.android.com/ndk/reference/group/midi).
- [CoreMIDI protocol-aware input](https://developer.apple.com/documentation/coremidi/midiinputportcreatewithprotocol(_:_:_:_:_:)).
