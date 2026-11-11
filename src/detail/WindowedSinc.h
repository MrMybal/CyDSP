#pragma once
namespace cydsp::detail {
// Valid prepared parameters: 0<cutoff<=1, halfLength>0; normalized sinc.
double windowedSinc(double distance, double cutoff, double halfLength) noexcept;
} // namespace cydsp::detail
