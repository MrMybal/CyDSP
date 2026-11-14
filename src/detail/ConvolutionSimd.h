#pragma once
// Included only by ISA translation units. All spans are disjoint; first and
// second contain count real values from the two halves of a packed IFFT.
#if defined(_MSC_VER)
#define CYDSP_RESTRICT __restrict
#else
#define CYDSP_RESTRICT __restrict__
#endif
template <class T>
void convolutionEmit(const T* CYDSP_RESTRICT first, const T* CYDSP_RESTRICT second,
                     T* CYDSP_RESTRICT overlap, T* CYDSP_RESTRICT ready, std::size_t count,
                     T scale) noexcept {
    for (std::size_t i = 0; i < count; ++i) {
        ready[i] = first[i] * scale + overlap[i];
        overlap[i] = second[i] * scale;
    }
}
#undef CYDSP_RESTRICT
