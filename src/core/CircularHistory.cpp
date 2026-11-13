#include "cydsp/core/CircularHistory.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <utility>
namespace cydsp {
template <Sample T> void CircularHistory<T>::swap(CircularHistory& o) noexcept {
    storage_.swap(o.storage_);
    std::swap(channels_, o.channels_);
    std::swap(capacity_, o.capacity_);
    std::swap(head_, o.head_);
    std::swap(filled_, o.filled_);
}
template <Sample T> CircularHistory<T>::CircularHistory(CircularHistory&& o) noexcept {
    swap(o);
}
template <Sample T>
CircularHistory<T>& CircularHistory<T>::operator=(CircularHistory&& o) noexcept {
    if (this != &o) {
        CircularHistory n(std::move(o));
        swap(n);
    }
    return *this;
}
template <Sample T>
Status CircularHistory<T>::prepare(std::size_t channels, std::size_t frames) noexcept {
    if (!channels || channels > 64 || !frames || frames > (1u << 24) ||
        frames * channels > (1u << 27))
        return Status::invalid_spec;
    try {
        CircularHistory n;
        n.storage_.resize(channels * frames);
        n.channels_ = channels;
        n.capacity_ = frames;
        swap(n);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
template <Sample T> void CircularHistory<T>::pushUnchecked(std::span<const T> frame) noexcept {
    for (std::size_t ch = 0; ch < channels_; ++ch)
        storage_[ch * capacity_ + head_] = frame[ch];
    if (++head_ == capacity_)
        head_ = 0;
    filled_ = std::min(filled_ + 1, capacity_);
}
template <Sample T> Status CircularHistory<T>::pushFrame(std::span<const T> frame) noexcept {
    if (!capacity_)
        return Status::not_prepared;
    if (frame.size() != channels_)
        return Status::channel_mismatch;
    for (T value : frame)
        if (!isFinite(value))
            return Status::invalid_argument;
    pushUnchecked(frame);
    return Status::ok;
}
template <Sample T> Status CircularHistory<T>::push(AudioBlock<const T> input) noexcept {
    if (!capacity_)
        return Status::not_prepared;
    if (!input.isValid())
        return Status::invalid_argument;
    if (input.channels() != channels_)
        return Status::channel_mismatch;
    for (std::size_t ch = 0; ch < channels_; ++ch)
        for (T v : input.channel(ch))
            if (!isFinite(v))
                return Status::invalid_argument;
    std::array<T, 64> frame{};
    for (std::size_t i = 0; i < input.frames(); ++i) {
        for (std::size_t ch = 0; ch < channels_; ++ch)
            frame[ch] = input.channel(ch)[i];
        pushUnchecked(std::span<const T>(frame).first(channels_));
    }
    return Status::ok;
}
template <Sample T>
double CircularHistory<T>::integer(std::size_t ch, std::ptrdiff_t age) const noexcept {
    if (age < 0 || std::size_t(age) >= filled_)
        return 0;
    return double(
        storage_[ch * capacity_ + (head_ + capacity_ - 1 - std::size_t(age)) % capacity_]);
}
template <Sample T>
T CircularHistory<T>::read(std::size_t ch, double age,
                           HistoryInterpolation interpolation) const noexcept {
    if (!capacity_ || ch >= channels_ || !isFinite(age) || age < 0 || age > double(capacity_ - 1) ||
        unsigned(interpolation) > unsigned(HistoryInterpolation::catmullRom))
        return T{};
    const auto j = std::ptrdiff_t(age);
    const double t = age - double(j), b = integer(ch, j), c = integer(ch, j + 1);
    if (interpolation == HistoryInterpolation::nearest)
        return T(integer(ch, j + (t >= .5 ? 1 : 0)));
    if (interpolation == HistoryInterpolation::linear)
        return T(b + (c - b) * t);
    const double a = integer(ch, j - 1), d = integer(ch, j + 2);
    if (interpolation == HistoryInterpolation::lagrange3)
        return T(-a * t * (t - 1) * (t - 2) / 6 + b * (t + 1) * (t - 1) * (t - 2) / 2 -
                 c * (t + 1) * t * (t - 2) / 2 + d * (t + 1) * t * (t - 1) / 6);
    return T(b + .5 * t * (c - a + t * (2 * a - 5 * b + 4 * c - d + t * (3 * (b - c) + d - a))));
}
template <Sample T>
Status CircularHistory<T>::readWindow(std::size_t ch, std::ptrdiff_t first,
                                      std::span<double> out) const noexcept {
    if (!capacity_)
        return Status::not_prepared;
    if (ch >= channels_)
        return Status::channel_mismatch;
    if (out.size() > (1u << 24) || first < -std::ptrdiff_t(1u << 24) ||
        first > std::ptrdiff_t(1u << 24))
        return Status::invalid_argument;
    std::fill(out.begin(), out.end(), 0.);
    const auto begin = std::size_t(std::max<std::ptrdiff_t>(0, -first));
    const auto end = std::size_t(std::max<std::ptrdiff_t>(
        0, std::min(std::ptrdiff_t(out.size()), std::ptrdiff_t(filled_) - first)));
    if (begin >= end)
        return Status::ok;
    const auto age = std::size_t(first + std::ptrdiff_t(begin));
    auto position = head_ > age ? head_ - 1 - age : capacity_ + head_ - 1 - age;
    const auto* data = storage_.data() + ch * capacity_;
    for (std::size_t i = begin; i < end; ++i) {
        out[i] = double(data[position]);
        position = position ? position - 1 : capacity_ - 1;
    }
    return Status::ok;
}

template <Sample T> void CircularHistory<T>::reset() noexcept {
    std::fill(storage_.begin(), storage_.end(), T{});
    head_ = filled_ = 0;
}
template class CircularHistory<float>;
template class CircularHistory<double>;
} // namespace cydsp
