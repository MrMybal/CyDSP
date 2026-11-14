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
    simdCubicDirect<SaturationVector>(in, out, n, stage, dc, gain, dryGain, wetGain, mixed);
}
void cubicDirectF64(const double* in, double* out, std::size_t n, const double* stage, double dc,
                    double gain, double dryGain, double wetGain, bool mixed) noexcept {
    simdCubicDirect<SaturationVector>(in, out, n, stage, dc, gain, dryGain, wetGain, mixed);
}
