#include "cydsp/processors/Convolver.h"
namespace cydsp {
template <Sample T>
    requires(!std::is_const_v<T>)
void Convolver<T>::swap(Convolver& other) noexcept {
    static_assert(std::is_nothrow_swappable_v<decltype(engine_)>);
    engine_.swap(other.engine_);
    std::swap(config_, other.config_);
    std::swap(prepared_, other.prepared_);
}
template <Sample T>
    requires(!std::is_const_v<T>)
Status Convolver<T>::prepare(const ProcessSpec& spec, std::span<const T> ir,
                             const ConvolutionConfig& config, CpuBackend backend) noexcept {
    if (!spec.isValid())
        return Status::invalid_spec;
    Convolver next;
    Status status;
    switch (config.mode) {
    case ConvolutionMode::hybrid:
        status = std::get<HybridConvolver<T>>(next.engine_)
                     .prepare(spec, ir, config.earlyPartition, backend);
        break;
    case ConvolutionMode::nonuniform:
        status = next.engine_.template emplace<NonuniformConvolver<T>>().prepare(
            spec, ir, config.earlyPartition, config.latePartition, backend);
        break;
    case ConvolutionMode::scheduled:
        status = next.engine_.template emplace<ScheduledConvolver<T>>().prepare(
            spec, ir, config.earlyPartition, config.latePartition, backend);
        break;
    default:
        return Status::invalid_argument;
    }
    if (status != Status::ok)
        return status;
    next.config_ = config;
    next.prepared_ = true;
    swap(next);
    return Status::ok;
}
template <Sample T>
    requires(!std::is_const_v<T>)
void Convolver<T>::reset() noexcept {
    std::visit([](auto& engine) noexcept { engine.reset(); }, engine_);
}
template <Sample T>
    requires(!std::is_const_v<T>)
Status Convolver<T>::process(AudioBlock<const T> input, AudioBlock<T> output) noexcept {
    return std::visit([&](auto& engine) noexcept { return engine.process(input, output); },
                      engine_);
}
template <Sample T>
    requires(!std::is_const_v<T>)
std::size_t Convolver<T>::tailFrames() const noexcept {
    return std::visit([](const auto& engine) noexcept { return engine.tailFrames(); }, engine_);
}
template <Sample T>
    requires(!std::is_const_v<T>)
CpuBackend Convolver<T>::backend() const noexcept {
    return std::visit([](const auto& engine) noexcept { return engine.backend(); }, engine_);
}
template class Convolver<float>;
template class Convolver<double>;
} // namespace cydsp
