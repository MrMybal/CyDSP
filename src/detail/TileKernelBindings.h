// Included inside each ISA translation unit after selecting SaturationVector.
void adaaTanhSimd(const double* x, double* y, std::size_t n, double previous, bool active,
                  double dc, double gain) noexcept {
    simdAdaaTanh<SaturationVector>(x, y, n, previous, active, dc, gain);
}
void adaaAtanSimd(const double* x, double* y, std::size_t n, double previous, bool active,
                  double dc, double gain) noexcept {
    simdAdaaAtan<SaturationVector>(x, y, n, previous, active, dc, gain);
}
void shapeTanhSimd(const double* x, double* y, std::size_t n, double dc, double gain) noexcept {
    simdShapeTanh<SaturationVector>(x, y, n, dc, gain);
}
void shapeAtanSimd(const double* x, double* y, std::size_t n, double dc, double gain) noexcept {
    simdShapeAtan<SaturationVector>(x, y, n, dc, gain);
}
void saturationDriveF32(const float* in, double* out, std::size_t n, const double* stage,
                        bool staged) noexcept {
    simdSaturationDrive<SaturationVector>(in, out, n, stage, staged);
}
void saturationDriveF64(const double* in, double* out, std::size_t n, const double* stage,
                        bool staged) noexcept {
    simdSaturationDrive<SaturationVector>(in, out, n, stage, staged);
}
void saturationPublishF32(const float* dry, const double* wet, float* out, std::size_t n,
                          double dryGain, double wetGain, bool mixed) noexcept {
    simdSaturationPublish<SaturationVector>(dry, wet, out, n, dryGain, wetGain, mixed);
}
void saturationPublishF64(const double* dry, const double* wet, double* out, std::size_t n,
                          double dryGain, double wetGain, bool mixed) noexcept {
    simdSaturationPublish<SaturationVector>(dry, wet, out, n, dryGain, wetGain, mixed);
}
void expanderGainSimd(const double* envelope, double* gains, std::size_t n, double thresholdDb,
                      double ratio) noexcept {
    simdExpanderGain<SaturationVector>(envelope, gains, n, thresholdDb, ratio);
}
void dynamicEqGainSimd(const double* envelope, double* gains, std::size_t n, double thresholdDb,
                       double staticGainDb, double rangeDb) noexcept {
    simdDynamicEqGain<SaturationVector>(envelope, gains, n, thresholdDb, staticGainDb, rangeDb);
}
void transientGainSimd(const double* fast, const double* slow, double* gains, std::size_t n,
                       const double* parameters) noexcept {
    simdTransientGain<SaturationVector>(fast, slow, gains, n, parameters);
}
double minimumSimd(const double* x, std::size_t n) noexcept {
    return simdMinimum<SaturationVector>(x, n);
}
void magnitudeF32(const float* in, double* out, std::size_t n, bool accumulate) noexcept {
    simdMagnitude<SaturationVector>(in, out, n, accumulate);
}
void magnitudeF64(const double* in, double* out, std::size_t n, bool accumulate) noexcept {
    simdMagnitude<SaturationVector>(in, out, n, accumulate);
}
void applyGainsF32(const float* in, const double* gains, float* out, std::size_t n, double scale,
                   double ceiling, const float* dry, double dryGain, double wetGain,
                   bool mixed) noexcept {
    simdApplyGains<SaturationVector>(in, gains, out, n, scale, ceiling, dry, dryGain, wetGain,
                                     mixed);
}
void applyGainsF64(const double* in, const double* gains, double* out, std::size_t n, double scale,
                   double ceiling, const double* dry, double dryGain, double wetGain,
                   bool mixed) noexcept {
    simdApplyGains<SaturationVector>(in, gains, out, n, scale, ceiling, dry, dryGain, wetGain,
                                     mixed);
}
void compressorGainSimd(const double* envelope, double* gains, std::size_t n, double thresholdDb,
                        double ratio, double kneeDb, bool limiter) noexcept {
    simdCompressorGain<SaturationVector>(envelope, gains, n,
                                         makeCompressorCurve(thresholdDb, ratio, kneeDb, limiter));
}
void compressorGainPreparedSimd(const double* envelope, double* gains, std::size_t n,
                                const CompressorCurve& curve) noexcept {
    simdCompressorGain<SaturationVector>(envelope, gains, n, curve);
}

void adaaCubicSimd(const double* x, double* y, std::size_t n, double previous, bool active,
                   double dc, double gain) noexcept {
    simdAdaaPiecewise<SaturationVector, 0>(x, y, n, previous, active, dc, gain);
}
void shapeCubicSimd(const double* x, double* y, std::size_t n, double dc, double gain) noexcept {
    simdShapeCubic<SaturationVector>(x, y, n, dc, gain);
}
#if defined(__aarch64__) || defined(_M_ARM64)
void adaaClipSimd(const double* x, double* y, std::size_t n, double previous, bool active,
                  double dc, double gain) noexcept {
    simdAdaaPiecewise<SaturationVector, 1>(x, y, n, previous, active, dc, gain);
}
void adaaFoldSimd(const double* x, double* y, std::size_t n, double previous, bool active,
                  double dc, double gain) noexcept {
    simdAdaaPiecewise<SaturationVector, 2>(x, y, n, previous, active, dc, gain);
}
#endif

void cubicDirectF32(const float* in, float* out, std::size_t n, const double* stage, double dc,
                    double gain, double dryGain, double wetGain, bool mixed) noexcept {
    simdPiecewiseDirect<SaturationVector, 0>(in, out, n, stage, dc, gain, dryGain, wetGain, mixed);
}
void cubicDirectF64(const double* in, double* out, std::size_t n, const double* stage, double dc,
                    double gain, double dryGain, double wetGain, bool mixed) noexcept {
    simdPiecewiseDirect<SaturationVector, 0>(in, out, n, stage, dc, gain, dryGain, wetGain, mixed);
}

void foldDirectF32(const float* in, float* out, std::size_t n, const double* stage, double dc,
                   double gain, double dryGain, double wetGain, bool mixed) noexcept {
    simdPiecewiseDirect<SaturationVector, 2>(in, out, n, stage, dc, gain, dryGain, wetGain, mixed);
}
void foldDirectF64(const double* in, double* out, std::size_t n, const double* stage, double dc,
                   double gain, double dryGain, double wetGain, bool mixed) noexcept {
    simdPiecewiseDirect<SaturationVector, 2>(in, out, n, stage, dc, gain, dryGain, wetGain, mixed);
}

void transientExponentSimd(const double* fast, const double* slow, double* out, std::size_t n,
                           const double* parameters) noexcept {
    simdTransientExponent<SaturationVector>(fast, slow, out, n, parameters);
}

#if defined(__aarch64__) || defined(_M_ARM64)
using RecursivePair = Double2;
#else
using RecursivePair = DoublePair;
#endif
// Exact ordered TDF2 recurrences, with one independent channel per lane.
// State = {z1L,z1R,z2L,z2R,envelopeL,envelopeR}; coefficients = b0,b1,b2,a1,a2.
template <class T>
void dynamicEqDetectPair(const T* left, const T* right, double* envelopeL, double* envelopeR,
                         std::size_t n, const double* c, double* state, double attack,
                         double release) noexcept {
    using V = RecursivePair;
    auto z1 = V::load(state), z2 = V::load(state + 2), envelope = V::load(state + 4);
    const auto b0 = V::set(c[0]), b1 = V::set(c[1]), b2 = V::set(c[2]), a1 = V::set(c[3]),
               a2 = V::set(c[4]);
    for (std::size_t i = 0; i < n; ++i) {
        const auto x = V::pair(double(left[i]), double(right[i]));
        const auto y = V::add(V::mul(b0, x), z1);
        z1 = V::add(V::sub(V::mul(b1, x), V::mul(a1, y)), z2);
        z2 = V::sub(V::mul(b2, x), V::mul(a2, y));
        const auto level = V::abs(y);
        envelope =
            V::add(level, V::mul(V::select(V::gt(level, envelope), V::set(attack), V::set(release)),
                                 V::sub(envelope, level)));
        envelopeL[i] = V::first(envelope);
        envelopeR[i] = V::second(envelope);
    }
    V::store(state, z1);
    V::store(state + 2, z2);
    V::store(state + 4, envelope);
}
void dynamicEqApplyPair(double* left, double* right, const double* weightL, const double* weightR,
                        std::size_t n, const double* c, double* state) noexcept {
    using V = RecursivePair;
    auto z1 = V::load(state), z2 = V::load(state + 2);
    const auto b0 = V::set(c[0]), b1 = V::set(c[1]), b2 = V::set(c[2]), a1 = V::set(c[3]),
               a2 = V::set(c[4]);
    for (std::size_t i = 0; i < n; ++i) {
        const auto x = V::pair(left[i], right[i]);
        const auto y = V::add(V::mul(b0, x), z1);
        z1 = V::add(V::sub(V::mul(b1, x), V::mul(a1, y)), z2);
        z2 = V::sub(V::mul(b2, x), V::mul(a2, y));
        const auto result = V::add(x, V::mul(V::pair(weightL[i], weightR[i]), y));
        left[i] = V::first(result);
        right[i] = V::second(result);
    }
    V::store(state, z1);
    V::store(state + 2, z2);
}
