#include "Fold.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cydsp/CyDSP.h>
#include <iostream>
using namespace cydsp;
volatile float sink = 0;
template <class F> double time(F f) {
    std::array<double, 21> t;
    for (int i = 0; i < 256; ++i)
        f();
    for (auto& x : t) {
        auto a = std::chrono::steady_clock::now();
        for (int i = 0; i < 256; ++i)
            f();
        x = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - a)
                .count() /
            256;
    }
    std::sort(t.begin(), t.end());
    return t[10];
}
int main() {
    if (!detectCpuFeatures().avx2) {
        std::cerr << "AVX2 CPU/OS support required for this isolated experiment\n";
        return 77;
    }
    AudioBuffer<float> input, ref, test;
    (void)input.resize(2, 256);
    (void)ref.resize(2, 256);
    (void)test.resize(2, 256);
    AdaaSaturator<float> p;
    (void)p.prepare({48000, 256, 2});
    SaturationParameters s;
    s.shape = AdaaShape::wavefolder;
    s.drive = 3;
    (void)p.setParameters(s);
    double state[2][2]{};
    double error = 0;
    for (int run = 0; run < 400; ++run) {
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < 256; ++i)
                input.block().channel(c)[i] = float(3 * std::sin((run * 256 + i) * .13 + c));
        (void)p.process(input.block(), ref.block());
        for (int c = 0; c < 2; ++c)
            ispc::fold(input.block().channel(c).data(), test.block().channel(c).data(), 256,
                       state[c], 3, 0);
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < 256; ++i)
                error = std::max(error, std::abs(double(ref.block().channel(c)[i]) -
                                                 test.block().channel(c)[i]));
    }
    auto cpp = [&] {
        (void)p.process(input.block(), ref.block());
        sink = ref.block().channel(0).back();
    };
    auto candidate = [&] {
        if (validateProcessBlocks<float>({48000, 256, 2}, input.block(), test.block()) !=
            Status::ok)
            std::terminate();
        for (int c = 0; c < 2; ++c) {
            for (float x : input.block().channel(c))
                if (!std::isfinite(x))
                    std::terminate();
            ispc::fold(input.block().channel(c).data(), test.block().channel(c).data(), 256,
                       state[c], 3, 0);
        }
        sink = test.block().channel(0).back();
    };
    std::cout << "{\"compiler\":\"ISPC experiment\",\"target\":\"avx2-i32x8\",\"maxError\":"
              << error << ",\"cppUs\":" << time(cpp) << ",\"ispcUs\":" << time(candidate) << "}\n";
    return error > 1e-6 ? 1 : 0;
}
