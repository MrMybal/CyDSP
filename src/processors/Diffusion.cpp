#include "cydsp/processors/Diffusion.h"
#include "detail/Analysis.h"
#include <vector>
namespace cydsp {
template <Sample T> struct Diffuser<T>::State {
    ProcessSpec spec;
    std::vector<DiffusionStage> stages;
    std::vector<std::size_t> offsets, heads;
    std::vector<T> history;
};
template <Sample T> Diffuser<T>::Diffuser() = default;
template <Sample T> Diffuser<T>::~Diffuser() = default;
template <Sample T> Diffuser<T>::Diffuser(Diffuser&&) noexcept = default;
template <Sample T> Diffuser<T>& Diffuser<T>::operator=(Diffuser&&) noexcept = default;
template <Sample T>
Status Diffuser<T>::prepare(const ProcessSpec& spec,
                            std::span<const DiffusionStage> stages) noexcept {
    if (!detail::validAnalysisSpec(spec) || stages.size() > 32)
        return Status::invalid_spec;
    std::size_t total = 0;
    for (const auto& stage : stages) {
        if (!stage.delayFrames || stage.delayFrames > (1u << 20) || !isFinite(stage.feedback) ||
            std::abs(stage.feedback) >= .9999)
            return Status::invalid_argument;
        if (stage.delayFrames * spec.channels > (1u << 25) - total)
            return Status::invalid_spec;
        total += stage.delayFrames * spec.channels;
    }
    try {
        auto s = std::make_unique<State>();
        s->spec = spec;
        s->stages.assign(stages.begin(), stages.end());
        s->heads.resize(stages.size());
        s->history.resize(total);
        std::size_t offset = 0;
        for (const auto& stage : stages) {
            s->offsets.push_back(offset);
            offset += stage.delayFrames * spec.channels;
        }
        state_.swap(s);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
template <Sample T>
Status Diffuser<T>::process(AudioBlock<const T> in, AudioBlock<T> out) noexcept {
    if (!state_)
        return Status::not_prepared;
    auto& s = *state_;
    auto result = validateProcessBlocks(s.spec, in, out);
    if (result != Status::ok)
        return result;
    result = detail::validateAnalysis(s.spec, in);
    if (result != Status::ok)
        return result;
    for (std::size_t i = 0; i < in.frames(); ++i) {
        for (std::size_t ch = 0; ch < in.channels(); ++ch) {
            double value = double(in.channel(ch)[i]);
            for (std::size_t stage = 0; stage < s.stages.size(); ++stage) {
                const auto& p = s.stages[stage];
                auto& stored = s.history[s.offsets[stage] + ch * p.delayFrames + s.heads[stage]];
                const double output = double(stored) - p.feedback * value;
                stored = T(value + p.feedback * output);
                value = output;
            }
            out.channel(ch)[i] = T(value);
        }
        for (std::size_t stage = 0; stage < s.stages.size(); ++stage)
            if (++s.heads[stage] == s.stages[stage].delayFrames)
                s.heads[stage] = 0;
    }
    return Status::ok;
}
template <Sample T> void Diffuser<T>::reset() noexcept {
    if (state_) {
        std::fill(state_->heads.begin(), state_->heads.end(), 0);
        std::fill(state_->history.begin(), state_->history.end(), T{});
    }
}
template class Diffuser<float>;
template class Diffuser<double>;
} // namespace cydsp
