#include "detail/CpuPolicy.h"
#include "detail/Scalar.h"
#include <array>
#include <cydsp/CyDSP.h>
#include <limits>
#include <type_traits>
using namespace cydsp;
using namespace cydsp::detail;
static_assert(ProcessSpec{}.isValid());
static_assert(!ProcessSpec{0, 512, 2}.isValid());
static_assert(!ProcessSpec{48000, 0, 2}.isValid());
static_assert(!ProcessSpec{48000, 512, 0}.isValid());
static_assert(!ProcessSpec{std::numeric_limits<double>::infinity(), 512, 2}.isValid());
static_assert(!isFinite(std::numeric_limits<float>::quiet_NaN()));
static_assert(!isPowerOfTwo(0) && isPowerOfTwo(1) && isPowerOfTwo(1024) && !isPowerOfTwo(1023));
static_assert(Sample<float> && Sample<const double> && !Sample<int>);
static_assert(!std::is_copy_constructible_v<AudioBuffer<float>>);
static_assert(std::is_nothrow_move_constructible_v<AudioBuffer<float>>);
static_assert(std::is_nothrow_move_constructible_v<Biquad<float>>);
static_assert(std::is_convertible_v<AudioBlock<float>, AudioBlock<const float>>);
static_assert(!std::is_convertible_v<AudioBlock<const float>, AudioBlock<float>>);
static_assert(BiquadCoefficients{}.isStable());
static_assert(!BiquadCoefficients{1, 0, 0, 0, 1}.isStable());
static_assert(!BiquadCoefficients{1, 0, 0, 2, 0}.isStable());
static_assert(!BiquadCoefficients{1, 0, 0, -2, 0}.isStable());
static_assert(!BiquadCoefficients{std::numeric_limits<double>::infinity(), 0, 0, 0, 0}.isStable());
constexpr bool scalarContracts() {
    std::array<float, 7> source{1, 2, 3, 4, 5, 6, 7}, output{};
    scalarCopy(source.data(), output.data(), output.size());
    if (source != output)
        return false;
    scalarScale(output.data(), output.data(), output.size(), 2.0f);
    if (output[0] != 2 || output[6] != 14)
        return false;
    scalarAddScaled(source.data(), output.data(), output.size(), -1.0f);
    if (source != output)
        return false;
    scalarFill(output.data(), output.size(), 0.0f);
    for (auto value : output)
        if (value != 0)
            return false;
    scalarCopy<float>(nullptr, nullptr, 0);
    return true;
}
static_assert(scalarContracts());
constexpr auto xsave = (1u << 26) | (1u << 27) | (1u << 28);
constexpr auto sse2bit = 1u << 26;
constexpr auto avx2bit = 1u << 5;
constexpr X86Snapshot complete{Architecture::x64, 7, xsave, sse2bit, avx2bit, 6};
static_assert(decodeX86(complete).avx2 && decodeX86(complete).sse2);
constexpr bool rejectsMissingOsSupport() {
    auto raw = complete;
    for (unsigned bit : {26u, 27u, 28u}) {
        raw = complete;
        raw.leaf1ecx &= ~(1u << bit);
        if (decodeX86(raw).avx2)
            return false;
    }
    for (std::uint64_t state : {0u, 2u, 4u}) {
        raw = complete;
        raw.xcr0 = state;
        if (decodeX86(raw).avx2)
            return false;
    }
    raw = complete;
    raw.maxLeaf = 6;
    if (decodeX86(raw).avx2)
        return false;
    raw = complete;
    raw.leaf7ebx = 0;
    return !decodeX86(raw).avx2;
}
static_assert(rejectsMissingOsSupport());
constexpr CompiledBackends all{true, true, true}, scalarOnly{};
static_assert(chooseBackend(decodeX86(complete), all) == CpuBackend::avx2);
static_assert(chooseBackend(decodeX86(complete), scalarOnly) == CpuBackend::scalar);
static_assert(chooseBackend({Architecture::x86, true, false, false}, all) == CpuBackend::sse2);
static_assert(chooseBackend({Architecture::arm64, false, false, true}, all) == CpuBackend::neon);
static_assert(!available(CpuBackend::avx2, {Architecture::arm64, true, true, true}, all));
static_assert(!available(CpuBackend::neon, decodeX86(complete), all));
static_assert(!available(CpuBackend::automatic, decodeX86(complete), all));

#include "detail/CheckedSize.h"
#include "detail/FftAlgorithm.h"
#include "detail/RateClock.h"
constexpr bool checkedSizeContracts() {
    std::size_t result = 77;
    if (!checkedMultiply(0, std::numeric_limits<std::size_t>::max(), result) || result != 0)
        return false;
    result = 77;
    if (checkedMultiply(std::numeric_limits<std::size_t>::max(), 2, result) || result != 77)
        return false;
    if (checkedAdd(std::numeric_limits<std::size_t>::max(), 1, result) || result != 77)
        return false;
    return checkedAdd(12, 34, result) && result == 46;
}
static_assert(checkedSizeContracts());
constexpr bool complexMathContracts() {
    float a[]{1, 2, 3, 4, 5, 6}, b[]{2, -1, 0, 1, -2, 3}, d[6]{};
    scalarComplexMac(a, b, d, 3);
    if (d[0] != 4 || d[1] != 3 || d[2] != -4 || d[3] != 3 || d[4] != -28 || d[5] != 3)
        return false;
    float x[]{1, 2, 3}, y[]{4, 5, 6}, out[3]{};
    scalarMultiply(x, y, out, 3);
    return out[0] == 4 && out[1] == 10 && out[2] == 18;
}
static_assert(complexMathContracts());
constexpr bool fftContracts() {
    using C = std::complex<double>;
    std::array<C, 4> input{C{1, 0}, C{2, 0}, C{3, 0}, C{4, 0}}, output{}, restored{};
    const std::array<std::size_t, 4> reverse{0, 2, 1, 3};
    const std::array<C, 3> twiddles{C{1, 0}, C{1, 0}, C{0, -1}};
    const auto kernel = [](auto e, auto o, auto w, bool inverse) constexpr noexcept {
        scalarComplexButterfly<double>(e, o, w, inverse);
    };
    fftTransform<double>(input, output, reverse, twiddles, false, kernel);
    if (output != std::array<C, 4>{C{10, 0}, C{-2, 2}, C{-2, 0}, C{-2, -2}})
        return false;
    fftTransform<double>(output, restored, reverse, twiddles, true, kernel);
    if (restored != input)
        return false;
    fftTransform<double>(input, input, reverse, twiddles, false, kernel);
    return input == output;
}
static_assert(fftContracts());
constexpr bool rateContracts() {
    for (const auto inputRate : {1u, 16000u, 44100u, 48000u, 96000u, 4294967295u})
        for (const auto outputRate : {1u, 16000u, 44100u, 48000u, 96000u, 4294967295u}) {
            RateClock clock;
            std::uint64_t consumed = 1;
            for (std::uint64_t output = 0; output < 128; ++output) {
                if (clock.phase >= outputRate || consumed != output * inputRate / outputRate + 1)
                    return false;
                clock.advance(inputRate, outputRate);
                consumed += clock.pending;
            }
        }
    return true;
}
static_assert(rateContracts());
constexpr bool convolutionIndexContracts() {
    for (std::size_t count = 1; count < 32; ++count)
        for (std::size_t head = 0; head < count; ++head)
            for (std::size_t part = 0; part < count; ++part)
                if (convolutionHistoryIndex(head, part, count) != (head + count - part) % count)
                    return false;
    return true;
}
static_assert(convolutionIndexContracts());
static_assert(ResampleSpec{}.isValid());
static_assert(!ResampleSpec{0, 44100}.isValid());
static_assert(!ResampleSpec{48000, 44100, 2, 512, 1024, 7, 1024}.isValid());
static_assert(!ResampleSpec{48000, 44100, 2, 512, 1024, 64, 31}.isValid());
static_assert(std::is_nothrow_move_constructible_v<PartitionedConvolver<float>>);
static_assert(std::is_nothrow_move_constructible_v<RealFft<double>>);
static_assert(std::is_nothrow_move_constructible_v<Resampler<float>>);

constexpr bool dotContracts() {
    constexpr float a[]{1, 2, 3, 4, 5}, b[]{5, 4, 3, 2, 1};
    return scalarDot(a, b, 5) == 35 && scalarDot<float>(nullptr, nullptr, 0) == 0;
}
static_assert(dotContracts());
static_assert(std::is_nothrow_move_constructible_v<Fir<float>>);

static_assert(std::is_nothrow_move_constructible_v<ComplexFft<float>>);
static_assert(!std::is_copy_constructible_v<ComplexFft<float>>);
constexpr bool subBlockContracts() {
    std::array<float, 8> samples{0, 1, 2, 3, 4, 5, 6, 7};
    float* pointers[]{samples.data()};
    const AudioBlock<float> original{pointers, 1, 8};
    const AudioBlock<const float> view = original.subBlock(2, 5).subBlock(1, 3);
    return view.channels() == 1 && view.frames() == 3 && view.frameOffset() == 3 &&
           view.channel(0)[2] == 5 && view.channelData() == original.channelData() &&
           original.subBlock(9, 0).channels() == 0 && original.subBlock(8, 0).channels() == 1 &&
           original.subBlock(8, 1).channels() == 0;
}
static_assert(subBlockContracts());

#include "detail/RealFftAlgorithm.h"
constexpr bool packedRealContracts() {
    using C = std::complex<double>;
    const std::array<double, 8> input{1, 2, 3, 4, 5, 6, 7, 8};
    std::array<double, 8> restored{};
    std::array<C, 4> packed{};
    std::array<C, 5> spectrum{};
    const std::array<std::size_t, 4> reverse{0, 2, 1, 3};
    constexpr double r = 0.707106781186547524400844362104849;
    const std::array<C, 3> stage{C{1, 0}, C{1, 0}, C{0, -1}}, split{C{r, -r}, C{0, -1}, C{-r, -r}};
    const auto kernel = [](auto e, auto o, auto w, bool inverse) constexpr noexcept {
        scalarComplexButterfly<double>(e, o, w, inverse);
    };
    packReal<double>(input, packed);
    fftTransform<double>(packed, packed, reverse, stage, false, kernel);
    splitRealSpectrum<double>(packed, split, spectrum);
    const std::array<C, 5> expected{C{36, 0}, C{-4, 4 + 8 * r}, C{-4, 4}, C{-4, 8 * r - 4},
                                    C{-4, 0}};
    const auto near = [](double a, double b) constexpr {
        const auto d = a - b;
        return d < 1e-12 && d > -1e-12;
    };
    for (std::size_t k = 0; k < spectrum.size(); ++k)
        if (!near(spectrum[k].real(), expected[k].real()) ||
            !near(spectrum[k].imag(), expected[k].imag()))
            return false;
    mergeRealSpectrum<double>(spectrum, split, packed);
    fftTransform<double>(packed, packed, reverse, stage, true, kernel);
    scatterReal<double>(packed, restored);
    for (std::size_t i = 0; i < input.size(); ++i)
        if (!near(input[i], restored[i]))
            return false;
    // Packed convolution against an independently known causal finite sum.
    std::array<double, 8> a{1, -0.5, 0.25}, b{2, 0.5};
    std::array<C, 5> fa{}, fb{};
    packReal<double>(a, packed);
    fftTransform<double>(packed, packed, reverse, stage, false, kernel);
    splitRealSpectrum<double>(packed, split, fa);
    packReal<double>(b, packed);
    fftTransform<double>(packed, packed, reverse, stage, false, kernel);
    splitRealSpectrum<double>(packed, split, fb);
    for (std::size_t k = 0; k < fa.size(); ++k)
        fa[k] = fa[k] * fb[k];
    mergeRealSpectrum<double>(fa, split, packed);
    fftTransform<double>(packed, packed, reverse, stage, true, kernel);
    scatterReal<double>(packed, restored);
    const std::array<double, 8> convolution{2, -0.5, 0.25, 0.125};
    for (std::size_t i = 0; i < restored.size(); ++i)
        if (!near(restored[i], convolution[i]))
            return false;
    return true;
}
static_assert(packedRealContracts());
constexpr bool packedPairContracts() {
    using C = std::complex<float>;
    const std::array<float, 2> input{3, 4};
    std::array<float, 2> restored{};
    std::array<C, 1> packed{};
    std::array<C, 2> spectrum{};
    const std::array<C, 0> twiddles{};
    const std::array<std::size_t, 1> reverse{0};
    const auto kernel = [](auto e, auto o, auto w, bool inverse) constexpr noexcept {
        scalarComplexButterfly<float>(e, o, w, inverse);
    };
    packReal<float>(input, packed);
    fftTransform<float>(packed, packed, reverse, twiddles, false, kernel);
    splitRealSpectrum<float>(packed, twiddles, spectrum);
    if (spectrum[0] != C{7, 0} || spectrum[1] != C{-1, 0})
        return false;
    mergeRealSpectrum<float>(spectrum, twiddles, packed);
    fftTransform<float>(packed, packed, reverse, twiddles, true, kernel);
    scatterReal<float>(packed, restored);
    return input == restored;
}
static_assert(packedPairContracts());

constexpr bool mixedDotContracts() {
    const float a[]{1.5f, -2, 0.25f, 4, 8};
    const double b[]{2, 0.125, 8, -0.5, 0.25};
    return scalarDotMixed(a, b, 5) == 4.75 && scalarDotMixed(nullptr, nullptr, 0) == 0;
}
static_assert(mixedDotContracts());

static_assert(std::is_nothrow_move_constructible_v<HybridConvolver<float>>);
static_assert(!std::is_copy_constructible_v<HybridConvolver<double>>);

#include "detail/LinearRamp.h"
constexpr bool rampContracts() {
    double current = 0;
    std::size_t remaining = 8;
    for (std::size_t i = 0; i < 8; ++i) {
        current = advanceLinearRamp(current, 4, remaining);
        if (current != 0.5 * static_cast<double>(i + 1))
            return false;
    }
    return remaining == 0 && advanceLinearRamp(current, 4, remaining) == 4;
}
static_assert(rampContracts());

#include "detail/AudioLayout.h"
constexpr bool layoutContracts() {
    std::array<float, 4> a{1, 2, 3, 4}, b{5, 6, 7, 8}, oa{}, ob{};
    float* ip[]{a.data(), b.data()};
    float* op[]{oa.data(), ob.data()};
    std::array<float, 4> packed{};
    const AudioBlock<float> in{ip, 2, 4}, out{op, 2, 4};
    interleaveUnchecked<float>(in.subBlock(1, 2), packed);
    if (packed != std::array<float, 4>{2, 6, 3, 7})
        return false;
    deinterleaveUnchecked<float>(packed, out.subBlock(1, 2));
    return oa == std::array<float, 4>{0, 2, 3, 0} && ob == std::array<float, 4>{0, 6, 7, 0};
}
static_assert(layoutContracts());
constexpr bool biquadBankContracts() {
    std::array<double, 18> data{1, 2, 1, 0, 0, 2, 0, 0, 3, 0, 0, 4, 0, 0, 5, 0, 0, 6};
    auto split = data;
    std::array<double, 6> states{}, splitStates{};
    const std::array<double, 15> coefficients{0.5, 1,    0, 0.25, 0,    1, 0.125, 0,
                                              0,   -0.5, 0, 0,    0.25, 0, 0};
    scalarBiquadBank(data.data(), 3, 6, coefficients.data(), states.data());
    scalarBiquadBank(split.data(), 3, 3, coefficients.data(), splitStates.data());
    scalarBiquadBank(split.data() + 9, 3, 3, coefficients.data(), splitStates.data());
    const std::array<double, 6> impulse{0.5, 0.5, 0.25, 0, -0.0625, -0.03125};
    for (std::size_t frame = 0; frame < 6; ++frame) {
        if (data[3 * frame] != impulse[frame] || data[3 * frame + 1] != (frame == 0 ? 2 : 0) ||
            data[3 * frame + 2] != static_cast<double>(frame))
            return false;
    }
    return data == split && states == splitStates;
}
static_assert(biquadBankContracts());
constexpr bool coefficientRoundingContracts() {
    BiquadCoefficients result;
    if (roundBiquad<float>({std::numeric_limits<double>::max(), 0, 0, 0, 0}, result) ||
        result.b0 != 1)
        return false;
    if (roundBiquad<float>({1, 0, 0, 0, 0.999999999999}, result) || result.a2 != 0)
        return false;
    return roundBiquad<float>({0.5, 0.25, 0.125, -0.5, 0.25}, result) && result.b0 == 0.5;
}
static_assert(coefficientRoundingContracts());
static_assert(std::is_nothrow_move_constructible_v<BiquadBank<float>>);
static_assert(!std::is_copy_constructible_v<BiquadBank<double>>);

#include "detail/Crossfade.h"
static_assert(linearCrossfade(0.0, 8.0, 0.25) == 2 && linearCrossfade(2.0, -2.0, 0.25) == 1);
static_assert(linearCrossfade(1.0, std::numeric_limits<double>::quiet_NaN(), 0) == 1);
static_assert(linearCrossfade(std::numeric_limits<double>::quiet_NaN(), 2.0, 1) == 2);
static_assert(std::is_nothrow_move_constructible_v<SmoothedBiquad<float>>);
static_assert(!std::is_copy_constructible_v<SmoothedBiquad<double>>);

#include "detail/OrderedAlias.h"
constexpr bool orderedAliasContracts() {
    for (std::size_t bytes = 1; bytes <= 3; ++bytes)
        for (std::uintptr_t a = 0; a < 4; ++a)
            for (std::uintptr_t b = 0; b < 4; ++b)
                for (std::uintptr_t c = 0; c < 4; ++c)
                    for (std::uintptr_t d = 0; d < 4; ++d) {
                        const std::array<std::uintptr_t, 2> input{a, b}, output{c, d};
                        auto expected = Status::ok;
                        for (std::size_t out = 0; out < 2; ++out) {
                            if (out && equalLengthRangesOverlap(output[out], output[0], bytes))
                                expected = Status::partial_overlap;
                            for (std::size_t in = 0; in < 2; ++in) {
                                if (in == out && input[in] == output[out])
                                    continue;
                                // Independent interval inequalities; small synthetic addresses
                                // cannot overflow.
                                if (input[in] < output[out] + bytes &&
                                    output[out] < input[in] + bytes)
                                    expected = Status::partial_overlap;
                            }
                        }
                        const auto result = validateOrderedAlias(
                            2, bytes, [&](auto ch) { return input[ch]; },
                            [&](auto ch) { return output[ch]; });
                        if (result.handled && result.status != expected)
                            return false;
                    }
    const std::array<std::uintptr_t, 3> a{100, 200, 300}, b{104, 204, 304};
    const auto partial =
        validateOrderedAlias(3, 8, [&](auto ch) { return a[ch]; }, [&](auto ch) { return b[ch]; });
    const auto inplace =
        validateOrderedAlias(3, 8, [&](auto ch) { return a[ch]; }, [&](auto ch) { return a[ch]; });
    return partial.handled && partial.status == Status::partial_overlap && inplace.handled &&
           inplace.status == Status::ok;
}
static_assert(orderedAliasContracts());

#include "detail/MatrixAlgorithm.h"
constexpr bool matrixRampContracts() {
    std::array<double, 4> current{1, 0, 0, 1};
    const std::array<double, 4> target{0, 1, 1, 0};
    std::array<double, 4> a{2, 2, 2, 2}, b{6, 6, 6, 6}, oa{}, ob{};
    double* ip[]{a.data(), b.data()};
    double* op[]{oa.data(), ob.data()};
    std::size_t remaining = 4;
    for (std::size_t frame = 0; frame < 4; ++frame) {
        advanceMatrixRamp(current, target, remaining);
        matrixFrame<double>({ip, 2, 4}, {op, 2, 4}, current, frame);
    }
    return remaining == 0 && current == target && oa == std::array<double, 4>{3, 4, 5, 6} &&
           ob == std::array<double, 4>{5, 4, 3, 2};
}
static_assert(matrixRampContracts());
static_assert(validMatrixCoefficient<float>(1) &&
              !validMatrixCoefficient<float>(std::numeric_limits<double>::max()) &&
              !validMatrixCoefficient<double>(std::numeric_limits<double>::infinity()));
constexpr bool matrixMuteContracts() {
    std::array<double, 1> a{std::numeric_limits<double>::quiet_NaN()}, b{2}, out{};
    double* ip[]{a.data(), b.data()};
    double* op[]{out.data()};
    constexpr std::array<double, 2> coefficients{0, 0.5};
    matrixFrame<double>({ip, 2, 1}, {op, 1, 1}, coefficients, 0);
    return out[0] == 1;
}
static_assert(matrixMuteContracts());
constexpr bool matrixAliasContracts() {
    // Exhaustive synthetic addresses, rectangular shape, independent interval oracle.
    for (std::size_t bytes = 1; bytes <= 3; ++bytes)
        for (std::uintptr_t a = 0; a < 4; ++a)
            for (std::uintptr_t b = 0; b < 4; ++b)
                for (std::uintptr_t c = 0; c < 4; ++c)
                    for (std::uintptr_t d = 0; d < 4; ++d) {
                        const std::array<std::uintptr_t, 3> input{a, b, a};
                        const std::array<std::uintptr_t, 2> output{c, d};
                        auto expected = Status::ok;
                        if (c < d + bytes && d < c + bytes)
                            expected = Status::partial_overlap;
                        for (const auto x : input)
                            for (const auto y : output)
                                if (x != y && x < y + bytes && y < x + bytes)
                                    expected = Status::partial_overlap;
                        if (validateMatrixAlias(
                                3, 2, bytes, [&](auto ch) { return input[ch]; },
                                [&](auto ch) { return output[ch]; }) != expected)
                            return false;
                    }
    return validateMatrixAlias(
               3, 2, 0, [](auto) { return 0u; }, [](auto) { return 0u; }) == Status::ok;
}
static_assert(matrixAliasContracts());
constexpr bool blockFadeContracts() {
    std::array<double, 4> a{0, 0, 0, 0}, b{8, 8, 8, 8}, oa{}, ob{};
    double* ip[]{a.data(), b.data()};
    double* tp[]{b.data(), a.data()};
    double* op[]{oa.data(), ob.data()};
    double blend = 0;
    std::size_t remaining = 4;
    const AudioBlock<const double> from{ip, 2, 4}, to{tp, 2, 4};
    AudioBlock<double> out{op, 2, 4};
    crossfadeBlocks<double>(from.subBlock(0, 1), to.subBlock(0, 1), out.subBlock(0, 1), blend,
                            remaining);
    if (remaining != 3 || blend != 0.25)
        return false;
    crossfadeBlocks<double>(from.subBlock(1, 3), to.subBlock(1, 3), out.subBlock(1, 3), blend,
                            remaining);
    return !remaining && blend == 1 && oa == std::array<double, 4>{2, 4, 6, 8} &&
           ob == std::array<double, 4>{6, 4, 2, 0};
}
static_assert(blockFadeContracts());
static_assert(std::is_nothrow_move_constructible_v<ChannelMatrix<float>>);
static_assert(!std::is_copy_constructible_v<ChannelMatrix<double>>);
static_assert(std::is_nothrow_move_constructible_v<SmoothedConvolver<float>>);
static_assert(!std::is_copy_constructible_v<SmoothedConvolver<double>>);

static_assert(resampleSpecForQuality(ResampleSpec{}, ResampleQuality::high).taps == 256);
static_assert(resampleSpecForQuality(ResampleSpec{48000, 8000}, ResampleQuality::high).taps == 512);
static_assert(!resampleSpecForQuality(ResampleSpec{}, static_cast<ResampleQuality>(99)).isValid());
static_assert(!std::is_move_constructible_v<AsyncConvolver<float>>);
static_assert(std::is_nothrow_move_constructible_v<AudioGraph<float>>);
static_assert(std::is_nothrow_move_constructible_v<MatrixConvolver<double>>);
static_assert(std::is_nothrow_move_constructible_v<NonuniformConvolver<float>>);
static_assert(std::is_nothrow_move_constructible_v<SmoothedBiquadCascade<double>>);

constexpr bool wideCpuGuards() {
    auto raw = complete;
    raw.leaf7ebx |= (1u << 16) | (1u << 17) | (1u << 28) | (1u << 30) | (1u << 31);
    raw.xcr0 = 0xe6;
    if (!decodeX86(raw).avx512)
        return false;
    for (unsigned bit : {1u, 2u, 5u, 6u, 7u}) {
        auto missing = raw;
        missing.xcr0 &= ~(std::uint64_t{1} << bit);
        if (decodeX86(missing).avx512)
            return false;
    }
    for (unsigned bit : {5u, 16u, 17u, 28u, 30u, 31u}) {
        auto missing = raw;
        missing.leaf7ebx &= ~(1u << bit);
        if (decodeX86(missing).avx512)
            return false;
    }
    return chooseBackend(decodeX86(raw), {true, true, true, true, true}) == CpuBackend::avx512 &&
           chooseBackend({Architecture::arm64, false, false, true, false, true},
                         {true, true, true, true, true}) == CpuBackend::sve &&
           !available(CpuBackend::sve, decodeX86(raw), {true, true, true, true, true});
}
static_assert(wideCpuGuards());

#include "detail/PartitionSchedule.h"
constexpr bool partitionScheduleContracts() {
    for (std::size_t partition : {1u, 2u, 16u, 1024u})
        for (std::size_t jobs : {0u, 1u, 3u, 31u, 2049u}) {
            std::size_t previous = 0, debt = 0, scheduled = 0;
            for (std::size_t f = 0; f <= partition; ++f) {
                if (f)
                    scheduled +=
                        advancePartitionJobs(jobs / partition, jobs % partition, partition, debt);
                auto due = duePartitionJobs(jobs, f, partition);
                if (due < previous || due > jobs || scheduled != due)
                    return false;
                previous = due;
            }
            if (previous != jobs)
                return false;
        }
    constexpr auto large = std::numeric_limits<std::size_t>::max() / 8;
    return duePartitionJobs(large, 1024, 1024) == large;
}
static_assert(partitionScheduleContracts());
static_assert(ResampleSpec{48000, 48000, 1, 64, 80, 128, 1024, 0.75, 3}.isValid());
static_assert(!ResampleSpec{48000, 48000, 1, 64, 80, 128, 1024, 1.5, 3}.isValid());
static_assert(!ResampleSpec{48000, 48000, 1, 64, 80, 128, 1024, 0.75, 0}.isValid());

static_assert(resampleSpecForQuality({48000, 48000, 1, 64, 80, 128, 1024, 0.75, 3},
                                     ResampleQuality::economy)
                  .taps == 192);

constexpr bool bitReverseRecurrence() {
    for (std::size_t n : {1u, 2u, 4u, 8u, 16u, 32u}) {
        std::array<std::size_t, 32> values{};
        fillBitReverse(std::span{values}.first(n));
        for (std::size_t i = 0; i < n; ++i) {
            std::size_t expected = 0, value = i;
            for (auto bits = n; bits > 1; bits /= 2) {
                expected = (expected << 1) | (value & 1);
                value >>= 1;
            }
            if (values[i] != expected)
                return false;
        }
    }
    return true;
}
static_assert(bitReverseRecurrence());

static_assert(!std::is_copy_constructible_v<ScheduledConvolver<float>>);
static_assert(std::is_nothrow_move_constructible_v<ScheduledConvolver<double>>);
static_assert(std::is_nothrow_destructible_v<ScheduledConvolver<double>>);

static_assert(std::is_nothrow_move_constructible_v<Convolver<float>>);
static_assert(std::is_nothrow_move_assignable_v<Convolver<double>>);
static_assert(!std::is_copy_constructible_v<Convolver<double>>);
static_assert(ConvolutionConfig{}.mode == ConvolutionMode::hybrid);

static_assert(std::is_nothrow_destructible_v<MatrixConvolver<float>>);
static_assert(std::is_nothrow_move_assignable_v<MatrixConvolver<double>>);
