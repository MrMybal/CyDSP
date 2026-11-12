#pragma once
#include <cstdlib>
#include <filesystem>
#include <memory>
namespace cydsp::testSupport {
inline std::filesystem::path temporaryDirectory() {
#if defined(_MSC_VER)
    char* configured = nullptr;
    std::size_t length = 0;
    if (_dupenv_s(&configured, &length, "CYDSP_TEST_TMP") == 0 && configured) {
        std::unique_ptr<char, decltype(&std::free)> memory(configured, &std::free);
        if (length > 1)
            return std::filesystem::path(configured);
    }
#else
    if (const auto* configured = std::getenv("CYDSP_TEST_TMP"))
        return std::filesystem::path(configured);
#endif
    return std::filesystem::temp_directory_path();
}
} // namespace cydsp::testSupport
