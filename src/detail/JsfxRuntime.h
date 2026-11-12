#pragma once
#include "ysfx.h"
namespace cydsp::detail {
bool jsfxMemoryWithinBudget(ysfx_t*) noexcept;
bool jsfxPrepareMemory(ysfx_t*) noexcept;
} // namespace cydsp::detail
