#pragma once
#include "cydsp/core/Types.h"
#include <array>
#include <atomic>
#include <stdexcept>
#include <vector>
namespace cydsp {
// Exactly one producer and one consumer; prepare/move/destruction require both idle.
template <class T>
    requires(std::is_trivially_copyable_v<T> && std::is_default_constructible_v<T>)
class SpscQueue {
    static_assert(std::atomic<std::size_t>::is_always_lock_free);

  public:
    Status prepare(std::size_t capacity) noexcept {
        if (!capacity || capacity == std::numeric_limits<std::size_t>::max())
            return Status::invalid_argument;
        try {
            std::vector<T> next(capacity + 1);
            storage_.swap(next);
            read_.store(0);
            write_.store(0);
            return Status::ok;
        } catch (const std::bad_alloc&) {
            return Status::allocation_failed;
        } catch (const std::length_error&) {
            return Status::invalid_argument;
        }
    }
    bool push(const T& value) noexcept {
        if (storage_.empty())
            return false;
        const auto w = write_.load(std::memory_order_relaxed), next = following(w);
        if (next == read_.load(std::memory_order_acquire))
            return false;
        storage_[w] = value;
        write_.store(next, std::memory_order_release);
        return true;
    }
    // Borrow valid until this consumer calls pop/discard. Only the consumer may peek.
    const T* peek() const noexcept {
        if (storage_.empty())
            return nullptr;
        const auto r = read_.load(std::memory_order_relaxed);
        return r == write_.load(std::memory_order_acquire) ? nullptr : &storage_[r];
    }
    bool discard() noexcept {
        if (!peek())
            return false;
        read_.store(following(read_.load(std::memory_order_relaxed)), std::memory_order_release);
        return true;
    }
    bool pop(T& value) noexcept {
        const auto* p = peek();
        if (!p)
            return false;
        value = *p;
        return discard();
    }
    std::size_t capacity() const noexcept {
        return storage_.empty() ? 0 : storage_.size() - 1;
    }

  private:
    // Indices stay below storage_.size(); a compare replaces the integer division.
    std::size_t following(std::size_t index) const noexcept {
        return index + 1 == storage_.size() ? 0 : index + 1;
    }
    std::vector<T> storage_;
    std::atomic<std::size_t> read_{0};
    std::array<std::byte, 128> separator_{};
    std::atomic<std::size_t> write_{0};
};
} // namespace cydsp
