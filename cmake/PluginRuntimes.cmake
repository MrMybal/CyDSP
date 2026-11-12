# Original CyDSP build adapter for pinned permissive plugin runtimes.
enable_language(C)
find_package(Threads REQUIRED)
set(CYDSP_PACKAGE_THREADS ON)
set(cydsp_runtime_sources)
list(APPEND cydsp_runtime_sources
    third_party/lilv/src/collections.c
    third_party/lilv/src/dylib.c
    third_party/lilv/src/instance.c
    third_party/lilv/src/lib.c
    third_party/lilv/src/load_skimmer.c
    third_party/lilv/src/node.c
    third_party/lilv/src/node_hash.c
    third_party/lilv/src/node_skimmer.c
    third_party/lilv/src/plugin.c
    third_party/lilv/src/pluginclass.c
    third_party/lilv/src/port.c
    third_party/lilv/src/query.c
    third_party/lilv/src/scalepoint.c
    third_party/lilv/src/state.c
    third_party/lilv/src/string_util.c
    third_party/lilv/src/syntax_skimmer.c
    third_party/lilv/src/sys_util.c
    third_party/lilv/src/type_skimmer.c
    third_party/lilv/src/ui.c
    third_party/lilv/src/uris.c
    third_party/lilv/src/world.c
)
list(APPEND cydsp_runtime_sources
    third_party/serd/src/base64.c
    third_party/serd/src/byte_source.c
    third_party/serd/src/env.c
    third_party/serd/src/n3.c
    third_party/serd/src/node.c
    third_party/serd/src/reader.c
    third_party/serd/src/read_utf8.c
    third_party/serd/src/string.c
    third_party/serd/src/system.c
    third_party/serd/src/uri.c
    third_party/serd/src/writer.c
)
list(APPEND cydsp_runtime_sources
    third_party/sord/src/sord.c
    third_party/sord/src/syntax.c
)
list(APPEND cydsp_runtime_sources
    third_party/sratom/src/sratom.c
)
list(APPEND cydsp_runtime_sources
    third_party/zix/src/allocator.c
    third_party/zix/src/btree.c
    third_party/zix/src/bump_allocator.c
    third_party/zix/src/digest.c
    third_party/zix/src/errno_status.c
    third_party/zix/src/filesystem.c
    third_party/zix/src/hash.c
    third_party/zix/src/path.c
    third_party/zix/src/ring.c
    third_party/zix/src/status.c
    third_party/zix/src/string_view.c
    third_party/zix/src/system.c
    third_party/zix/src/tree.c
)
if(WIN32)
list(APPEND cydsp_runtime_sources
    third_party/zix/src/win32/environment_win32.c
    third_party/zix/src/win32/filesystem_win32.c
    third_party/zix/src/win32/sem_win32.c
    third_party/zix/src/win32/system_win32.c
    third_party/zix/src/win32/thread_win32.c
    third_party/zix/src/win32/win32_util.c
)
else()
list(APPEND cydsp_runtime_sources
    third_party/zix/src/posix/environment_posix.c
    third_party/zix/src/posix/filesystem_posix.c
    third_party/zix/src/posix/sem_posix.c
    third_party/zix/src/posix/system_posix.c
    third_party/zix/src/posix/thread_posix.c
)
endif()
list(APPEND cydsp_runtime_sources
    src/detail/JsfxRuntime.cpp
    src/detail/JsfxEelApi.cpp
    third_party/ysfx/sources/ysfx_api_file.cpp
    third_party/ysfx/sources/ysfx_api_gfx.cpp
    third_party/ysfx/sources/ysfx_api_reaper.cpp
    third_party/ysfx/sources/ysfx_audio_flac.cpp
    third_party/ysfx/sources/ysfx_audio_wav.cpp
    third_party/ysfx/sources/ysfx_config.cpp
    third_party/ysfx/sources/ysfx_eel_utils.cpp
    third_party/ysfx/sources/ysfx_midi.cpp
    third_party/ysfx/sources/ysfx_parse.cpp
    third_party/ysfx/sources/ysfx_parse_menu.cpp
    third_party/ysfx/sources/ysfx_preset.cpp
    third_party/ysfx/sources/ysfx_reader.cpp
    third_party/ysfx/sources/ysfx_utils.cpp
    third_party/ysfx/sources/ysfx_utils_fts.cpp
    third_party/ysfx/thirdparty/WDL/source/WDL/eel2/nseel-caltab.c
    third_party/ysfx/thirdparty/WDL/source/WDL/eel2/nseel-cfunc.c
    third_party/ysfx/thirdparty/WDL/source/WDL/eel2/nseel-compiler.c
    third_party/ysfx/thirdparty/WDL/source/WDL/eel2/nseel-eval.c
    third_party/ysfx/thirdparty/WDL/source/WDL/eel2/nseel-lextab.c
    third_party/ysfx/thirdparty/WDL/source/WDL/eel2/nseel-ram.c
    third_party/ysfx/thirdparty/WDL/source/WDL/eel2/nseel-yylex.c
    third_party/ysfx/thirdparty/WDL/source/WDL/fft.c
)
if(WIN32)
    list(APPEND cydsp_runtime_sources third_party/ysfx/thirdparty/WDL/source/WDL/win32_utf8.c)
endif()
add_library(CyDSPPluginRuntime OBJECT ${cydsp_runtime_sources})
set_target_properties(CyDSPPluginRuntime PROPERTIES POSITION_INDEPENDENT_CODE ON CXX_EXTENSIONS OFF)
target_compile_features(CyDSPPluginRuntime PRIVATE cxx_std_20)
target_compile_definitions(CyDSPPluginRuntime PRIVATE
    LILV_STATIC SERD_STATIC SORD_STATIC SRATOM_STATIC ZIX_STATIC
    EEL_TARGET_PORTABLE YSFX_NO_FTS WDL_FFT_REALSIZE=8
    WDL_LINEPARSE_ATOF=ysfx_wdl_atof NSEEL_ATOF=ysfx_wdl_atof
    _FILE_OFFSET_BITS=64 _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_WARNINGS NOMINMAX)
set(cydsp_runtime_includes
    third_party/lv2/include third_party/lilv/include third_party/serd/include
    third_party/sord/include third_party/sratom/include third_party/zix/include
    third_party/ysfx/include third_party/ysfx/sources
    third_party/ysfx/thirdparty/WDL/source third_party/ysfx/thirdparty/dr_libs)
target_include_directories(CyDSPPluginRuntime SYSTEM PRIVATE ${cydsp_runtime_includes})
target_link_libraries(CyDSPPluginRuntime PRIVATE Threads::Threads)
# Upstream headers/sources remain byte-exact. Warning policy is local to them.
if(MSVC)
    target_compile_options(CyDSPPluginRuntime PRIVATE /W0 /fp:strict /utf-8)
else()
    target_compile_options(CyDSPPluginRuntime PRIVATE -w -ffp-contract=off -fsigned-char)
    target_compile_definitions(CyDSPPluginRuntime PRIVATE _POSIX_C_SOURCE=200809L _XOPEN_SOURCE=600)
    if(ANDROID)
        target_compile_definitions(CyDSPPluginRuntime PRIVATE HAVE_COPY_FILE_RANGE=0 HAVE_POSIX_FADVISE=0)
    endif()
endif()
target_sources(CyDSPPlugins PRIVATE $<TARGET_OBJECTS:CyDSPPluginRuntime>)
target_include_directories(CyDSPPlugins SYSTEM PRIVATE ${cydsp_runtime_includes})
target_compile_definitions(CyDSPPlugins PRIVATE NOMINMAX YSFX_NO_FTS EEL_TARGET_PORTABLE WDL_FFT_REALSIZE=8 LILV_STATIC SERD_STATIC SORD_STATIC SRATOM_STATIC ZIX_STATIC)
target_link_libraries(CyDSPPlugins PRIVATE Threads::Threads)

if(WIN32)
    target_sources(CyDSPPluginRuntime PRIVATE
        third_party/ysfx/thirdparty/WDL/source/WDL/lice/lice.cpp
        third_party/ysfx/thirdparty/WDL/source/WDL/lice/lice_arc.cpp
        third_party/ysfx/thirdparty/WDL/source/WDL/lice/lice_colorspace.cpp
        third_party/ysfx/thirdparty/WDL/source/WDL/lice/lice_image.cpp
        third_party/ysfx/thirdparty/WDL/source/WDL/lice/lice_line.cpp
        third_party/ysfx/thirdparty/WDL/source/WDL/lice/lice_palette.cpp
        third_party/ysfx/thirdparty/WDL/source/WDL/lice/lice_texgen.cpp
        third_party/ysfx/thirdparty/WDL/source/WDL/lice/lice_text.cpp
        third_party/ysfx/thirdparty/WDL/source/WDL/lice/lice_textnew.cpp
        third_party/ysfx/sources/lice_stb/lice_stb_bmp.cpp
        third_party/ysfx/sources/lice_stb/lice_stb_gif.cpp
        third_party/ysfx/sources/lice_stb/lice_stb_jpg.cpp
        third_party/ysfx/sources/lice_stb/lice_stb_loaders.cpp
        third_party/ysfx/sources/lice_stb/lice_stb_png.cpp
        third_party/ysfx/sources/lice_stb/lice_stb_write.cpp
    )
    target_include_directories(CyDSPPluginRuntime SYSTEM PRIVATE third_party/ysfx/thirdparty/stb)
    target_link_libraries(CyDSPPlugins PRIVATE user32 gdi32 comdlg32)
else()
    target_compile_definitions(CyDSPPluginRuntime PRIVATE YSFX_NO_GFX)
    target_compile_definitions(CyDSPPlugins PRIVATE YSFX_NO_GFX)
endif()
