#pragma once
#include "cydsp/processors/ChannelMatrix.h"
#include <memory>
namespace cydsp {
enum class PanLaw { balanced, linear, sine3dB, sine4_5dB, sine6dB, squareRoot3dB, squareRoot4_5dB };
enum class MixLaw { linear, sine3dB, sine4_5dB, sine6dB, squareRoot3dB, squareRoot4_5dB };
template <Sample T> class Panner {
  public:
    [[nodiscard]] Status prepare(const ProcessSpec&, CpuBackend = CpuBackend::automatic) noexcept;
    [[nodiscard]] Status setPan(double pan, PanLaw = PanLaw::sine3dB,
                                std::size_t rampFrames = 0) noexcept;
    [[nodiscard]] Status process(AudioBlock<const T> input, AudioBlock<T> stereoOutput) noexcept;
    void reset() noexcept {
        matrix_.reset();
    }

  private:
    ChannelMatrix<T> matrix_;
};
template <Sample T> class DryWetMixer {
  public:
    DryWetMixer();
    ~DryWetMixer();
    DryWetMixer(DryWetMixer&&) noexcept;
    DryWetMixer& operator=(DryWetMixer&&) noexcept;
    [[nodiscard]] Status prepare(const ProcessSpec&, std::size_t maximumWetLatency = 0) noexcept;
    [[nodiscard]] Status setMix(double wet, MixLaw = MixLaw::linear) noexcept;
    [[nodiscard]] Status setWetLatency(std::size_t frames) noexcept;
    [[nodiscard]] Status pushDrySamples(AudioBlock<const T>) noexcept;
    [[nodiscard]] Status mixWetSamples(AudioBlock<T>) noexcept;
    void reset() noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
enum class Shaper { tanh, atan, hardClip, cubic, fullWaveRectifier };
template <Sample T> class WaveShaper {
  public:
    using Function = T (*)(T, void*) noexcept;
    [[nodiscard]] Status prepare(const ProcessSpec&) noexcept;
    [[nodiscard]] Status setDrive(double linearGain) noexcept;
    void setShape(Shaper shape) noexcept {
        shape_ = shape;
        function_ = nullptr;
    }
    void setFunction(Function function, void* context = nullptr) noexcept {
        function_ = function;
        context_ = context;
    }
    void reset() noexcept {}
    [[nodiscard]] Status process(AudioBlock<const T>, AudioBlock<T>) noexcept;

  private:
    ProcessSpec spec_{};
    double drive_ = 1;
    Shaper shape_ = Shaper::tanh;
    Function function_ = nullptr;
    void* context_ = nullptr;
    bool prepared_ = false;
};
template <Sample T> class Bias {
  public:
    [[nodiscard]] Status prepare(const ProcessSpec& spec) noexcept {
        if (!spec.isValid())
            return Status::invalid_spec;
        spec_ = spec;
        prepared_ = true;
        return Status::ok;
    }
    [[nodiscard]] Status setBias(T value) noexcept {
        if (!isFinite(value))
            return Status::invalid_argument;
        value_ = value;
        return Status::ok;
    }
    void reset() noexcept {}
    [[nodiscard]] Status process(AudioBlock<const T> in, AudioBlock<T> out) noexcept {
        if (!prepared_)
            return Status::not_prepared;
        const auto status = validateProcessBlocks(spec_, in, out);
        if (status != Status::ok)
            return status;
        for (std::size_t ch = 0; ch < in.channels(); ++ch)
            for (std::size_t i = 0; i < in.frames(); ++i)
                out.channel(ch)[i] = in.channel(ch)[i] + value_;
        return Status::ok;
    }

  private:
    ProcessSpec spec_{};
    T value_{};
    bool prepared_ = false;
};
extern template class Panner<float>;
extern template class Panner<double>;
extern template class DryWetMixer<float>;
extern template class DryWetMixer<double>;
extern template class WaveShaper<float>;
extern template class WaveShaper<double>;
} // namespace cydsp
