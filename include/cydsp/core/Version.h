#pragma once
// Package version. Keep in step with project(CyDSP VERSION ...) in CMakeLists.txt;
// scripts/verify_sources.py rejects a mismatch.
#define CYDSP_VERSION_MAJOR 0
#define CYDSP_VERSION_MINOR 4
#define CYDSP_VERSION_PATCH 4
namespace cydsp {
inline constexpr int versionMajor = CYDSP_VERSION_MAJOR, versionMinor = CYDSP_VERSION_MINOR,
                     versionPatch = CYDSP_VERSION_PATCH;
}
