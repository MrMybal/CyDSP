# CyDSP development

Work only inside this project unless the user explicitly changes scope.
Use C++20 and namespace cydsp. Read README.md and docs/Architecture.md.
Keep public audio conventions common across processors. prepare may allocate;
process/reset must not allocate, block, perform I/O or mutate global CPU state.
Keep ISA flags local to their implementation files. Never use global -march=native,
/arch:AVX2, fast-math or FMA contraction for the portable library.
Retain exact upstream notices and pinned provenance for adapted third-party code.
Original CyDSP code uses the MIT license; preserve LICENSE and all upstream notices.
Production builds create static libraries only. Native tests/examples/benchmarks
must not be launched implicitly. Source/data and compile-only checks are allowed;
record runtime and unexecuted architecture coverage honestly.
Do not create or enable GitHub Actions without explicit authorization.

The owner explicitly authorized native regression/benchmark/quality runs on
2026-11-11. Keep production presets static and tests off; use separate opt-in
validation builds. Do not change antivirus settings or GitHub Actions.
