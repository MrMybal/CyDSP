#pragma once
#include "cydsp/processors/Convolver.h"
#include <memory>
namespace cydsp {
template <Sample T> struct ConvolutionRoute {
    std::size_t inputChannel = 0, outputChannel = 0;
    std::span<const T> impulse;
};
// Sparse MIMO convolution; duplicate routes add. Impulses are copied during prepare.
template <Sample T>
    requires(!std::is_const_v<T>)
class MatrixConvolver {
  public:
    MatrixConvolver() noexcept;
    ~MatrixConvolver();
    MatrixConvolver(const MatrixConvolver&) = delete;
    MatrixConvolver& operator=(const MatrixConvolver&) = delete;
    MatrixConvolver(MatrixConvolver&&) noexcept;
    MatrixConvolver& operator=(MatrixConvolver&&) noexcept;
    [[nodiscard]] Status prepare(const ProcessSpec& inputSpec, std::size_t outputChannels,
                                 std::span<const ConvolutionRoute<T>>,
                                 std::size_t earlyPartition = 128, std::size_t latePartition = 1024,
                                 CpuBackend = CpuBackend::automatic) noexcept;
    // The legacy overload selects nonuniform. Config also supports hybrid and scheduled.
    [[nodiscard]] Status prepare(const ProcessSpec& inputSpec, std::size_t outputChannels,
                                 std::span<const ConvolutionRoute<T>>, const ConvolutionConfig&,
                                 CpuBackend = CpuBackend::automatic) noexcept;
    [[nodiscard]] ConvolutionConfig configuration() const noexcept {
        return config_;
    }
    [[nodiscard]] bool schedulingActive() const noexcept {
        return progressive_ != nullptr;
    }
    // Scheduled late-tier chunks only; excludes direct/early work and does not bound CPU time.
    [[nodiscard]] std::size_t workUnitsPerPartition() const noexcept;
    [[nodiscard]] std::size_t maximumWorkUnitsPerFrame() const noexcept;
    void reset() noexcept;
    [[nodiscard]] Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;
    [[nodiscard]] std::size_t latencyFrames() const noexcept {
        return 0;
    }
    [[nodiscard]] std::size_t tailFrames() const noexcept {
        return tail_;
    }
    [[nodiscard]] CpuBackend backend() const noexcept {
        return ops_.backend();
    }

  private:
    struct Route {
        std::size_t input = 0, output = 0;
        std::vector<T> coefficients;
    };
    struct Tier {
        struct Filter {
            std::size_t input = 0, output = 0, parts = 0, offset = 0;
        };
        struct Job {
            std::size_t filter = 0, part = 0;
        };
        // Spectrum of the first partition of one route's impulse (offset 0).
        struct Head {
            std::size_t input = 0, output = 0, offset = 0;
        };
        RealFft<T> fft;
        VectorOps ops;
        std::vector<std::size_t> inputs, outputs;
        std::vector<unsigned char> coveredOutputs;
        std::vector<Filter> routes;
        std::vector<Job> jobs;
        std::vector<T> input, ready, overlap, time;
        std::vector<std::complex<T>> filters, history, sum, headFilters;
        std::vector<Head> heads;
        // With heads, the inverse transform of a completed partition waits for
        // the next partition so a whole-partition call can add its head first.
        bool pending = false;
        std::size_t partition = 0, parts = 0, position = 0, head = 0, done = 0, baseJobs = 0,
                    extraJobs = 0, debt = 0;
        Status prepare(std::span<const ConvolutionRoute<T>>, std::size_t, std::size_t, std::size_t,
                       CpuBackend, bool progressive = false, bool spectralHead = false) noexcept;
        void reset() noexcept;
        void process(AudioBlock<const T>, AudioBlock<T>, bool aligned = false) noexcept;
        void accumulate(const Filter&, std::size_t) noexcept;
        void boundary() noexcept;
        void emit() noexcept;
        void alignedPartition(AudioBlock<const T>, AudioBlock<T>, std::size_t) noexcept;
    };
    struct ProgressiveTier;
    void swap(MatrixConvolver&) noexcept;
    ProcessSpec spec_{};
    ConvolutionConfig config_{ConvolutionMode::nonuniform, 128, 1024};
    std::unique_ptr<ProgressiveTier> progressive_;
    std::vector<Route> routes_;
    std::vector<T> headHistory_;
    Tier early_, late_;
    AudioBuffer<T> input_, scratch_;
    VectorOps ops_;
    std::size_t outputs_ = 0, tile_ = 0, tail_ = 0, headSize_ = 0, write_ = 0, capacity_ = 0;
    // True while the direct head history holds only zeros (reset, or the last
    // partition was convolved spectrally and its tail lives in the tier overlap).
    bool headZero_ = true;
};
extern template class MatrixConvolver<float>;
extern template class MatrixConvolver<double>;
} // namespace cydsp
