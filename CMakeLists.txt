cmake_minimum_required(VERSION 3.24)
project(CyDSP VERSION 0.2.0 LANGUAGES CXX)
if(CMAKE_GENERATOR_PLATFORM STREQUAL "ARM64EC" OR CMAKE_CXX_COMPILER_ARCHITECTURE_ID STREQUAL "ARM64EC")
    message(FATAL_ERROR "ARM64EC needs a separate CyDSP ABI/dispatch implementation")
endif()
set(CYDSP_PACKAGE_THREADS OFF)
set(CYDSP_PACKAGE_ALSA OFF)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
option(CYDSP_BUILD_TESTS "Build native regression executable (separate explicit validation)" OFF)
option(CYDSP_SANITIZE_ADDRESS "Instrument explicit native validation builds" OFF)
option(CYDSP_BUILD_MEASUREMENTS "Build explicitly requested native measurements" OFF)
option(CYDSP_BUILD_STRETCH "Build private MIT time/pitch stretching backend" ON)
option(CYDSP_ENABLE_SIMD "Build architecture-specific kernels" ON)
option(CYDSP_ENABLE_AVX512 "Build optional x64 AVX-512 kernels" ON)
option(CYDSP_ENABLE_SVE "Build optional ARM64 Linux/Android scalable SVE kernels" ON)
option(CYDSP_COMPILE_CHECKS "Compile regression sources without linking/running programs" OFF)
add_library(CyDSP STATIC
    src/synthesis/SourceBatch.cpp
    src/spatial/Diffraction.cpp
    src/analysis/ImpulseAnalysis.cpp
    src/spectral/HarmonicPercussive.cpp
    src/spectral/SpectralPitch.cpp
    src/synthesis/ContactExciter.cpp
    src/processors/EchoCanceller.cpp
    src/control/Quality.cpp
    src/analysis/Diagnostics.cpp
    src/processors/Restoration.cpp
    src/processors/AdaptiveFilter.cpp
    src/processors/Multirate.cpp
    src/synthesis/PhysicalModels.cpp
    src/processors/PitchEffects.cpp
    src/analysis/MusicalAnalyzer.cpp
    src/synthesis/AdvancedSynthesis.cpp
    src/synthesis/LiveGranulator.cpp
    src/spectral/SpectralEffects.cpp
    src/processors/AdvancedEffects.cpp
    src/processors/AdvancedDynamics.cpp
    src/spatial/HybridReverb.cpp
    src/spatial/SpatialRendering.cpp
    src/spatial/AcousticRooms.cpp
    src/spatial/FdnReverb.cpp
    src/spatial/AcousticPaths.cpp
    src/spatial/SpatialMath.cpp
    src/processors/BandGain.cpp
    src/processors/MultiBusGraph.cpp
    src/processors/FilterBank.cpp
    src/processors/Hilbert.cpp
    src/processors/Diffusion.cpp
    src/processors/AdvancedFilterDesign.cpp
    src/spectral/Stft.cpp
    src/core/CircularHistory.cpp
    src/control/Parameters.cpp
    src/processors/MultiTapDelay.cpp
    src/synthesis/Granulator.cpp
    src/analysis/LevelMeter.cpp
    src/analysis/TruePeakMeter.cpp
    src/analysis/SpectrumAnalyzer.cpp
    src/analysis/ConstantQAnalyzer.cpp
    src/analysis/LoudnessMeter.cpp
    src/processors/SincInterpolator.cpp
    src/synthesis/SampleData.cpp
    src/synthesis/Wavetable.cpp
    src/core/Cpu.cpp
    src/core/AudioValidation.cpp
    src/core/AudioLayout.cpp
    src/math/VectorOps.cpp
    src/math/Scalar.cpp
    src/math/WindowedSinc.cpp
    src/processors/Convolver.cpp
    src/processors/ChannelMatrix.cpp
    src/processors/SmoothedConvolver.cpp
    src/processors/SmoothedBiquadCascade.cpp
    src/processors/NonuniformConvolver.cpp
    src/processors/ScheduledConvolver.cpp
    src/processors/MatrixConvolver.cpp
    src/processors/AsyncConvolver.cpp
    src/processors/AudioGraph.cpp
    src/processors/Gain.cpp
    src/processors/Effects.cpp
    src/processors/Generators.cpp
    src/processors/Dynamics.cpp
    src/processors/PolynomialInterpolator.cpp
    src/processors/FilterDesign.cpp
    src/processors/TptFilter.cpp
    src/processors/Mixing.cpp
    src/processors/Oversampler.cpp
    src/processors/Fir.cpp
    src/processors/Biquad.cpp
    src/processors/BiquadBank.cpp
    src/processors/SmoothedBiquad.cpp
    src/processors/BiquadDesign.cpp
    src/processors/BiquadCascade.cpp
    src/processors/ComplexFft.cpp
    src/processors/RealFft.cpp
    src/processors/HybridConvolver.cpp
    src/processors/PartitionedConvolver.cpp
    src/processors/Resampler.cpp
    src/processors/DelayLine.cpp)
add_library(CyDSP::CyDSP ALIAS CyDSP)
target_compile_features(CyDSP PUBLIC cxx_std_20)
target_compile_definitions(CyDSP PRIVATE CYDSP_WITH_STRETCH=$<BOOL:${CYDSP_BUILD_STRETCH}>)
target_include_directories(CyDSP PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}> PRIVATE src)
set_target_properties(CyDSP PROPERTIES CXX_EXTENSIONS OFF POSITION_INDEPENDENT_CODE ON)
# Visual Studio may report the host system processor when cross-building ARM.
# Prefer the architecture identified from the target compiler/ABI.
if(MSVC AND CMAKE_CXX_COMPILER_ARCHITECTURE_ID)
    string(TOLOWER "${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}" cydsp_processor)
else()
    string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" cydsp_processor)
endif()
set(cydsp_sse2 0)
set(cydsp_avx2 0)
set(cydsp_neon 0)
set(cydsp_avx512 0)
set(cydsp_sve 0)
if(CYDSP_ENABLE_SIMD)
    if(cydsp_processor MATCHES "^(amd64|x86_64|x64|x86|i[3-6]86)$")
        set(cydsp_sse2 1)
        set(cydsp_avx2 1)
        target_sources(CyDSP PRIVATE src/math/Sse2.cpp src/math/Avx2.cpp)
        if(CYDSP_ENABLE_AVX512 AND CMAKE_SIZEOF_VOID_P EQUAL 8)
            set(cydsp_avx512 1)
            target_sources(CyDSP PRIVATE src/math/Avx512.cpp)
            if(MSVC)
                set_source_files_properties(src/math/Avx512.cpp PROPERTIES COMPILE_OPTIONS "/arch:AVX512")
            else()
                # Clang models FMA as a prerequisite of AVX-512F; -mno-fma disables AVX-512.
                # The target-wide -ffp-contract=off still forbids fused contraction.
                set_source_files_properties(src/math/Avx512.cpp PROPERTIES COMPILE_OPTIONS "-mavx512f;-mavx512dq;-mavx512cd;-mavx512bw;-mavx512vl")
            endif()
        endif()
        if(MSVC)
            set_source_files_properties(src/math/Avx2.cpp PROPERTIES COMPILE_OPTIONS "/arch:AVX2")
            if(CMAKE_SIZEOF_VOID_P EQUAL 4)
                set_source_files_properties(src/math/Sse2.cpp PROPERTIES COMPILE_OPTIONS "/arch:SSE2")
            endif()
        else()
            set_source_files_properties(src/math/Sse2.cpp PROPERTIES COMPILE_OPTIONS "-msse2")
            set_source_files_properties(src/math/Avx2.cpp PROPERTIES COMPILE_OPTIONS "-mavx2;-mno-fma")
        endif()
    elseif(cydsp_processor MATCHES "^(arm64|aarch64|armv[7-8].*|arm)$")
        set(cydsp_neon 1)
        target_sources(CyDSP PRIVATE src/math/Neon.cpp)
        if(CYDSP_ENABLE_SVE AND NOT MSVC AND CMAKE_SYSTEM_NAME MATCHES "^(Linux|Android)$" AND cydsp_processor MATCHES "^(arm64|aarch64)$")
            set(cydsp_sve 1)
            target_sources(CyDSP PRIVATE src/math/Sve.cpp)
            set_source_files_properties(src/math/Sve.cpp PROPERTIES COMPILE_OPTIONS "-march=armv8-a+sve;-msve-vector-bits=scalable")
        endif()
        if(NOT MSVC AND NOT cydsp_processor MATCHES "^(arm64|aarch64)$")
            set_source_files_properties(src/math/Neon.cpp PROPERTIES COMPILE_OPTIONS "-mfpu=neon")
        endif()
    endif()
endif()
target_compile_definitions(CyDSP PRIVATE CYDSP_HAS_SSE2=${cydsp_sse2}
    CYDSP_HAS_AVX2=${cydsp_avx2} CYDSP_HAS_NEON=${cydsp_neon}
    CYDSP_HAS_AVX512=${cydsp_avx512} CYDSP_HAS_SVE=${cydsp_sve})
if(MSVC)
    target_compile_options(CyDSP PRIVATE /W4 /permissive- /fp:strict /utf-8)
else()
    target_compile_options(CyDSP PRIVATE -Wall -Wextra -Wpedantic -ffp-contract=off)
endif()
if(CYDSP_BUILD_STRETCH)
    add_library(CyDSPStretch STATIC src/processors/TimeStretch.cpp)
    add_library(CyDSP::Stretch ALIAS CyDSPStretch)
    set_target_properties(CyDSPStretch PROPERTIES EXPORT_NAME Stretch CXX_EXTENSIONS OFF POSITION_INDEPENDENT_CODE ON)
    target_link_libraries(CyDSPStretch PUBLIC CyDSP::CyDSP)
    target_include_directories(CyDSPStretch PRIVATE src)
    if(MSVC)
        target_compile_options(CyDSPStretch PRIVATE /W4 /permissive- /fp:strict /utf-8)
    else()
        target_compile_options(CyDSPStretch PRIVATE -Wall -Wextra -Wpedantic -ffp-contract=off)
    endif()
    install(TARGETS CyDSPStretch EXPORT CyDSPTargets ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()

install(TARGETS CyDSP EXPORT CyDSPTargets ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT CyDSPTargets NAMESPACE CyDSP:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/CyDSP)
configure_package_config_file(cmake/CyDSPConfig.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/CyDSPConfig.cmake INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/CyDSP)
write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/CyDSPConfigVersion.cmake VERSION ${PROJECT_VERSION} COMPATIBILITY SameMinorVersion)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/CyDSPConfig.cmake ${CMAKE_CURRENT_BINARY_DIR}/CyDSPConfigVersion.cmake DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/CyDSP)
install(DIRECTORY third_party/licenses/ DESTINATION ${CMAKE_INSTALL_DATADIR}/CyDSP/licenses)
install(FILES LICENSE third_party/manifest.json docs/Licensing.md DESTINATION ${CMAKE_INSTALL_DATADIR}/CyDSP)
set(cydsp_test_sources
 tests/AdvancedExpansionContracts.cpp
 tests/AdvancedExpansionRegression.cpp
 tests/ExpansionContracts.cpp
 tests/RestorationDiagnosticsRegression.cpp
 tests/MusicalPhysicalRegression.cpp
 tests/CreativeRegression.cpp
 tests/AdvancedDynamicsRegression.cpp
 tests/SpatialRenderingRegression.cpp
 tests/AcousticRegression.cpp
 tests/SpatialFoundationRegression.cpp
 tests/MultiBusGraphRegression.cpp
 tests/AdvancedFoundationRegression.cpp
 tests/FoundationRegression.cpp
 tests/GranulationRegression.cpp
 tests/AudioAnalysisRegression.cpp
 tests/AdvancedRegression.cpp
 tests/CompletionRegression.cpp
 tests/Contracts.cpp
 tests/DawExpansionRegression.cpp
 tests/ExpandedRegression.cpp
 tests/FiltersRegression.cpp
 tests/MatrixSchedulingRegression.cpp
 tests/OptimizationRegression.cpp
 tests/PerformanceRegression.cpp
 tests/RealtimeRegression.cpp
 tests/Regression.cpp
 tests/RoutingRegression.cpp
 tests/SchedulingRegression.cpp
 tests/StretchFftRegression.cpp
)
if(CYDSP_BUILD_TESTS)
 enable_testing()
 find_package(Threads REQUIRED)
 add_executable(cydsp_tests ${cydsp_test_sources})
 target_link_libraries(cydsp_tests PRIVATE CyDSP::CyDSP Threads::Threads)
 target_include_directories(cydsp_tests PRIVATE src)
 add_test(NAME cydsp_regression COMMAND cydsp_tests)
endif()
if(CYDSP_COMPILE_CHECKS)
 file(GLOB_RECURSE headers CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/cydsp/*.h")
 set(header_sources)
 foreach(header IN LISTS headers)
  file(RELATIVE_PATH relative "${CMAKE_CURRENT_SOURCE_DIR}/include" "${header}")
  string(MAKE_C_IDENTIFIER "${relative}" id)
  file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/header-checks/${id}.cpp" CONTENT "#include <${relative}>\n")
  list(APPEND header_sources "${CMAKE_CURRENT_BINARY_DIR}/header-checks/${id}.cpp")
 endforeach()
 add_library(cydsp_header_checks OBJECT ${header_sources})
 target_link_libraries(cydsp_header_checks PRIVATE CyDSP::CyDSP)
 add_library(cydsp_contract_checks OBJECT ${cydsp_test_sources} examples/Processing.cpp examples/GranulationAndAnalysis.cpp examples/SpatialAndDaw.cpp examples/AdvancedAudio.cpp)
 target_link_libraries(cydsp_contract_checks PRIVATE CyDSP::CyDSP)
 target_include_directories(cydsp_contract_checks PRIVATE src)
 target_compile_definitions(cydsp_contract_checks PRIVATE CYDSP_HAS_SSE2=${cydsp_sse2} CYDSP_HAS_AVX2=${cydsp_avx2} CYDSP_HAS_NEON=${cydsp_neon} CYDSP_HAS_AVX512=${cydsp_avx512} CYDSP_HAS_SVE=${cydsp_sve})
 add_library(cydsp_measurement_checks OBJECT tests/Measurements.cpp tests/ExpansionMeasurements.cpp tests/AdvancedExpansionMeasurements.cpp)
 add_library(cydsp_focused_measurement_checks OBJECT tests/FocusedMeasurements.cpp)
 target_link_libraries(cydsp_focused_measurement_checks PRIVATE CyDSP::CyDSP)
 target_link_libraries(cydsp_measurement_checks PRIVATE CyDSP::CyDSP)
 target_include_directories(cydsp_measurement_checks PRIVATE src)
endif()
if(CYDSP_BUILD_MEASUREMENTS)
 add_executable(cydsp_focused_measurements tests/FocusedMeasurements.cpp)
 target_link_libraries(cydsp_focused_measurements PRIVATE CyDSP::CyDSP)
 add_executable(cydsp_measurements tests/Measurements.cpp tests/ExpansionMeasurements.cpp tests/AdvancedExpansionMeasurements.cpp)
 target_link_libraries(cydsp_measurements PRIVATE CyDSP::CyDSP)
 target_include_directories(cydsp_measurements PRIVATE src)
endif()
foreach(t IN ITEMS cydsp_tests cydsp_contract_checks cydsp_header_checks cydsp_measurements cydsp_focused_measurements cydsp_measurement_checks cydsp_focused_measurement_checks)
 if(TARGET ${t})
  if(MSVC)
   target_compile_options(${t} PRIVATE /W4 /permissive- /fp:strict /utf-8)
  else()
   target_compile_options(${t} PRIVATE -Wall -Wextra -Wpedantic -ffp-contract=off)
  endif()
 endif()
endforeach()
if(CYDSP_SANITIZE_ADDRESS)
 foreach(t IN ITEMS CyDSP CyDSPStretch cydsp_tests cydsp_measurements cydsp_focused_measurements)
  if(TARGET ${t})
   if(MSVC)
    target_compile_options(${t} PRIVATE /fsanitize=address /Zi)
    target_link_options(${t} PRIVATE /DEBUG /INCREMENTAL:NO)
   else()
    target_compile_options(${t} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(${t} PRIVATE -fsanitize=address,undefined)
   endif()
  endif()
 endforeach()
endif()
if(CYDSP_BUILD_STRETCH)
    foreach(cydsp_measure_target IN ITEMS cydsp_measurements cydsp_measurement_checks)
        if(TARGET ${cydsp_measure_target})
            target_link_libraries(${cydsp_measure_target} PRIVATE CyDSP::Stretch)
            target_compile_definitions(${cydsp_measure_target} PRIVATE CYDSP_MEASURE_STRETCH=1)
            target_include_directories(${cydsp_measure_target} SYSTEM PRIVATE third_party/signalsmith-stretch third_party/signalsmith-linear/include)
        endif()
    endforeach()
endif()
