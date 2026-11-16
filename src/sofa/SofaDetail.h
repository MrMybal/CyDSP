#pragma once
#include "cydsp/spatial/Sofa.h"
#include <array>
#include <map>
#include <stdexcept>
namespace cydsp::sofa_detail {
struct Failure {
    SofaError error;
};
[[noreturn]] inline void fail(SofaError e) {
    throw Failure{e};
}
inline std::size_t product(std::size_t a, std::size_t b, std::size_t limit) {
    if (a && b > limit / a)
        fail(SofaError::limit_exceeded);
    return a * b;
}
struct Array {
    std::vector<double> values;
    std::vector<std::size_t> shape;
    std::string type, units;
    unsigned precision = 0;
};
struct Raw {
    std::map<std::string, Array> arrays;
    std::map<std::string, std::string> attributes;
    bool reduced = false;
};
struct Budget {
    std::size_t remaining;
    void consume(std::size_t n) {
        if (n > remaining)
            fail(SofaError::limit_exceeded);
        remaining -= n;
    }
};
Raw readCompact(std::span<const std::byte>, const SofaLoadOptions&);
Raw readComplete(std::span<const std::byte>, const SofaLoadOptions&);
inline Status status(SofaError e) noexcept {
    if (e == SofaError::allocation_failed)
        return Status::allocation_failed;
    if (e == SofaError::unsupported_convention || e == SofaError::unsupported_storage ||
        e == SofaError::backend_unavailable || e == SofaError::precision_loss)
        return Status::unsupported_format;
    return Status::invalid_argument;
}
} // namespace cydsp::sofa_detail
