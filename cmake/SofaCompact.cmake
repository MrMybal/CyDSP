target_sources(CyDSPSofa PRIVATE
    src/sofa/SofaCompact.cpp
    src/sofa/compact/Helpers.c
    src/sofa/compact/hrtf/reader.c
    src/sofa/compact/hdf/superblock.c
    src/sofa/compact/hdf/dataobject.c
    src/sofa/compact/hdf/btree.c
    src/sofa/compact/hdf/fractalhead.c
    src/sofa/compact/hdf/gcol.c
    third_party/miniz/miniz_tinfl.c)
target_include_directories(CyDSPSofa PRIVATE src/sofa/compact src/sofa/compact/hrtf third_party/miniz)
target_compile_definitions(CyDSPSofa PRIVATE
    MINIZ_NO_ARCHIVE_APIS MINIZ_NO_DEFLATE_APIS
    MINIZ_NO_STDIO MINIZ_NO_ZLIB_APIS MINIZ_NO_ZLIB_COMPATIBLE_NAMES MINIZ_NO_MALLOC
    tinfl_decompress=cydsp_sofa_tinfl_decompress
    tinfl_decompress_mem_to_heap=cydsp_sofa_tinfl_decompress_mem_to_heap
    tinfl_decompress_mem_to_mem=cydsp_sofa_tinfl_decompress_mem_to_mem
    tinfl_decompress_mem_to_callback=cydsp_sofa_tinfl_decompress_mem_to_callback)
