# Consolidate fresh executed validation reports; never runs project programs.
from datetime import datetime,timezone
import json
from pathlib import Path
import re
from reporting import source_digest
ROOT=Path(__file__).resolve().parents[1]
digest=source_digest()
paths=['build/simd-validation.json','build/validation.json','build/native-x64.json','build/native-x86.json','build/native-scalar.json',
       'build/native-x64-clang.json','build/native-x64-asan.json',
       *['build/android-'+abi+'/report.json' for abi in ['armeabi-v7a','arm64-v8a','x86','x86_64']],
       'build/device-arm64-v8a.json','build/device-armeabi-v7a.json']
reports={}
for name in paths:
    report=json.loads((ROOT/name).read_text(encoding='utf-8'))
    if report.get('status')!='passed' or report.get('sourceSha256')!=digest:
        raise SystemExit('Failed or stale report: '+name)
    reports[name]=report
for name in ['windows-x64','windows-x86','windows-x64-scalar','windows-arm64-clang']:
    cache=(ROOT/'build'/name/'CMakeCache.txt').read_text(encoding='utf-8')
    for flag in ['CYDSP_BUILD_TESTS','CYDSP_BUILD_MEASUREMENTS','CYDSP_SANITIZE_ADDRESS']:
        if flag+':BOOL=OFF' not in cache:raise SystemExit('Production flag enabled: '+name+' '+flag)
for name,report in reports.items():
    if name.startswith(('build/native-','build/device-')) and not all(marker in report.get('regressions','') for marker in ('PASS expanded DSP/MIDI/SMF/SPSC/realtime contracts','PASS audio formats/virtual devices/pitch-time','PASS integrated FFT/synthesis/MPE/UMP/transport','PASS read-ahead/resampled seeks/background recording','PASS CI resources/GET/SET/subscriptions/Notify/realtime','PASS CI partial/instances/pagination/ChannelList/JSONSchema/client-cache/realtime','PASS MIDI-CI/UMP negotiation/profiles/properties','PASS native UMP queues/bridges','PASS DAW expansion DSP/wavetable/sampler/presets/realtime','PASS DAW MIDI clock/reconnect/Flex/Stream/controllers/profiles/realtime','PASS DAW AIFC/CAF/markers/loops/MP3 fixture')):
        raise SystemExit('Missing expanded DSP/I/O regressions: '+name)
    if name.startswith(('build/native-','build/device-')) and 'PASS zero allocations/frees' not in report.get('regressions',''):
        raise SystemExit('Missing allocation audit: '+name)
    if name.startswith(('build/native-','build/device-')) and 'PASS shared FFT/scheduling/prewarm/variable rate/fused graph' not in report.get('regressions',''):
        raise SystemExit('Missing new optimization regressions: '+name)
    if name.startswith(('build/native-','build/device-')) and 'PASS staged FFT/incremental convolution/configured IR engines' not in report.get('regressions',''):
        raise SystemExit('Missing staged FFT/scheduled convolution regressions: '+name)
    if name.startswith(('build/native-','build/device-')) and 'PASS progressive shared MIMO/convolution engines' not in report.get('regressions',''):
        raise SystemExit('Missing progressive shared MIMO regressions: '+name)
    if name.startswith('build/device-') and not report.get('temporaryFilesRemoved'):
        raise SystemExit('Device cleanup not confirmed: '+name)
# The installed NDK's upstream CMake deprecation messages remain in configure.log.
for log in [*(ROOT/'build/native-logs').glob('*-build.log'),*(ROOT/'build').glob('android-*/build.log')]:
    if re.search(r'(?:warning|avertissement)\s*(?:[A-Z]+\d+|:)',log.read_text(encoding='utf-8'),re.I):
        raise SystemExit('Compiler warning: '+str(log))
integration_markers=(
    'PASS integration WAV LIST/adtl/INFO/typed metadata/independent fixtures',
    'PASS integration device identity/capabilities/negotiation/timing',
    'PASS integration PCM editing proxy/exact random seek/bounds',
    'PASS integration MIDI selection/multiport/SysEx/epochs/voice releases',
    'PASS integration source policies/time-pitch curves/repitch/prime/tails/providers',
    'PASS plugin CLAP ABI/multibus/automation/state/editor/lifecycle/realtime',
    'PASS plugin VST3 ABI/controller/messages/multibus/state/editor/realtime')
for name,report in reports.items():
    if name.startswith(('build/native-','build/device-')):
        for marker in integration_markers:
            if marker not in report.get('regressions',''):raise SystemExit('Missing integration regression '+marker+': '+name)
    if name.startswith('build/native-'):
        if 'PASS plugin separate process/IPC/state/editor/late blocks/crash/restart/realtime' not in report.get('regressions',''):
            raise SystemExit('Missing plugin worker process acceptance: '+name)
        for marker in ('Long codec proxy AAC/M4A','Long codec proxy WMA'):
            if marker not in report.get('regressions',''):raise SystemExit('Missing long native-codec measurement: '+name)
component_count=len(json.loads((ROOT/'third_party/manifest.json').read_text())['components'])
summary={'timeUtc':datetime.now(timezone.utc).isoformat(),'sourceSha256':digest,
         'status':'passed_available_targets','requestedScopeComplete':False,'implementedLibraryScopeComplete':True,'applicationMigrationComplete':False,'nativeRuntimeExecuted':True,
         'reports':paths,'unverified':['desktop Linux/GCC (deferred by owner)','macOS/Apple Clang (deferred by owner)',
         'Windows ARM64 application linking/runtime','physical MIDI2/AMidi hardware and Apple native transport/codec compilation/runtime','SVE native execution and multiple vector lengths','physical audio I/O, listening acceptance and all possible rate/frequency combinations','CyDAW shell/model/routing migration (outside CyDSP)','application-specific beats/texture providers; built-in AU/AAX/LV2/JSFX adapters','macOS/Linux desktop plugin/worker/editor runtime and native Android service worker'],
         'untouched':['CyCode','CyDAW_Project','GitHub Actions','antivirus settings'],'installedDeviceApplication':False}

static=reports['build/validation.json']
simd=reports['build/simd-validation.json']
objects=sum(c['objectsInspected'] for c in simd['checks'])
sourceCount=sum(1 for folder in ['include','src','tests','examples'] for file in (ROOT/folder).rglob('*') if file.suffix in ['.h','.cpp'])
validation=f'''# Validation

2026-11-12. Work is confined to CyDSP. The owner explicitly authorized native tests,
benchmarks and the connected Quest 3. Production configurations remain static,
with test/measurement/sanitizer options OFF. No CyCode or CyDAW_Project source, GitHub Actions or
antivirus setting was changed. No application was installed on the Quest.

Current compiler-input SHA-256: {digest}

## Executed and passed

| Environment | Evidence |
| --- | --- |
| Windows x64 MSVC 19.40 | Native scalar/SSE2/AVX2/AVX-512 regressions, allocation/free audit and measurements |
| Windows x86 MSVC 19.40 | Native regressions and allocation/free audit |
| Windows x64 without explicit SIMD | Native regressions and allocation/free audit |
| Windows x64 Clang 19.1.5 | Native regressions and allocation/free audit |
| Windows x64 AddressSanitizer | Instrumented library and full native regression suite |
| Quest 3 Android ARM64 | Native scalar/NEON regressions, allocation/free audit and measurements |
| Quest 3 Android ARM32 compatibility | Native scalar/NEON regressions, allocation/free audit and measurements; double NEON kernels fall back to scalar |
| Android ARM32/ARM64/x86/x86_64 NDK Clang 18 | Archives, isolated public headers, regression objects and target executable linking |
| Windows x64/x86/scalar/ARM64 Clang | Four Release production configurations, static libraries only, with compile-only checks |
| Installed CMake package | Byte-exact headers/notices/manifest and separate static consumer |

All {static['publicHeaders']} public headers compile individually. {static['staticAssertSites']}
static_assert sites pass under MSVC and Clang. Source checks cover {sourceCount} owned C++
files, local includes, Python syntax, documentation links, build/ISA policy and all
{component_count} pinned permissive component archives/notices. Final compiler build logs have no
warnings. An additional audit disassembles {objects} production objects across all
eight target archives, verifies per-file wide-ISA isolation and finds no fused FP
instructions. The instruction audit is generated locally in `build/simd-validation.json`. SVE is compiled and
linked for Android ARM64; no SVE-native execution is claimed.
The installed NDK emits upstream CMake deprecation notices during
configuration; its compiler build logs are clean.

Prepared property-service regressions additionally validate ResourceList/DeviceInfo/ChannelList/JSONSchema, resId isolation, RFC6901 partial SET, list pagination/totals, full/partial/notify delivery, TTL/page/binary caching, stale-GET protection, eviction/ACK pressure, timeout, cancellation/MUID cleanup and retained-memory request/patch/subscription processing. [Property service](CiProperty.md) records exact scope.

Native MIDI2 regressions check CI discovery/collisions/profiles/chunked properties, protocol confirmation/timeout, retained-memory processing and parallel audio/SysEx queues. Quest also exercises native-endian descriptor framing, saturation, ownership and disconnect through nonblocking sockets. No physical MIDI port is opened; Windows MIDI Services is unavailable on this Windows 10 host, Android JNI/AMidi hardware is not exercised, and Apple transport source remains uncompiled. See [MIDI 2](Midi2.md).

The integrated suites additionally check half-bin FFTs against direct DFTs for power-of-two and mixed-radix sizes on every available backend, MIDI 2 scaling including all 16384 pitch values, MPE zones/RPN/pedals, sample-accurate custom/oscillator/sampler voices, queued transport/loops/underruns, random epoch seeks with full-capacity prefill and centered sinc timing, and background recording/draining with exact file readback. Audio-thread memory auditing covers those new paths.

The expanded suites additionally check DSP transfer functions, envelopes, oversampling, modulation/reverb decay, MIDI/SMF/SPSC, independent WAV/AIFF fixtures, WAV/AIFF/FLAC/Ogg roundtrips, spectral pitch shifting, retained-memory processing/reset and silent virtual audio callbacks. The DAW expansion adds filter families/interpolators/windows, gate look-ahead, half-band/IIR oversampling, wavetable/mip sampling/presets, worker stretching/IR/peak caches, clock collection/reconnects/Function Blocks/controllers/profile dispatch, AIFC/CAF with retained marker/loop chunks and an independent successful MP3 fixture. Windows native MP3/AAC/M4A/WMA encode/decode/tone/decoded-seek tests pass. Exact scope and platform limits are recorded in [DAW integration](DawIntegration.md) and [Coverage](Coverage.md).

The native suites exercise direct DFT/convolution references, SIMD residuals and
unaligned views, invalid/overlapping requests, reset/moves, split callbacks, queued
filter/EQ targets, fractional delays, rectangular mixing and cross-channel aliases,
nonuniform partition boundaries, sparse MIMO route sums, EOS versus an explicit
zero stream, rate-change history, graph cycle rejection/topological sorting/slot
reuse/custom processor callbacks, and 200 long-IR producer publications per
convolution mode concurrent with audio consumption. All available backends also run shared-FFT MIMO versus direct
convolution, live-history prewarming versus explicit convolution, ratio ramps versus
an independent absolute-position clock, callback splits/moves, and fused gain chains
with exact stage rounding and NaN mute checks. Scheduled convolution is compared
with dense direct convolution at late sizes 1..256 and a sparse three-channel 1024
case, including zero tail, partial FFT moves/reset, boundary impulses and invalid
requests. Stage kernels are compared against scalar butterfly references on
unaligned guarded spans. Deterministic async tests cover replacement, empty callbacks,
queue pressure and immediate queued activation. The common Convolver interface
checks successful/failed engine changes, configuration/history transactions and
moves during partial FFT work. All three modes run serialized and asynchronous
updates against live-history direct references with/without prewarm and with
zero/positive fade durations. Progressive shared MIMO covers 4->3 and 2->5 channel
layouts at late sizes 1..256, sparse 1024-frame tiers, disconnected inputs/outputs,
unequal/duplicate routes, complete tails and rectangular cross-channel aliases.
Late work budgets explicitly check sharing per active input/output. Partial FFT
moves/reset and successful/failed engine changes retain their documented history.

The allocation audit replaces C++ allocation/deallocation entry points on the test
thread. It covers prepared process/reset/finish and fade activation for every
processor, including retired IR plans and progressive FFT jobs/reset. It detected zero allocations AND frees in
the exercised regions. Worker preparation remains intentionally non-real-time.
The audit does not make arbitrary user graph callbacks allocation-free.

## Integration report follow-up

Independent WAV LIST/adtl/INFO, odd chunks and native24-in-32 PCM references pass.
Public metadata versions/typed edits preserve unknown chunks. Fake/null device
tests distinguish negotiated values, unknown hardware facts and callback clocks.
MIDI tests cover ambiguous selection, two ports, fragmented12 KiB SysEx, epochs
and voice-release notifications.

Source tests cover time/pitch curves, repitch reference tones, extended ratios,
prime/tail contracts and an original external-provider fixture. Beats/texture
remain provider policies, not claimed native matches for CyDAW algorithms.
Long60-second AAC/WMA proxy fixtures compare exact near-EOF PCM, seek time,
bounded copy storage and sampled process private-memory increase.

Original CLAP/VST3 shared modules verify the ABI, multibus/sidechain, automation,
copied events, component/controller states, lifecycle and host allocation audit.
Windows tests actual child editors, resize/DPI, focus/keys/wheel and a separate
process worker: IPC, one-block pipeline, late-result silence, abrupt plugin exit
and restart. Android runs in-process module tests; standalone Android process
workers require an application service and are explicitly skipped.

See [Plugin contracts](Plugins.md) and [Remediation scope](IntegrationRemediation.md).
Application shell/model/routing migration and other plugin-format providers remain
outside this library-only change; a complete JUCE replacement is not claimed.

## Reports and reproduction

The consolidated report (`build/completion.json`) and raw Windows/Quest data are
generated locally under `build/`. See the [measurement analysis](Measurements.md)
for recorded results and limitations.
Static-only validation.json describes that runner alone; nativeRuntimeExecuted=false
there does not describe the separate native runs.

```text
python -B scripts/validate.py
python -B scripts/run_native.py --measure
python -B scripts/run_native.py --arch x86
python -B scripts/run_native.py --arch scalar
python -B scripts/run_native.py --compiler clang
python -B scripts/run_native.py --sanitize
python -B scripts/build_android.py --abi arm64-v8a --measure
python -B scripts/build_android.py --abi armeabi-v7a --measure
python -B scripts/build_android.py --abi x86
python -B scripts/build_android.py --abi x86_64
python -B scripts/run_android_device.py --abi arm64-v8a --measure
python -B scripts/run_android_device.py --abi armeabi-v7a --measure
python -B scripts/verify_simd.py
python -B scripts/summarize_validation.py
```

Device execution requires the intended owner-authorized ADB connection. The runner
uses a unique /data/local/tmp directory and removes only its exact files afterward.
No emulator, root access, application install or device setting change is required.
Every required report must pass and match the current compiler-input digest before
consolidation succeeds.

## Remaining target coverage

Desktop Linux/GCC and macOS/Apple Clang tests are deferred by the owner.
No corresponding host is available in this session; Windows WSL is not enabled. run_portable.py is ready to run natively on those hosts.
Android uses a Linux kernel and libc++, and is reported separately from desktop
Linux/glibc/GCC. SVE is compiled and linked in Android ARM64; the Quest reports SVE unavailable.
Native SVE execution at different vector lengths needs compatible hardware.
Windows ARM64 archives compile, but application linking/runtime
still needs its target SDK libraries/host; Android ARM64 execution does not validate
that Windows ABI. No CI workflow was enabled to bypass these missing environments.

The synthetic workloads and sampled spectral checks do not establish all possible
stream states, hard audio deadlines, listening quality, physical audio I/O latency
or performance on other machines. CyDSP uses the MIT license; upstream notices
are indexed in Licensing.md. Further implementation opportunities are in Roadmap.md.
'''
(ROOT/'docs/Validation.md').write_text(validation,encoding='utf-8',newline='\n')
measurements='''# Native measurements

2026-11-12. Explicit Release validation runs; strict floating-point policy, no
fast-math/FMA contraction or global CPU-environment changes. Raw JSON retains
all cases. Native programs perform numerical audio DSP without opening an audio
device; no listening or physical audio I/O measurement is claimed.

## Performance samples

All rows below are float, two-channel convolution of a 32768-tap IR in 128-frame
blocks. Uniform hybrid uses P=128; nonuniform uses early=128 and late=1024.
Times are microseconds per call, including normal OS scheduling noise.

| Host / backend | Algorithm | Mean us | Median us | p99 us | Maximum us |
| --- | --- | ---: | ---: | ---: | ---: |
'''
for path,label in [('build/native-x64.json','Ryzen 9 7900X'),('build/device-arm64-v8a.json','Quest 3 ARM64'),('build/device-armeabi-v7a.json','Quest 3 ARM32')]:
    r=reports[path]
    for m in r['measurements']:
        if m['kind']=='performance' and m['sampleType']=='float' and m['size']==32768 and m['operation'] in ['hybrid128','nonuniform128']:
            measurements+=f"| {label} / {m['backend']} | {m['operation']} | {m['meanNs']/1000:.2f} | {m['medianNs']/1000:.2f} | {m['p99Ns']/1000:.2f} | {m['maxNs']/1000:.2f} |\n"
measurements+='''
The nonuniform late tier runs less often, so lower average work can coexist with
larger callback peaks. Choose a configuration against both mean and tail cost.
These are sampled call distributions, not hard real-time bounds. No CPU affinity,
frequency lock, thermal policy or OS/antivirus setting was changed. ARM32 runs in
the Quest's compatibility mode; this is not a second independent ARM device.

Each operation has 24 warmup calls and 400 measured samples. Dot products batch
256 calls per timing sample to reduce clock quantization; their percentiles are
of batch-average costs. Other reported calls use batchSize=1. Compiler/ABI/sample
type and dimensions are in raw records. Measurements include dot, complex FFT,
voice-parallel filters, FIR, hybrid/nonuniform convolution, shared versus independent
MIMO, fused versus callback graph chains, and rational/variable resampling.

## Numerical and spectral checks

The finite resampling grid uses four rate pairs (48->44.1, 44.1->48, 48->16 and
96->44.1 kHz), three presets, 10 passband frequencies through 0.8 times the lower
Nyquist frequency and 12 stopband frequencies from 1.05 times output Nyquist
through 0.98 times input Nyquist when downsampling. Tones are projected over
8192 output frames after startup. Preset acceptance: absolute sampled passband
error <0.1 dB and sampled stopband gain <-60 dB. Additional raw 64/128/256-tap
configurations are measured independently without declaring every setting high quality.

| Host | Measurement records | Maximum preset passband error dB | Weakest sampled preset rejection dB | Worst float FFT roundtrip error | Worst double FFT roundtrip error |
| --- | ---: | ---: | ---: | ---: | ---: |
'''
for path,label in [('build/native-x64.json','Windows x64'),('build/device-arm64-v8a.json','Quest ARM64'),('build/device-armeabi-v7a.json','Quest ARM32')]:
    ms=reports[path]['measurements'];ps=[m for m in ms if m.get('metric')=='resamplePreset']
    passband=max(m['maxSampledPassbandErrorDb'] for m in ps);stop=max(m['worstSampledStopbandGainDb'] for m in ps if m['inputRate']>m['outputRate'])
    fft=lambda t:max(m['value'] for m in ms if m.get('metric')=='fftRoundtripMaxError' and m['sampleType']==t)
    if passband>=0.1 or stop>=-60 or fft('float')>=1e-4 or fft('double')>=1e-10:raise SystemExit('Quality criteria failed: '+path)
    measurements+=f"| {label} | {len(ms)} | {passband:.6g} | {stop:.2f} | {fft('float'):.3g} | {fft('double'):.3g} |\n"
raw=reports['build/native-x64.json']['measurements']
short=max((m for m in raw if m.get('metric')=='passbandGainDb' and m['taps']==64),key=lambda m:abs(m['valueDb']))
measurements+=f'''
The raw 64-tap configuration reached {short['valueDb']:.3f} dB gain at
{short['frequencyHz']:.2f} Hz for {short['inputRate']}->{short['outputRate']} Hz.
That finding motivated the 128-tap raw default and ratio-aware support in the
quality presets. Explicit short filters remain available for a chosen CPU/quality
tradeoff; they do not inherit the preset acceptance results.

The 16->48 kHz image test projects a 1 kHz source onto its 15 kHz image using a
Hann window; all three presets must keep that image below -60 dB. FFT checks cover
64..4096 points, both sample types and every available backend. A 512-frame linear
IR transition between gains +1 and -1 on constant input must follow its reference
trajectory within 1e-12 and limit each step to 2/512. These pass on Windows and both
Quest ABIs. Variable-ratio DC measurements sweep spacing through [0.75,3], with
2048-output-frame ramps after startup, and require error/sample steps <1e-10.
Default IR prewarming supplies complete finite destination history before fading;
explicit cold mode, phase cancellation and deliberately faded old tails remain available.

Raw data is generated locally in `build/native-x64.json`,
`build/device-arm64-v8a.json` and `build/device-armeabi-v7a.json`.
See [validation](Validation.md) for the executed checks and coverage.
'''
measurements+='\n## Same-build optimization comparisons\n\n'
measurements+='Float, 128-frame blocks. MIMO is 4 inputs x 4 outputs, 16 routes, 8192 taps per route; graph is a 32-stage two-channel gain chain. Both alternatives use the same selected backend. Fused kernels process four independent register groups per gain. Ratios use sampled mean call times.\n\n'
measurements+='| Host / backend | Shared MIMO us | Independent MIMO us | Ratio | Fused graph us | Callback graph us | Ratio |\n| --- | ---: | ---: | ---: | ---: | ---: | ---: |\n'
for path,label,backend in [('build/native-x64.json','Ryzen 9 7900X','avx512'),('build/device-arm64-v8a.json','Quest ARM64','neon'),('build/device-armeabi-v7a.json','Quest ARM32','neon')]:
    ms=reports[path]['measurements']
    find=lambda op:next(m['meanNs']/1000 for m in ms if m.get('operation')==op and m['sampleType']=='float' and m['backend']==backend)
    shared,independent,fused,callbacks=[find(op) for op in ['matrixShared128','matrixIndependent128','graphFused32x128','graphCallbacks32x128']]
    measurements+=f'| {label} / {backend} | {shared:.2f} | {independent:.2f} | {independent/shared:.2f}x | {fused:.2f} | {callbacks:.2f} | {callbacks/fused:.2f}x |\n'
    cap=next(m for m in ms if m.get('kind')=='capabilities')
    if backend=='avx512' and not cap['avx512CpuOs']:raise SystemExit('AVX-512 hardware coverage missing')
    if path.startswith('build/device-') and cap['sveCpuOs']:raise SystemExit('Update SVE runtime coverage documentation for this device')
    dc=next(m for m in ms if m.get('metric')=='variableRatioDc')
    if dc['maximumError']>=1e-10 or dc['maximumSampleStep']>=1e-10:raise SystemExit('Variable-rate quality failed')
measurements+='\nThe reference MIMO executes one NonuniformConvolver per route and sums its outputs. The reference graph retains identity callbacks at each stage. Fusion applies to null routing nodes whose semantics are known; this does not fuse arbitrary processors. Costs and ratios depend on workload, ISA, scheduling and hardware. CPU/OS capability records explicitly distinguish AVX-512 execution from SVE compile/link-only coverage.\n'

measurements+='\n## Whole-stage FFT and progressive convolution\n\n'
measurements+='The FFT comparison uses a 1024-point float forward transform and the same prepared tables. The reference retains the former group dispatcher and scalar tiny stages. All timings include input permutation; this is a same-build comparison, not a cross-library ranking.\n\n'
measurements+='| Host / backend | Full-stage FFT us | Group reference us | Reference / full-stage |\n| --- | ---: | ---: | ---: |\n'
targets=[('build/native-x64.json','Ryzen 9 7900X','avx512'),('build/device-arm64-v8a.json','Quest ARM64','neon'),('build/device-armeabi-v7a.json','Quest ARM32','neon')]
for path,label,backend in targets:
    ms=reports[path]['measurements']
    get=lambda op:next(m for m in ms if m.get('operation')==op and m['backend']==backend and m['sampleType']=='float' and m['size']==1024)
    full,group=get('complexFFT'),get('complexFFTReference')
    measurements+=f"| {label} / {backend} | {full['meanNs']/1000:.2f} | {group['meanNs']/1000:.2f} | {group['meanNs']/full['meanNs']:.2f}x |\n"
measurements+='\nThe convolution rows use the same 32768-tap IR, two float channels, 128-frame callbacks, early partition 128 and late partition 1024. ScheduledConvolver covers the first 2048 taps with its hybrid prefix, then spreads tail FFT/recombination/MAC/overlap work across a full input interval. NonuniformConvolver covers 1024 early taps and retains boundary transforms. Both produce the same causal response with zero added block latency.\n\n'
measurements+='| Host / backend | Nonuniform mean us | Scheduled mean us | Mean cost ratio | Nonuniform p99 us | Scheduled p99 us |\n| --- | ---: | ---: | ---: | ---: | ---: |\n'
for path,label,backend in targets:
    ms=reports[path]['measurements']
    get=lambda op:next(m for m in ms if m.get('operation')==op and m['backend']==backend and m['sampleType']=='float' and m['size']==32768)
    normal,scheduled=get('nonuniform128'),get('scheduled128')
    measurements+=f"| {label} / {backend} | {normal['meanNs']/1000:.2f} | {scheduled['meanNs']/1000:.2f} | {scheduled['meanNs']/normal['meanNs']:.2f}x | {normal['p99Ns']/1000:.2f} | {scheduled['p99Ns']/1000:.2f} |\n"
measurements+='\nJob counts bound the scheduled tail only, not the synchronous prefix or OS scheduling. Chunk costs differ by phase. The sampled p99/max distributions are not hard deadline guarantees. Both modes remain selectable because reducing peaks may increase average CPU. No affinity/frequency or device setting was changed.\n\n'
measurements+='| Host | Worst float scheduled/direct error | Worst double scheduled/direct error |\n| --- | ---: | ---: |\n'
for path,label,_ in targets:
    qs=[m for m in reports[path]['measurements'] if m.get('metric')=='scheduledConvolutionMaxError']
    worst=lambda typ:max(m['value'] for m in qs if m['sampleType']==typ)
    if worst('float')>=1e-4 or worst('double')>=1e-10 or any(m['latencyFrames']!=0 for m in qs):raise SystemExit('Scheduled convolution numerical criteria failed: '+path)
    measurements+=f"| {label} | {worst('float'):.3g} | {worst('double'):.3g} |\n"
measurements+='\nThe numerical grid uses a 521-tap finite IR, 769 nonzero input frames followed by zeros, two channels, 137-frame callback limits and 64-frame late partitions. Every available backend is checked against explicit time-domain convolution, including startup and the complete tail.\n'

measurements+='\n## Common convolution interface and active IR fades\n\n'
measurements+='The same 32768-tap IR, two float channels and 128-frame callbacks use early=128 and late=1024. Configuration and IR publication are outside timing. Fade measurements follow full destination prewarming; a 1048576-frame fade keeps both engines active through all 24 warmups and 400 measured calls. The asynchronous benchmark measures the consumer with no concurrent producer.\n\n'
measurements+='| Host / backend | Engine | Direct mean us | Common interface mean us | Serialized fade mean / p99 us | Async fade mean / p99 us |\n| --- | --- | ---: | ---: | ---: | ---: |\n'
for path,label,backend in targets:
    ms=reports[path]['measurements']
    # Require every mode/type/backend record, rather than silently accepting old measurement binaries.
    available=next(m['availableBackends'] for m in ms if m.get('kind')=='capabilities')
    for isa in available:
        for typ in ['float','double']:
            for mode in ['Hybrid','Nonuniform','Scheduled']:
                for op in ['configured'+mode+'128','smoothed'+mode+'Fade128','async'+mode+'Fade128']:
                    if not any(m.get('operation')==op and m.get('backend')==isa and m.get('sampleType')==typ for m in ms):
                        raise SystemExit('Missing configured engine benchmark: '+path+' '+op+' '+isa+' '+typ)
    get=lambda op:next(m for m in ms if m.get('operation')==op and m['backend']==backend and m['sampleType']=='float' and m['size']==32768)
    for mode in ['Hybrid','Nonuniform','Scheduled']:
        direct,common,smooth,async_=[get(op) for op in [mode.lower()+'128','configured'+mode+'128','smoothed'+mode+'Fade128','async'+mode+'Fade128']]
        measurements+=f"| {label} / {backend} | {mode.lower()} | {direct['meanNs']/1000:.2f} | {common['meanNs']/1000:.2f} | {smooth['meanNs']/1000:.2f} / {smooth['p99Ns']/1000:.2f} | {async_['meanNs']/1000:.2f} / {async_['p99Ns']/1000:.2f} |\n"
measurements+='\nSeparate runs have normal scheduling noise, so small direct/interface differences do not establish dispatch overhead. Transition peaks include two active convolution banks and output blending. Sampled distributions do not guarantee real-time deadlines.\n\n'
measurements+='| Host | Worst float IR-update error | Worst double IR-update error |\n| --- | ---: | ---: |\n'
for path,label,_ in targets:
    ms=reports[path]['measurements']
    qs=[m for m in ms if m.get('metric')=='configuredIrTransitionMaxError']
    available=next(m['availableBackends'] for m in ms if m.get('kind')=='capabilities')
    if len(qs)!=len(available)*6:raise SystemExit('Missing configured transition quality records: '+path)
    for isa in available:
        for typ in ['float','double']:
            for mode in ['hybrid','nonuniform','scheduled']:
                record=next(m for m in qs if m['backend']==isa and m['sampleType']==typ and m['mode']==mode)
                if record['value']>=(1e-4 if typ=='float' else 1e-10):raise SystemExit('Configured IR transition quality failed: '+path)
    worst=lambda typ:max(m['value'] for m in qs if m['sampleType']==typ)
    measurements+=f"| {label} | {worst('float'):.3g} | {worst('double'):.3g} |\n"
measurements+='\nThe numeric grid changes a unit IR at frame 131 to a dense 273-tap IR, prewarms for 272 frames, then fades for 37 frames. Two channels and arbitrary callbacks up to 53 frames include startup, live history and complete tail. Both serialized and asynchronous outputs are compared to direct time-domain references for every available backend and sample type. Regression tests additionally cover cold/instant transitions, failed replacement, queued long-IR publications, moves and allocation/free accounting.\n'

measurements+='\n## Progressive shared MIMO\n\n'
measurements+='These 4-input/4-output cases use 16 routes, identical finite IR lengths, float samples and 128-frame callbacks. Both modes retain one forward transform per active input and one inverse per active output. Early partition=128, late=1024. Progressive prefix coverage extends to 2048 taps; the nonuniform prefix ends at 1024.\n\n'
measurements+='| Host / backend | Taps per route | Nonuniform mean / p99 us | Progressive mean / p99 us | Mean cost ratio |\n| --- | ---: | ---: | ---: | ---: |\n'
for path,label,backend in targets:
    ms=reports[path]['measurements']
    available=next(m['availableBackends'] for m in ms if m.get('kind')=='capabilities')
    for isa in available:
        for typ in ['float','double']:
            for size,normal in [(8192,'matrixShared128'),(32768,'matrixNonuniform128')]:
                for op in [normal,'matrixScheduled128']:
                    if not any(m.get('kind')=='performance' and m.get('operation')==op and m.get('backend')==isa and m.get('sampleType')==typ and m.get('size')==size for m in ms):
                        raise SystemExit('Missing progressive MIMO benchmark: '+path+' '+op+' '+str(size)+' '+isa+' '+typ)
                budget=next(m for m in ms if m.get('kind')=='scheduling' and m.get('operation')=='matrixScheduled128' and m['backend']==isa and m['sampleType']==typ and m['size']==size)
                expected=2528 if size==8192 else 9056
                if budget['workUnitsPerPartition']!=expected or budget['maximumWorkUnitsPerFrame']!=(expected+1023)//1024:
                    raise SystemExit('Progressive MIMO shared-transform budget changed: '+path)
    for size,normal in [(8192,'matrixShared128'),(32768,'matrixNonuniform128')]:
        get=lambda op:next(m for m in ms if m.get('kind')=='performance' and m.get('operation')==op and m['backend']==backend and m['sampleType']=='float' and m['size']==size)
        normal,scheduled=get(normal),get('matrixScheduled128')
        measurements+=f"| {label} / {backend} | {size} | {normal['meanNs']/1000:.2f} / {normal['p99Ns']/1000:.2f} | {scheduled['meanNs']/1000:.2f} / {scheduled['p99Ns']/1000:.2f} | {scheduled['meanNs']/normal['meanNs']:.2f}x |\n"
measurements+='\nThe scheduled late budget is 2528 chunks per 1024-frame interval (at most 3/frame) for 8192 taps, or 9056 (at most 9/frame) for 32768. Those counts exclude direct/early-tier work. Each phase has a different cost; a job count does not bound wall time. The larger prefix and scheduling can increase average CPU; sampled p99/max cannot guarantee an audio deadline and are subject to OS/device variation. No automatic mode switch is introduced.\n\n'
measurements+='| Host | Worst float MIMO/direct error | Worst double MIMO/direct error |\n| --- | ---: | ---: |\n'
for path,label,_ in targets:
    ms=reports[path]['measurements']
    qs=[m for m in ms if m.get('metric')=='scheduledMimoMaxError']
    available=next(m['availableBackends'] for m in ms if m.get('kind')=='capabilities')
    if len(qs)!=len(available)*2:raise SystemExit('Missing progressive MIMO quality records: '+path)
    for isa in available:
        for typ in ['float','double']:
            m=next(m for m in qs if m['backend']==isa and m['sampleType']==typ)
            if m['value']>=(1e-4 if typ=='float' else 1e-10) or m['latencyFrames']!=0:
                raise SystemExit('Progressive MIMO numerical criteria failed: '+path)
    worst=lambda typ:max(m['value'] for m in qs if m['sampleType']==typ)
    measurements+=f"| {label} | {worst('float'):.3g} | {worst('double'):.3g} |\n"
measurements+='\nThe numerical measurement uses 4 inputs, 5 outputs, 6 sparse/duplicate routes with unequal dense IR lengths 1..521, 64-frame late partitions, 769 source frames followed by zeros and variable callbacks up to 137 frames. Independent double time-domain convolution covers every output and the full tail. Regressions also cover all three MIMO modes, rectangular exact aliases, smaller/larger partitions and invalid requests before state mutation.\n'
measurements+='\n## Expanded DSP (128 frames, stereo, 48 kHz)\n\n'
measurements+='Times below are microseconds per 128-frame callback, float samples, mean / p99. The audio period at 48 kHz is 2666.67 us. These are single-process synthetic workloads, not physical audio latency or universal speedup ratios. Scalar labels describe these processors without an explicit SIMD implementation; oversampling requests the automatic FIR backend.\n\n'
measurements+='| Host | Dynamics | TPT | Ladder | Chorus | Phaser | Reverb | FIR4x roundtrip |\n| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |\n'
for path,label,_ in targets:
    ms=reports[path]['measurements']
    values=[]
    for op in ['dynamicsCompressor','stateVariableTpt','ladderLowpass','chorus','phaser','schroederReverb','oversample4Roundtrip']:
        m=next(m for m in ms if m.get('operation')==op and m.get('sampleType')=='float')
        values.append(f"{m['meanNs']/1000:.2f} / {m['p99Ns']/1000:.2f}")
    measurements+='| '+label+' | '+' | '.join(values)+' |\n'
measurements+='\n| Host | Float alias reduction dB | Double alias reduction dB |\n| --- | ---: | ---: |\n'
for path,label,_ in targets:
    values=[]
    for typ in ['float','double']:
        q=next(m for m in reports[path]['measurements'] if m.get('metric')=='oversamplingCubicAliasReductionDb' and m.get('sampleType')==typ)
        if q['value']>=-40:raise SystemExit('Oversampling alias rejection failed: '+path)
        values.append(f"{q['value']:.2f}")
    measurements+='| '+label+' | '+' | '.join(values)+' |\n'
measurements+='\nThe quality fixture cubes a 0.8-amplitude 9.6 kHz sine at 48 kHz. Without oversampling the third harmonic aliases to 19.2 kHz with amplitude 0.8^3/4. Values are 20log10(measured alias amplitude/reference amplitude), so more negative means stronger rejection. FIR4x adds 32 input frames of latency. This one tone does not establish rejection for every frequency or nonlinear processor.\n'
measurements+='\n## Integrated synthesis and Stretch\n\n'
measurements+='The synth workload uses sine/ADSR voices with sustained notes, stereo float output at 48 kHz and 128-frame callbacks. Tables report mean microseconds per callback; the available period is 2666.67 us. Voice setup and note publication are outside timing. This is CPU processing cost, not device latency.\n\n'
measurements+='| Host / selected backend | 16 voices mean us | 64 voices mean us |\n| --- | ---: | ---: |\n'
for path,label,backend in targets:
    ms=reports[path]['measurements']
    values=[]
    for voices in [16,64]:
        m=next(m for m in ms if m.get('operation')=='polyphonicSynth128' and m.get('sampleType')=='float' and m.get('size')==voices)
        values.append(f"{m['meanNs']/1000:.2f}")
    measurements+='| '+label+' / '+backend+' | '+' | '.join(values)+' |\n'
measurements+='\nStretch compares the pinned unmodified portable Signalsmith engine against the adapted CyDSP engine in the same strict-FP Release binary. The integrated implementation uses CyDSP FFT/vector kernels, half-bin reconstruction, fused packing/permutation and compact 3×power-of-two plans. Original headers are compiled only in this opt-in benchmark. The upstream label scalar identifies its portable implementation, not a ban on compiler auto-vectorization. Both engines use two channels, 128-frame input/output calls and identical configurations: a 4096-frame window/1024 hop without split computation, and the 48 kHz default 5760-frame window with split computation. No pitch change is requested in this CPU workload.\n\n'
measurements+='The ratio is **original mean time / integrated mean time**: 1.10x means about 10% more processing throughput in this measured case; below 1.00x means the integration is slower. Small differences are subject to OS noise. These are not comparisons to JUCE or a universal acceleration claim.\n\n'
measurements+='| Host | Type | Window | Original mean us | Integrated mean us | Original / integrated |\n| --- | --- | --- | ---: | ---: | ---: |\n'
for path,label,backend in targets:
    ms=reports[path]['measurements']
    for typ in ['float','double']:
        for variant,title in [('4096','4096'),('Preset','default 5760')]:
            original=next(m for m in ms if m.get('operation')=='stretchOriginal'+variant and m.get('sampleType')==typ)
            integrated=next(m for m in ms if m.get('operation')=='stretchCyDSP'+variant and m.get('sampleType')==typ)
            measurements+=f"| {label} | {typ} | {title} | {original['meanNs']/1000:.2f} | {integrated['meanNs']/1000:.2f} | {original['meanNs']/integrated['meanNs']:.2f}x |\n"
measurements+='\nThe unchanged spectral regression also checks an octave shift of a 440 Hz tone, formants, float/double, allocation-free retained processing/reset and finite tail draining. Direct DFT tests independently validate the altered FFT normalization for small mixed sizes. These checks do not replace listening acceptance for transients, music or extreme duration ratios. Worker tests exercise exact native decode, 48-to-44.1 kHz tone timing, arbitrary epoch seeks and final-recording drain; filesystem completion times are deliberately excluded from audio CPU timings.\n'

measurements+='\n## DAW expansion measurements\n\nEach timing is a 128-frame stereo callback, float, mean microseconds. These are measured configurations, not a universal speedup ratio. The gate has a scalar detector and no backend selector.\n\n| Target | Backend | Half-band4x roundtrip µs | IIR4x roundtrip µs | Wavetable oscillator µs | Sampler alias rejection dB |\n| --- | --- | --- | --- | --- | --- |\n'
for path,label,isa in [('build/native-x64.json','Windows x64','AVX-512'),('build/device-arm64-v8a.json','Quest ARM64','NEON'),('build/device-armeabi-v7a.json','Quest ARM32','NEON')]:
    ms=reports[path]['measurements']
    # Backend names are emitted in lower case by the library.
    backend={"AVX-512":"avx512","NEON":"neon"}[isa]
    def timing(op):
        return next(m['meanNs']/1000 for m in ms if m.get('operation')==op and m['sampleType']=='float' and m['backend']==backend)
    rejection=next(m['reductionDb'] for m in ms if m.get('metric')=='samplerOctaveAliasReductionDb' and m['sampleType']=='float')
    if rejection<40:raise SystemExit('Sampler alias quality failed: '+path)
    measurements+=f"| {label} | {isa} | {timing('oversample4HalfBandRoundtrip'):.3f} | {timing('oversample4IirRoundtrip'):.3f} | {timing('wavetableOscillator2048'):.3f} | {rejection:.2f} |\n"
measurements+='\nSampler rejection is 10log10(linear-output power / filtered-mip-output power) for a0.35-cycles/sample sine read at2x, excluding boundaries. Higher positive dB means stronger alias reduction. This one out-of-band tone does not establish full-band transparency. Wavetable banks and mip samples are prepared before timing; device/file I/O is excluded. IIR delay is frequency dependent, and half-band FIR has a longer fixed delay than the original FIR mode; see the integration contracts.\n\n'
(ROOT/'docs/Measurements.md').write_text(measurements.rstrip()+'\n',encoding='utf-8',newline='\n')
(ROOT/'build/completion.json').write_text(json.dumps(summary,indent=2)+'\n',encoding='utf-8')
print('PASS: all available-target reports match '+digest[:16]+'; desktop Linux/macOS and Windows ARM64 runtime remain unverified')
print('Consolidated report: '+str(ROOT/'build/completion.json'))
