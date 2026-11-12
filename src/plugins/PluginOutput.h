#pragma once
#include "PluginInternal.h"
#include <algorithm>
#include <cstring>
namespace cydsp::detail {
class PluginOutput {
    struct Record {
        PluginEvent event;
        std::size_t order;
    };
    std::vector<Record> events_;
    std::vector<std::uint8_t> bytes_;
    std::size_t count_ = 0, used_ = 0;

  public:
    void prepare(std::size_t events, std::size_t bytes) {
        events_.resize(events);
        bytes_.resize(bytes);
        reset();
    }
    void reset() noexcept {
        count_ = used_ = 0;
    }
    Status add(PluginEvent e) noexcept {
        if (count_ == events_.size() || e.bytes.size() > bytes_.size() - used_)
            return Status::update_queue_full;
        if (!e.bytes.empty()) {
            std::memcpy(bytes_.data() + used_, e.bytes.data(), e.bytes.size());
            e.bytes = {bytes_.data() + used_, e.bytes.size()};
            used_ += e.bytes.size();
        }
        events_[count_] = {e, count_};
        ++count_;
        return Status::ok;
    }
    Status finish(PluginEventBuffer* result) noexcept {
        if (!result)
            return Status::ok;
        std::sort(events_.begin(), events_.begin() + std::ptrdiff_t(count_),
                  [](const auto& a, const auto& b) {
                      return a.event.frame < b.event.frame ||
                             (a.event.frame == b.event.frame && a.order < b.order);
                  });
        for (std::size_t i = 0; i < count_; ++i) {
            auto r = result->add(events_[i].event);
            if (r != Status::ok)
                return r;
        }
        return Status::ok;
    }
};
} // namespace cydsp::detail
