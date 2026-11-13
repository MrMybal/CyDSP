# Isolated ISPC wavefolder comparison

Original MIT experiment. This directory is not part of the production CyDSP build
or installed package. It requires an explicitly supplied ISPC compiler, an x64
AVX2-capable CPU/OS and a compatible Release CyDSP archive. No compiler is
downloaded, installed or redistributed. No task runtime or GPU is used.

Configure this directory as a separate CMake project with `ISPC_EXECUTABLE`,
`CYDSP_ARCHIVE` and optionally `CYDSP_INCLUDE`. Build `cydsp_ispc_comparison`,
then execute it explicitly. Record `ispc --version` alongside its JSON output.
The measured candidate includes validation; the library path includes its own
validation. The comparison checks 204800 rendered values before timing. Results
apply only to this wavefolder workload, compiler and machine.

The ISPC candidate has no parameter automation, hysteresis or general-shape API.
It is a benchmark implementation, not a production processor. CyDSP's production
AVX2/AVX-512 implementation remains original C++ with intrinsics and a scalar
fallback; adopting ISPC is not required to obtain the measured optimization.
