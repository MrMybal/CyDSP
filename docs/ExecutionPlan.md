# Authorized completion work

2026-11-11. Owner requested native tests, measured performance/audio quality,
platform coverage and the five listed advanced feature groups. Work stayed in
the CyDSP project directory. Native runs and Quest ADB execution were explicitly authorized.
No CyCode edit, GitHub Action, antivirus/device setting or application install.

## Implemented and exercised

- SmoothedBiquadCascade: preallocated complete-EQ transitions and one queued target.
- AsyncConvolver: single-producer worker preparation/publication with three retained
  slots, bounded queue pressure and no audio-thread destruction.
- NonuniformConvolver: two partition sizes with aligned late-tier onset.
- MatrixConvolver: sparse independent IR routes, summed outputs and exact aliases.
- Resampler finish/isFinished, non-real-time history-preserving setRates and quality
  presets. Wider measurements motivated a 128-tap default and ratio-aware support.
- AudioGraph: copied DAG, cycle/reference checks, topological execution, borrowed
  callbacks and reuse of buffers after last consumption.

## Executed evidence

Current reports are consolidated by scripts/summarize_validation.py into
build/completion.json, docs/Validation.md and docs/Measurements.md. It rejects failed
or stale source digests and requires clean compiler logs and device cleanup.
Windows x64/x86/scalar, native Clang and x64 AddressSanitizer pass. Quest 3 ARM64
and ARM32 compatibility both pass native numerical/allocation/concurrency suites
and measurements. Four Android ABI builds link test ELF files. Four Windows static
archives, 27 isolated public headers, 79 compile-time assertions and installed
CMake package checks pass. No C++ allocation/free was detected in audited regions.

## Still requires external environments

Desktop Linux/GCC and macOS/Apple Clang need their own hosts; WSL is not enabled.
Windows ARM64 application linking/runtime needs its target host/runtime libraries.
run_portable.py is prepared. Android execution does not validate those other ABIs.
Physical audio I/O/listening and unmeasured workloads are not claimed. Unreal and
original-code release licensing were not part of the selected completion batch.
