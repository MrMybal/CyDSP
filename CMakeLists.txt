cmake_minimum_required(VERSION 3.24)
project(CyDSP VERSION 0.1.0 LANGUAGES CXX)
if(CMAKE_GENERATOR_PLATFORM STREQUAL "ARM64EC" OR CMAKE_CXX_COMPILER_ARCHITECTURE_ID STREQUAL "ARM64EC")
    message(FATAL_ERROR "ARM64EC needs a separate CyDSP ABI/dispatch implementation")
endif()
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
option(CYDSP_BUILD_TESTS "Build native regression executable (separate explicit validation)" OFF)
option(CYDSP_SANITIZE_ADDRESS "Instrument explicit native validation builds" OFF)
option(CYDSP_BUILD_MEASUREMENTS "Build explicitly requested native measurements" OFF)
option(CYDSP_ENABLE_SIMD "Build architecture-specific kernels" ON)
option(CYDSP_ENABLE_AVX512 "Build optional x64 AVX-512 kernels" ON)
option(CYDSP_ENABLE_SVE "Build optional ARM64 Linux/Android scalable SVE kernels" ON)
option(CYDSP_COMPILE_CHECKS "Compile regression sources without linking/running programs" OFF)
add_library(CyDSP STATIC
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
if(CYDSP_COMPILE_CHECKS)
    # Compile every installed public header alone, without the umbrella/private includes.
    file(GLOB_RECURSE cydsp_public_headers CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/cydsp/*.h")
    set(cydsp_header_sources)
    foreach(cydsp_header IN LISTS cydsp_public_headers)
        file(RELATIVE_PATH cydsp_header_relative "${CMAKE_CURRENT_SOURCE_DIR}/include" "${cydsp_header}")
        string(MAKE_C_IDENTIFIER "${cydsp_header_relative}" cydsp_header_id)
        set(cydsp_header_source "${CMAKE_CURRENT_BINARY_DIR}/header-checks/${cydsp_header_id}.cpp")
        file(GENERATE OUTPUT "${cydsp_header_source}" CONTENT "#include <${cydsp_header_relative}>\n")
        list(APPEND cydsp_header_sources "${cydsp_header_source}")
    endforeach()
    add_library(cydsp_header_checks OBJECT ${cydsp_header_sources})
    target_link_libraries(cydsp_header_checks PRIVATE CyDSP::CyDSP)
    add_library(cydsp_contract_checks OBJECT tests/Contracts.cpp tests/Regression.cpp tests/AdvancedRegression.cpp tests/FiltersRegression.cpp tests/RoutingRegression.cpp tests/CompletionRegression.cpp tests/RealtimeRegression.cpp tests/OptimizationRegression.cpp tests/SchedulingRegression.cpp tests/MatrixSchedulingRegression.cpp examples/Processing.cpp)
    target_link_libraries(cydsp_contract_checks PRIVATE CyDSP::CyDSP)
    target_include_directories(cydsp_contract_checks PRIVATE src)
    target_compile_definitions(cydsp_contract_checks PRIVATE CYDSP_HAS_SSE2=${cydsp_sse2}
        CYDSP_HAS_AVX2=${cydsp_avx2} CYDSP_HAS_NEON=${cydsp_neon}
    CYDSP_HAS_AVX512=${cydsp_avx512} CYDSP_HAS_SVE=${cydsp_sve})
    add_library(cydsp_measurement_checks OBJECT tests/Measurements.cpp)
    target_link_libraries(cydsp_measurement_checks PRIVATE CyDSP::CyDSP)
    target_include_directories(cydsp_measurement_checks PRIVATE src)
    foreach(cydsp_checks IN ITEMS cydsp_contract_checks cydsp_header_checks cydsp_measurement_checks)
        if(MSVC)
            target_compile_options(${cydsp_checks} PRIVATE /W4 /permissive- /fp:strict /utf-8)
        else()
            target_compile_options(${cydsp_checks} PRIVATE -Wall -Wextra -Wpedantic -ffp-contract=off)
        endif()
    endforeach()
endif()
install(TARGETS CyDSP EXPORT CyDSPTargets ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT CyDSPTargets NAMESPACE CyDSP:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/CyDSP)
configure_package_config_file(cmake/CyDSPConfig.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/CyDSPConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/CyDSP)
write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/CyDSPConfigVersion.cmake
    VERSION ${PROJECT_VERSION} COMPATIBILITY SameMinorVersion)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/CyDSPConfig.cmake ${CMAKE_CURRENT_BINARY_DIR}/CyDSPConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/CyDSP)
install(DIRECTORY third_party/licenses/ DESTINATION ${CMAKE_INSTALL_DATADIR}/CyDSP/licenses)
install(FILES third_party/manifest.json docs/Licensing.md DESTINATION ${CMAKE_INSTALL_DATADIR}/CyDSP)

if(CYDSP_BUILD_TESTS)
    enable_testing()
    add_executable(cydsp_tests tests/Regression.cpp tests/AdvancedRegression.cpp tests/FiltersRegression.cpp tests/RoutingRegression.cpp tests/CompletionRegression.cpp tests/RealtimeRegression.cpp tests/OptimizationRegression.cpp tests/SchedulingRegression.cpp tests/MatrixSchedulingRegression.cpp tests/Contracts.cpp)
    find_package(Threads REQUIRED)
    target_link_libraries(cydsp_tests PRIVATE CyDSP::CyDSP Threads::Threads)
    target_include_directories(cydsp_tests PRIVATE src)
    add_test(NAME cydsp_regression COMMAND cydsp_tests)
endif()

if(CYDSP_BUILD_MEASUREMENTS)
    add_executable(cydsp_measurements tests/Measurements.cpp)
    target_link_libraries(cydsp_measurements PRIVATE CyDSP::CyDSP)
    target_include_directories(cydsp_measurements PRIVATE src)
endif()
foreach(cydsp_native IN ITEMS cydsp_tests cydsp_measurements)
    if(TARGET ${cydsp_native})
        if(MSVC)
            target_compile_options(${cydsp_native} PRIVATE /W4 /permissive- /fp:strict /utf-8)
        else()
            target_compile_options(${cydsp_native} PRIVATE -Wall -Wextra -Wpedantic -ffp-contract=off)
        endif()
    endif()
endforeach()

if(CYDSP_SANITIZE_ADDRESS)
    foreach(cydsp_instrumented IN ITEMS CyDSP cydsp_tests cydsp_measurements)
        if(TARGET ${cydsp_instrumented})
            if(MSVC)
                target_compile_options(${cydsp_instrumented} PRIVATE /fsanitize=address /Zi)
                if(NOT cydsp_instrumented STREQUAL "CyDSP")
                    target_link_options(${cydsp_instrumented} PRIVATE /DEBUG /INCREMENTAL:NO)
                endif()
            else()
                target_compile_options(${cydsp_instrumented} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
                target_link_options(${cydsp_instrumented} PRIVATE -fsanitize=address,undefined)
            endif()
        endif()
    endforeach()
endif()
