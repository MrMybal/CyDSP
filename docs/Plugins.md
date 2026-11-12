# Plugin hosting and native editors

Link CyDSP::Plugins (CYDSP_BUILD_PLUGINS, on by default). Production creates a static
archive; fixture plugin modules and the fixture worker executable are built only
with the explicit CYDSP_BUILD_TESTS option. No installed third-party plugin is
scanned or opened by the regression suite.

## Common original contract

PluginProvider scans an explicitly selected module and creates a PluginInstance.
The instance owns the backend; instances retain module initialization/factory
ownership until the last reference is destroyed. Scanning/loading executes foreign
code. Use the separate worker for processing isolation; an in-process scan is not
isolated. Relative module paths are rejected. CLAP modules and VST3 binaries/bundles
use the platform loader; Windows dependency lookup uses the module directory and
default trusted loader directories.

The instance creator is the UI owner for activation, state, parameter text, native
editor and pumpMainThread calls. On Windows/macOS it must be the application's UI
thread. The application supplies its platform window/event loop. A single
serialized symbolic audio owner calls start/process/stop/reset. No two calls on
that audio owner may overlap. UI operations requiring stopped processing return
transition_in_progress. Stop the audio owner before deactivation, replacement,
move or destruction. Optional VST3 reset returns unsupported_format because VST3
has no universal realtime processor-reset operation.

PluginProcess accepts distinct input/output bus arrays of AudioBlock views.
Sub-block offsets are retained, precision is selected during activate, bus counts
and channels must match, and event offsets must be inside the block. No silent
sidechain downmix occurs. PluginEventBuffer has prepared event/payload capacities,
keeps sample order and copies foreign SysEx payloads. Do not share one mutable
event buffer between UI/audio threads; publish prepared events through an owned
queue. The host's tested process path allocates/frees no memory; this cannot
guarantee the behavior of arbitrary foreign plugins.

Parameters have stable IDs, names, ranges, defaults and flags. CLAP keeps plain
parameter values. VST3 marks parameters normalized and uses [0,1], including
nonlinear controller mappings; values are never falsely treated as linear physical
units. Parameters, notes, gestures, expression, MIDI/SysEx/UMP and transport use
explicit event kinds. Unsupported format translations return unsupported_format.
VST3 MIDI note/CC/pressure/pitch conversion uses its native event/IMidiMapping
contracts; controller mappings are prepared on UI, never queried from process.
CLAP-specific per-note modulation, note-choke/end and raw UMP have no universal
VST3 mapping. VST3 transport is block-level; split a block at a transport change
rather than silently dropping a mid-block event.

State streams are bounded (instance default/maximum64 MiB), use short-read/write
semantics and preserve component plus controller state for VST3. CyDSP's version1
VST3 state envelope has a signature and exact component/controller lengths. It is
not a Steinberg .vstpreset file. VST3 inactive flush queues processor updates for the
next audio block; save returns would_block while UI edits are still pending.
Foreign loadState implementations may mutate themselves before returning failure;
the wrapper cannot make arbitrary plugin code transactional.

Latency/tail validity and infinite tail are explicit. They describe processor
latency, separate from audio hardware and the worker pipeline. pumpMainThread
delivers deferred callbacks, requests/rescans, UI parameter synchronization and
CLAP timers/POSIX fd events. The application must service restart/port/parameter
requests by stopping/reconfiguring/refreshing as appropriate.

## Native editor lifecycle

Create the parent window/view in the application. openEditor checks the selected
Win32/Cocoa/X11/Wayland capability, creates and attaches/shows the plugin editor,
and reports its actual dimensions. Embedded Wayland CLAP is rejected (the protocol
supports floating there). Do not pass a destroyed/foreign-process Cocoa view.
On resize, the plugin constrains the requested size. DPI is explicit; Cocoa CLAP
uses logical units and does not accept forced scaling. Close the editor before
destroying its parent or instance.

CLAP negotiates GUI extensions and forwards main-thread parent resize requests.
The application resize callback must actually resize its parent client area before
returning true. Other-thread requests can be rejected; no unapplied size is
acknowledged. VST3 uses IPlugView/IPlugFrame and optional content-scale support;
resize callbacks call onSize in the same call stack. Visibility of its parent
belongs to the application, so showEditor can return unsupported_format.
VST3 focus, UTF-16 key events, portable key/modifier codes and wheel events are
forwarded explicitly. CLAP receives its native window messages; those optional
VST3 forwarding calls are not emulated. A generic parameter UI can be drawn by
CyUI/ImGui using description().parameters and prepared parameter/gesture events;
CyDSP does not import or impose a GUI renderer.

## Separate-process worker

PluginWorker launches a caller-supplied executable. That executable recognizes
--cydsp-worker <mapping> and calls runPluginWorker(mapping); the opt-in original
fixture shows this entry point. The child loads/activates the plugin, owns its UI
and serialized audio calls, and pumps Windows messages. Windows/POSIX memory maps
and child lifetime are original CyDSP code; Android requires an application service
provider and returns unsupported_backend for this standalone worker.

The protocol checks magic/version, native ABI/word size/record size, dimensions and
all packet/payload bounds. It contains no cross-process C++ object or payload
pointers. Client and worker must be built from the same protocol/target ABI.
Memory is bounded by maximumChannelsPerDirection/events/payload/state bytes and
an overall512 MiB limit. Only the child and owned mapping are cleaned up.
The worker exits when shutdown is requested or its parent disappears.

The audio client uses a nonblocking mailbox, prepared SIMD copies and a fixed
block size equal to maximumFrames. It never waits, launches, performs OS polling
or allocates. Pipeline latency is exactly one requested block when the child meets
its deadline; add the reported plugin latency in the application's routing graph.
The first block is silent. Missing/late replies produce silence/would_block;
stale replies are discarded, preserving timeline sequence. Inspect
PluginWorkerBlockResult. poll on control detects an exited child (and a stalled heartbeat) and reports
disconnected/timed_out. Relaunch and reload a saved state to recover.

Control/state/editor calls require audio idle and may wait up to their timeout.
A stuck owned child is terminated on control timeout/cleanup. Client control waits
pump Windows messages so a cross-process child editor can attach/detach correctly.
No callback may destroy/reenter the worker during a control call. Never call close
or control RPC from audio.

Cross-process embedding is implemented for Win32 HWND only. Cocoa/X11 opaque parent
objects are not treated as shared addresses; non-Windows embedded editors return
unsupported_backend. The worker forwards scale/show/focus/key/wheel where the
format supports them. Plugin-initiated resize proposals are rejected synchronously,
published as pluginEditorResize plus requestedEditorSize(), then decided by the
parent UI; the host may request a supported constrained resize afterward.
In-process editor callbacks support immediate acceptance. This is crash/lifetime
isolation, not a security sandbox for malicious code. In-process scanning remains
an explicit foreign-code operation; an isolated discovery/catalogue service is
application work.

## Format availability and validation

Built-in adapters: CLAP and current MIT VST3 interfaces. The public
PluginProvider/PluginBackend boundary also accepts application-owned AU, AAX,
LV2 or JSFX adapters without borrowing JUCE types. No built-in support for those
formats is claimed. AAX/ASIO/VST2 SDKs are not bundled; their separate access terms
are not granted by CyDSP MIT. Plugin exports already provided by an application
remain separate from this host API.

Windows acceptance loads original CLAP/VST3 modules and tests float/double,
sidechain, sample-offset automation, copied12 KiB SysEx, normalized parameters,
messages, states, lifecycle and actual hidden-parent child windows, resize/DPI,
focus/keys/wheel. The worker test checks copied audio/events, bounded nonblocking
late replies, state/editor operations, controlled abrupt plugin exit and relaunch.
Its C++ allocation audit covers the host code reached by the test, not all code
inside foreign DLLs. macOS/Linux desktop, real third-party plugin compatibility,
AU/AAX, native Android service workers and Windows ARM64 runtime remain separate
acceptance work. Source Apple bundle loading uses CoreFoundation and is not a
macOS compilation/runtime claim.

Native VST3 UI gestures are copied into the next audio output event buffer for
recording; the optional edited callback also exposes them on the UI thread.
Parameter descriptions retain native units. Audio-to-controller parameter mirrors
use atomic32 mailboxes per parameter, preserving the final value when the UI
pumps slowly. An original fixture checks 4096 automation points in-process and UI gestures
both in-process and across the worker mailbox.
