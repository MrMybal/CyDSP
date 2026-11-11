#include "cydsp/processors/Gain.h"
#include "detail/LinearRamp.h"
namespace cydsp {
template <Sample T>
    requires(!std::is_const_v<T>)
Status Gain<T>::prepare(const ProcessSpec& spec, CpuBackend backend) noexcept {
    if (!spec.isValid())
        return Status::invalid_spec;
    VectorOps next;
    const auto status = VectorOps::create(backend, next);
    if (status != Status::ok)
        return status;
    spec_ = spec;
    ops_ = next;
    prepared_ = true;
    reset();
    return Status::ok;
}
template <Sample T>
    requires(!std::is_const_v<T>)
void Gain<T>::reset() noexcept {
    current_ = target_;
    remaining_ = 0;
}
template <Sample T>
    requires(!std::is_const_v<T>)
Status Gain<T>::setGain(T gain, std::size_t rampFrames) noexcept {
    if (!isFinite(gain))
        return Status::invalid_argument;
    if (!prepared_)
        return Status::not_prepared;
    if (rampFrames && !isFinite(static_cast<double>(gain) - static_cast<double>(current_)))
        return Status::invalid_argument;
    target_ = static_cast<double>(gain);
    remaining_ = rampFrames;
    if (!rampFrames)
        current_ = target_;
    return Status::ok;
}
template <Sample T>
    requires(!std::is_const_v<T>)
Status Gain<T>::process(AudioBlock<const T> input, AudioBlock<T> output) noexcept {
    if (!prepared_)
        return Status::not_prepared;
    const auto status = validateProcessBlocks(spec_, input, output);
    if (status != Status::ok)
        return status;
    if (!remaining_) {
        for (std::size_t ch = 0; ch < spec_.channels; ++ch)
            (void)ops_.scale(input.channel(ch), output.channel(ch), static_cast<T>(current_));
    } else {
        for (std::size_t frame = 0; frame < input.frames(); ++frame) {
            current_ = detail::advanceLinearRamp(current_, target_, remaining_);
            for (std::size_t ch = 0; ch < spec_.channels; ++ch)
                output.channel(ch)[frame] = input.channel(ch)[frame] * static_cast<T>(current_);
        }
    }
    return Status::ok;
}
template class Gain<float>;
template class Gain<double>;
} // namespace cydsp
