#include "cydsp/processors/DelayLine.h"
#include "detail/CheckedSize.h"
#include "detail/LinearRamp.h"
#include <algorithm>
#include <new>
#include <stdexcept>
namespace cydsp {
template <Sample T>
    requires(!std::is_const_v<T>)
void DelayLine<T>::swap(DelayLine& other) noexcept {
    std::swap(spec_, other.spec_);
    history_.swap(other.history_);
    std::swap(capacity_, other.capacity_);
    std::swap(write_, other.write_);
    std::swap(maximum_, other.maximum_);
    std::swap(delay_, other.delay_);
    std::swap(target_, other.target_);
    std::swap(remaining_, other.remaining_);
}
template <Sample T>
    requires(!std::is_const_v<T>)
Status DelayLine<T>::prepare(const ProcessSpec& spec, std::size_t maximum) noexcept {
    if (!spec.isValid())
        return Status::invalid_spec;
    if (maximum > (std::size_t{1} << 24))
        return Status::invalid_argument;
    std::size_t count{};
    if (!detail::checkedMultiply(maximum + 2, spec.channels, count))
        return Status::invalid_argument;
    DelayLine next;
    try {
        next.history_.resize(count);
    } catch (const std::bad_alloc&) {
        return Status::allocation_failed;
    } catch (const std::length_error&) {
        return Status::invalid_argument;
    }
    next.spec_ = spec;
    next.capacity_ = maximum + 2;
    next.maximum_ = maximum;
    swap(next);
    return Status::ok;
}
template <Sample T>
    requires(!std::is_const_v<T>)
Status DelayLine<T>::setDelay(double frames, std::size_t rampFrames) noexcept {
    if (!capacity_)
        return Status::not_prepared;
    if (!isFinite(frames) || frames < 0 || frames > static_cast<double>(maximum_))
        return Status::invalid_argument;
    target_ = frames;
    remaining_ = rampFrames;
    if (!rampFrames)
        delay_ = target_;
    return Status::ok;
}
template <Sample T>
    requires(!std::is_const_v<T>)
void DelayLine<T>::reset() noexcept {
    std::fill(history_.begin(), history_.end(), T{});
    write_ = 0;
    delay_ = target_;
    remaining_ = 0;
}
template <Sample T>
    requires(!std::is_const_v<T>)
void DelayLine<T>::processFrame(AudioBlock<const T> input, AudioBlock<T> output, std::size_t frame,
                                std::size_t read, T fraction) noexcept {
    const auto older = read ? read - 1 : capacity_ - 1;
    for (std::size_t ch = 0; ch < spec_.channels; ++ch) {
        auto* history = history_.data() + ch * capacity_;
        history[write_] = input.channel(ch)[frame];
        output.channel(ch)[frame] =
            fraction == T{0} ? history[read]
                             : history[read] * (T{1} - fraction) + history[older] * fraction;
    }
    if (++write_ == capacity_)
        write_ = 0;
}
template <Sample T>
    requires(!std::is_const_v<T>)
void DelayLine<T>::processConstant(AudioBlock<const T> input, AudioBlock<T> output) noexcept {
    const auto integer = static_cast<std::size_t>(delay_);
    const auto fraction = static_cast<T>(delay_ - static_cast<double>(integer));
    auto read = (write_ + capacity_ - integer) % capacity_;
    for (std::size_t frame = 0; frame < input.frames(); ++frame) {
        processFrame(input, output, frame, read, fraction);
        if (++read == capacity_)
            read = 0;
    }
}
template <Sample T>
    requires(!std::is_const_v<T>)
Status DelayLine<T>::process(AudioBlock<const T> input, AudioBlock<T> output) noexcept {
    if (!capacity_)
        return Status::not_prepared;
    const auto status = validateProcessBlocks(spec_, input, output);
    if (status != Status::ok)
        return status;
    const auto ramp = std::min(input.frames(), remaining_);
    for (std::size_t frame = 0; frame < ramp; ++frame) {
        delay_ = detail::advanceLinearRamp(delay_, target_, remaining_);
        const auto integer = static_cast<std::size_t>(delay_);
        const auto fraction = static_cast<T>(delay_ - static_cast<double>(integer));
        const auto read = (write_ + capacity_ - integer) % capacity_;
        processFrame(input, output, frame, read, fraction);
    }
    processConstant(input.subBlock(ramp, input.frames() - ramp),
                    output.subBlock(ramp, output.frames() - ramp));
    return Status::ok;
}

template class DelayLine<float>;
template class DelayLine<double>;
} // namespace cydsp
