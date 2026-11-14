#pragma once
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <numbers>
namespace cydsp::detail {
// Original range-reduced double arithmetic for audio transfer curves. These are
// private numerical kernels, not a change to the application's FP environment.
// Tables are rounded binary64 values of reciprocal/log bin centres and 2^(i/64).
struct LogBin {
    double reciprocal, logarithm;
};
inline constexpr std::array<LogBin, 128> audioLogBins{
    {{0x1.fe01fe01fe020p-1, 0x1.ff00aa2b10bc0p-9}, {0x1.fa11caa01fa12p-1, 0x1.7dc475f810a77p-7},
     {0x1.f6310aca0dbb5p-1, 0x1.3cea44346a575p-6}, {0x1.f25f644230ab5p-1, 0x1.b9fc027af9198p-6},
     {0x1.ee9c7f8458e02p-1, 0x1.1b0d98923d980p-5}, {0x1.eae807aba01ebp-1, 0x1.58a5bafc8e4d5p-5},
     {0x1.e741aa59750e4p-1, 0x1.95c830ec8e3ebp-5}, {0x1.e3a9179dc1a73p-1, 0x1.d276b8adb0b52p-5},
     {0x1.e01e01e01e01ep-1, 0x1.075983598e471p-4}, {0x1.dca01dca01dcap-1, 0x1.253f62f0a1417p-4},
     {0x1.d92f2231e7f8ap-1, 0x1.42edcbea646f0p-4}, {0x1.d5cac807572b2p-1, 0x1.60658a93750c4p-4},
     {0x1.d272ca3fc5b1ap-1, 0x1.7da766d7b12cdp-4}, {0x1.cf26e5c44bfc6p-1, 0x1.9ab42462033adp-4},
     {0x1.cbe6d9601cbe7p-1, 0x1.b78c82bb0eda1p-4}, {0x1.c8b265afb8a42p-1, 0x1.d4313d66cb35dp-4},
     {0x1.c5894d10d4986p-1, 0x1.f0a30c01162a6p-4}, {0x1.c26b5392ea01cp-1, 0x1.0671512ca596ep-3},
     {0x1.bf583ee868d8bp-1, 0x1.14785846742acp-3}, {0x1.bc4fd65883e7bp-1, 0x1.2266f190a5acbp-3},
     {0x1.b951e2b18ff23p-1, 0x1.303d718e47fd3p-3}, {0x1.b65e2e3beee05p-1, 0x1.3dfc2b0ecc62ap-3},
     {0x1.b37484ad806cep-1, 0x1.4ba36f39a55e5p-3}, {0x1.b094b31d922a4p-1, 0x1.59338d9982086p-3},
     {0x1.adbe87f94905ep-1, 0x1.66acd4272ad51p-3}, {0x1.aaf1d2f87ebfdp-1, 0x1.740f8f54037a5p-3},
     {0x1.a82e65130e159p-1, 0x1.815c0a14357ebp-3}, {0x1.a574107688a4ap-1, 0x1.8e928de886d41p-3},
     {0x1.a2c2a87c51ca0p-1, 0x1.9bb362e7dfb83p-3}, {0x1.a01a01a01a01ap-1, 0x1.a8becfc882f19p-3},
     {0x1.9d79f176b682dp-1, 0x1.b5b519e8fb5a4p-3}, {0x1.9ae24ea5510dap-1, 0x1.c2968558c18c1p-3},
     {0x1.9852f0d8ec0ffp-1, 0x1.cf6354e09c5dcp-3}, {0x1.95cbb0be377aep-1, 0x1.dc1bca0abec7dp-3},
     {0x1.934c67f9b2ce6p-1, 0x1.e8c0252aa5a60p-3}, {0x1.90d4f120190d5p-1, 0x1.f550a564b7b37p-3},
     {0x1.8e6527af1373fp-1, 0x1.00e6c45ad501dp-2}, {0x1.8bfce8062ff3ap-1, 0x1.071b85fcd590dp-2},
     {0x1.899c0f601899cp-1, 0x1.0d46b579ab74bp-2}, {0x1.87427bcc092b9p-1, 0x1.136870293a8b0p-2},
     {0x1.84f00c2780614p-1, 0x1.1980d2dd4236fp-2}, {0x1.82a4a0182a4a0p-1, 0x1.1f8ff9e48a2f3p-2},
     {0x1.8060180601806p-1, 0x1.2596010df763ap-2}, {0x1.7e225515a4f1dp-1, 0x1.2b9303ab89d25p-2},
     {0x1.7beb3922e017cp-1, 0x1.31871c9544185p-2}, {0x1.79baa6bb6398bp-1, 0x1.3772662bfd85bp-2},
     {0x1.77908119ac60dp-1, 0x1.3d54fa5c1f710p-2}, {0x1.756cac201756dp-1, 0x1.432ef2a04e814p-2},
     {0x1.734f0c541fe8dp-1, 0x1.49006804009d1p-2}, {0x1.713786d9c7c09p-1, 0x1.4ec973260026ap-2},
     {0x1.6f26016f26017p-1, 0x1.548a2c3add263p-2}, {0x1.6d1a62681c861p-1, 0x1.5a42ab0f4cfe2p-2},
     {0x1.6b1490aa31a3dp-1, 0x1.5ff3070a793d4p-2}, {0x1.691473a88d0c0p-1, 0x1.659b57303e1f3p-2},
     {0x1.6719f3601671ap-1, 0x1.6b3bb2235943ep-2}, {0x1.6524f853b4aa3p-1, 0x1.70d42e2789236p-2},
     {0x1.63356b88ac0dep-1, 0x1.7664e1239dbcfp-2}, {0x1.614b36831ae94p-1, 0x1.7bede0a37afc0p-2},
     {0x1.5f66434292dfcp-1, 0x1.816f41da0d496p-2}, {0x1.5d867c3ece2a5p-1, 0x1.86e919a330ba0p-2},
     {0x1.5babcc647fa91p-1, 0x1.8c5b7c858b48bp-2}, {0x1.59d61f123ccaap-1, 0x1.91c67eb45a83ep-2},
     {0x1.5805601580560p-1, 0x1.972a341135158p-2}, {0x1.56397ba7c52e2p-1, 0x1.9c86b02dc0863p-2},
     {0x1.54725e6bb82fep-1, 0x1.a1dc064d5b995p-2}, {0x1.52aff56a8054bp-1, 0x1.a72a4966bd9eap-2},
     {0x1.50f22e111c4c5p-1, 0x1.ac718c258b0e4p-2}, {0x1.4f38f62dd4c9bp-1, 0x1.b1b1e0ebdfc5bp-2},
     {0x1.4d843bedc2c4cp-1, 0x1.b6eb59d3cf35ep-2}, {0x1.4bd3edda68fe1p-1, 0x1.bc1e08b0dad0ap-2},
     {0x1.4a27fad76014ap-1, 0x1.c149ff115f027p-2}, {0x1.4880522014880p-1, 0x1.c66f4e3ff6ff8p-2},
     {0x1.46dce34596066p-1, 0x1.cb8e0744d7acap-2}, {0x1.453d9e2c776cap-1, 0x1.d0a63ae721e64p-2},
     {0x1.43a2730abee4dp-1, 0x1.d5b7f9ae2c684p-2}, {0x1.420b5265e5951p-1, 0x1.dac353e2c5954p-2},
     {0x1.40782d10e6566p-1, 0x1.dfc859906d5b5p-2}, {0x1.3ee8f42a5af07p-1, 0x1.e4c71a8687704p-2},
     {0x1.3d5d991aa75c6p-1, 0x1.e9bfa659861f5p-2}, {0x1.3bd60d9232955p-1, 0x1.eeb20c640ddf4p-2},
     {0x1.3a524387ac822p-1, 0x1.f39e5bc811e5cp-2}, {0x1.38d22d366088ep-1, 0x1.f884a36fe9ec2p-2},
     {0x1.3755bd1c945eep-1, 0x1.fd64f20f61572p-2}, {0x1.35dce5f9f2af8p-1, 0x1.011fab125ff8ap-1},
     {0x1.34679ace01346p-1, 0x1.0389eefce633bp-1}, {0x1.32f5ced6a1dfap-1, 0x1.05f14bd26459cp-1},
     {0x1.3187758e9ebb6p-1, 0x1.0855c884b450ep-1}, {0x1.301c82ac40260p-1, 0x1.0ab76bece14d2p-1},
     {0x1.2eb4ea1fed14bp-1, 0x1.0d163ccb9d6b8p-1}, {0x1.2d50a012d50a0p-1, 0x1.0f7241c9b497dp-1},
     {0x1.2bef98e5a3711p-1, 0x1.11cb81787ccf8p-1}, {0x1.2a91c92f3c105p-1, 0x1.1422025243d45p-1},
     {0x1.293725bb804a5p-1, 0x1.1675cababa60ep-1}, {0x1.27dfa38a1ce4dp-1, 0x1.18c6e0ff5cf06p-1},
     {0x1.268b37cd60127p-1, 0x1.1b154b57da29fp-1}, {0x1.2539d7e9177b2p-1, 0x1.1d610fe677003p-1},
     {0x1.23eb79717605bp-1, 0x1.1faa34b87094cp-1}, {0x1.22a0122a0122ap-1, 0x1.21f0bfc65beecp-1},
     {0x1.21579804855e6p-1, 0x1.2434b6f483934p-1}, {0x1.2012012012012p-1, 0x1.26762013430e0p-1},
     {0x1.1ecf43c7fb84cp-1, 0x1.28b500df60783p-1}, {0x1.1d8f5672e4abdp-1, 0x1.2af15f02640adp-1},
     {0x1.1c522fc1ce059p-1, 0x1.2d2b4012edc9ep-1}, {0x1.1b17c67f2bae3p-1, 0x1.2f62a99509546p-1},
     {0x1.19e0119e0119ep-1, 0x1.3197a0fa7fe6ap-1}, {0x1.18ab083902bdbp-1, 0x1.33ca2ba328995p-1},
     {0x1.1778a191bd684p-1, 0x1.35fa4edd36ea0p-1}, {0x1.1648d50fc3201p-1, 0x1.38280fe58797fp-1},
     {0x1.151b9a3fdd5c9p-1, 0x1.3a5373e7ebdfap-1}, {0x1.13f0e8d344724p-1, 0x1.3c7c7fff73206p-1},
     {0x1.12c8b89edc0acp-1, 0x1.3ea33936b2f5cp-1}, {0x1.11a3019a74826p-1, 0x1.40c7a4880dce9p-1},
     {0x1.107fbbe011080p-1, 0x1.42e9c6ddf80bfp-1}, {0x1.0f5edfab325a2p-1, 0x1.4509a5133bb0ap-1},
     {0x1.0e40655826011p-1, 0x1.472743f33aaadp-1}, {0x1.0d24456359e3ap-1, 0x1.4942a83a2fc07p-1},
     {0x1.0c0a7868b4171p-1, 0x1.4b5bd6956e274p-1}, {0x1.0af2f722eecb5p-1, 0x1.4d72d3a39fd00p-1},
     {0x1.09ddba6af8360p-1, 0x1.4f87a3f5026e9p-1}, {0x1.08cabb37565e2p-1, 0x1.519a4c0ba3446p-1},
     {0x1.07b9f29b8eae2p-1, 0x1.53aad05b99b7dp-1}, {0x1.06ab59c7912fbp-1, 0x1.55b9354b40bcdp-1},
     {0x1.059eea0727586p-1, 0x1.57c57f336f191p-1}, {0x1.04949cc1664c5p-1, 0x1.59cfb25fae87ep-1},
     {0x1.038c6b78247fcp-1, 0x1.5bd7d30e71c73p-1}, {0x1.02864fc7729e9p-1, 0x1.5ddde57149923p-1},
     {0x1.0182436517a37p-1, 0x1.5fe1edad18919p-1}, {0x1.0080402010080p-1, 0x1.61e3efda46467p-1}}};
inline constexpr std::array<double, 64> audioExpBins{
    {0x1.0000000000000p+0, 0x1.02c9a3e778061p+0, 0x1.059b0d3158574p+0, 0x1.0874518759bc8p+0,
     0x1.0b5586cf9890fp+0, 0x1.0e3ec32d3d1a2p+0, 0x1.11301d0125b51p+0, 0x1.1429aaea92de0p+0,
     0x1.172b83c7d517bp+0, 0x1.1a35beb6fcb75p+0, 0x1.1d4873168b9aap+0, 0x1.2063b88628cd6p+0,
     0x1.2387a6e756238p+0, 0x1.26b4565e27cddp+0, 0x1.29e9df51fdee1p+0, 0x1.2d285a6e4030bp+0,
     0x1.306fe0a31b715p+0, 0x1.33c08b26416ffp+0, 0x1.371a7373aa9cbp+0, 0x1.3a7db34e59ff7p+0,
     0x1.3dea64c123422p+0, 0x1.4160a21f72e2ap+0, 0x1.44e086061892dp+0, 0x1.486a2b5c13cd0p+0,
     0x1.4bfdad5362a27p+0, 0x1.4f9b2769d2ca7p+0, 0x1.5342b569d4f82p+0, 0x1.56f4736b527dap+0,
     0x1.5ab07dd485429p+0, 0x1.5e76f15ad2148p+0, 0x1.6247eb03a5585p+0, 0x1.6623882552225p+0,
     0x1.6a09e667f3bcdp+0, 0x1.6dfb23c651a2fp+0, 0x1.71f75e8ec5f74p+0, 0x1.75feb564267c9p+0,
     0x1.7a11473eb0187p+0, 0x1.7e2f336cf4e62p+0, 0x1.82589994cce13p+0, 0x1.868d99b4492edp+0,
     0x1.8ace5422aa0dbp+0, 0x1.8f1ae99157736p+0, 0x1.93737b0cdc5e5p+0, 0x1.97d829fde4e50p+0,
     0x1.9c49182a3f090p+0, 0x1.a0c667b5de565p+0, 0x1.a5503b23e255dp+0, 0x1.a9e6b5579fdbfp+0,
     0x1.ae89f995ad3adp+0, 0x1.b33a2b84f15fbp+0, 0x1.b7f76f2fb5e47p+0, 0x1.bcc1e904bc1d2p+0,
     0x1.c199bdd85529cp+0, 0x1.c67f12e57d14bp+0, 0x1.cb720dcef9069p+0, 0x1.d072d4a07897cp+0,
     0x1.d5818dcfba487p+0, 0x1.da9e603db3285p+0, 0x1.dfc97337b9b5fp+0, 0x1.e502ee78b3ff6p+0,
     0x1.ea4afa2a490dap+0, 0x1.efa1bee615a27p+0, 0x1.f50765b6e4540p+0, 0x1.fa7c1819e90d8p+0}};
inline double audioLog(double x) noexcept {
    // Preserve relative accuracy near unity, including log(1)==0 exactly.
    // This matters when a very small material loss is multiplied by thickness.
    if (x >= .9375 && x <= 1.0625) {
        const double z = (x - 1) / (x + 1), square = z * z;
        return 2 * z *
               (1 +
                square *
                    (1. / 3 +
                     square * (1. / 5 +
                               square * (1. / 7 +
                                         square * (1. / 9 + square * (1. / 11 + square / 13))))));
    }
    const auto bits = std::bit_cast<std::uint64_t>(x);
    const auto exponent = int((bits >> 52) & 2047) - 1023;
    if (exponent == -1023 || exponent == 1024 || (bits >> 63))
        return std::log(x); // Preserve subnormal, zero, infinity and domain behavior.
    const auto mantissa =
        std::bit_cast<double>((bits & 0x000fffffffffffffull) | 0x3ff0000000000000ull);
    const auto index = (bits >> 45) & 127;
    const auto& bin = audioLogBins[index];
    const double centre = 1 + (double(index) + .5) / 128;
    const double r = (mantissa - centre) * bin.reciprocal;
    // |r| <= 1/256. Taylor remainder after degree 7 is below 7e-21.
    const double correction =
        r * (1 + r * (-.5 + r * (1. / 3 + r * (-.25 + r * (.2 + r * (-1. / 6 + r / 7))))));
    return exponent * std::numbers::ln2 +
           (bin.logarithm + correction + exponent * 2.3190468138462996e-17);
}
inline double audioExp(double x) noexcept {
    if (!(x >= -700 && x <= 700))
        return std::exp(x); // Includes underflow/overflow edges and NaN.
    const auto n = std::int64_t(x * (64 / std::numbers::ln2) + (x < 0 ? -.5 : .5));
    const auto e = n >> 6;
    const auto index = std::size_t(n - e * 64);
    constexpr double high = 0x1.62e42fefa0000p-7;
    constexpr double low = 0x1.cf79abc9e3b3ap-46;
    const double r = (x - double(n) * high) - double(n) * low;
    // |r| <= ln(2)/128; degree 6 has a remainder below 3e-20.
    const double polynomial =
        1 + r * (1 + r * (.5 + r * (1. / 6 + r * (1. / 24 + r * (1. / 120 + r / 720)))));
    const auto bits = std::bit_cast<std::uint64_t>(audioExpBins[index]);
    const auto scale =
        std::bit_cast<double>(std::uint64_t(std::int64_t(bits) + e * 0x10000000000000ll));
    return scale * polynomial;
}
} // namespace cydsp::detail
