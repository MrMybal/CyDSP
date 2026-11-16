#include "sofa/SofaDetail.h"
#include <algorithm>
#include <cstring>
#include <hdf5.h>
#include <mutex>
namespace cydsp::sofa_detail {
namespace {
struct Id {
    hid_t id = -1;
    herr_t (*close)(hid_t) = nullptr;
    Id(hid_t value, herr_t (*fn)(hid_t)) : id(value), close(fn) {
        if (id < 0)
            fail(SofaError::invalid_file);
    }
    ~Id() {
        if (id >= 0)
            close(id);
    }
    operator hid_t() const {
        return id;
    }
    Id(const Id&) = delete;
};
struct Errors {
    H5E_auto2_t handler = nullptr;
    void* context = nullptr;
    Errors() {
        H5Eget_auto2(H5E_DEFAULT, &handler, &context);
        H5Eset_auto2(H5E_DEFAULT, nullptr, nullptr);
    }
    ~Errors() {
        H5Eset_auto2(H5E_DEFAULT, handler, context);
    }
};
std::string attribute(hid_t object, const char* name, Budget& metadata) {
    if (H5Aexists(object, name) <= 0)
        return {};
    Id a(H5Aopen(object, name, H5P_DEFAULT), H5Aclose);
    Id type(H5Aget_type(a), H5Tclose), space(H5Aget_space(a), H5Sclose);
    if (H5Sget_simple_extent_npoints(space) == 0)
        return {};
    if (H5Sget_simple_extent_npoints(space) != 1 || H5Tget_class(type) != H5T_STRING)
        fail(SofaError::invalid_file);
    std::string result;
    if (H5Tis_variable_str(type) > 0) {
        char* text = nullptr;
        if (H5Aread(a, type, &text) < 0)
            fail(SofaError::invalid_file);
        if (text) {
            const auto n = std::strlen(text);
            if (n > metadata.remaining) {
                H5free_memory(text);
                fail(SofaError::limit_exceeded);
            }
            metadata.consume(n);
            try {
                result.assign(text, n);
            } catch (...) {
                H5free_memory(text);
                throw;
            }
            H5free_memory(text);
        }
    } else {
        const auto n = H5Tget_size(type);
        metadata.consume(n);
        result.resize(n);
        if (H5Aread(a, type, result.data()) < 0)
            fail(SofaError::invalid_file);
        const auto zero = result.find('\0');
        if (zero != std::string::npos)
            result.resize(zero);
        while (!result.empty() && result.back() == ' ')
            result.pop_back();
    }
    return result;
}
Array array(hid_t file, const char* name, const SofaLoadOptions& o, Budget& budget,
            Budget& metadata) {
    H5L_info_t link{};
    if (H5Lget_info(file, name, &link, H5P_DEFAULT) < 0 || link.type != H5L_TYPE_HARD)
        fail(SofaError::unsupported_storage);
    Id d(H5Dopen2(file, name, H5P_DEFAULT), H5Dclose);
    Id type(H5Dget_type(d), H5Tclose), space(H5Dget_space(d), H5Sclose);
    if (H5Tget_class(type) != H5T_FLOAT || (H5Tget_size(type) != 4 && H5Tget_size(type) != 8))
        fail(SofaError::unsupported_storage);
    const auto rank = H5Sget_simple_extent_ndims(space);
    if (rank < 1 || rank > 3)
        fail(SofaError::invalid_dimensions);
    hsize_t dims[3]{};
    if (H5Sget_simple_extent_dims(space, dims, nullptr) < 0)
        fail(SofaError::invalid_file);
    Array a;
    std::size_t count = 1;
    for (int i = 0; i < rank; ++i) {
        if (!dims[i] || dims[i] > std::max<std::size_t>(o.maximumPoints, o.maximumTaps))
            fail(SofaError::limit_exceeded);
        count = product(count, std::size_t(dims[i]), o.maximumDecodedBytes / sizeof(double));
        a.shape.push_back(std::size_t(dims[i]));
    }
    if (std::strcmp(name, "Data.IR") == 0 &&
        (rank != 3 || dims[1] != 2 || dims[0] > o.maximumPoints || dims[2] > o.maximumTaps))
        fail(SofaError::limit_exceeded);
    budget.consume(product(count, sizeof(double), o.maximumDecodedBytes));
    Id creation(H5Dget_create_plist(d), H5Pclose);
    const auto layout = H5Pget_layout(creation);
    if (layout == H5D_VIRTUAL || H5Pget_external_count(creation) != 0)
        fail(SofaError::unsupported_storage);
    if (layout == H5D_CHUNKED) {
        hsize_t chunk[3]{};
        if (H5Pget_chunk(creation, 3, chunk) != rank)
            fail(SofaError::invalid_file);
        std::size_t size = H5Tget_size(type);
        for (int i = 0; i < rank; ++i)
            size = product(size, std::size_t(chunk[i]), o.maximumDecodedBytes);
    }
    const int filters = H5Pget_nfilters(creation);
    if (filters < 0 || filters > 8)
        fail(SofaError::unsupported_storage);
    for (int i = 0; i < filters; ++i) {
        unsigned flags = 0;
        std::size_t n = 0;
        const auto filter =
            H5Pget_filter2(creation, unsigned(i), &flags, &n, nullptr, 0, nullptr, nullptr);
        if (filter != H5Z_FILTER_DEFLATE && filter != H5Z_FILTER_SHUFFLE &&
            filter != H5Z_FILTER_FLETCHER32)
            fail(SofaError::unsupported_storage);
        // Reject unknown filters before any plugin-capable availability/read call.
        if (H5Zfilter_avail(filter) <= 0)
            fail(SofaError::unsupported_storage);
    }
    a.values.resize(count);
    if (H5Dread(d, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, a.values.data()) < 0)
        fail(SofaError::invalid_file);
    a.precision = unsigned(H5Tget_precision(type));
    a.type = attribute(d, "Type", metadata);
    a.units = attribute(d, "Units", metadata);
    return a;
}
} // namespace
Raw readComplete(std::span<const std::byte> bytes, const SofaLoadOptions& o) {
    // Only import calls use this lock. HDF5 may be supplied as a non-threadsafe SDK.
    static std::mutex mutex;
    std::lock_guard guard(mutex);
    Errors errors;
    Id access(H5Pcreate(H5P_FILE_ACCESS), H5Pclose);
    if (H5Pset_fapl_core(access, 65536, 0) < 0 ||
        H5Pset_file_image(access, const_cast<std::byte*>(bytes.data()), bytes.size()) < 0)
        fail(SofaError::invalid_file);
    // File-image + core driver: no path is opened and no backing store is written.
    Id file(H5Fopen("cydsp-sofa-memory", H5F_ACC_RDONLY, access), H5Fclose);
    Raw raw;
    Budget budget{o.maximumDecodedBytes}, metadata{o.maximumMetadataBytes};
    for (const auto* name : {"Conventions", "SOFAConventions", "DataType", "Title", "DatabaseName",
                             "ListenerShortName", "License"})
        raw.attributes[name] = attribute(file, name, metadata);
    for (const auto* name :
         {"Data.IR", "Data.SamplingRate", "Data.Delay", "SourcePosition", "ReceiverPosition",
          "ListenerPosition", "ListenerView", "ListenerUp", "EmitterPosition"}) {
        if (H5Lexists(file, name, H5P_DEFAULT) > 0)
            raw.arrays.emplace(name, array(file, name, o, budget, metadata));
    }
    return raw;
}
} // namespace cydsp::sofa_detail
