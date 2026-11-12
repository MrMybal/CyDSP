#pragma once
#include "cydsp/sources/AudioSource.h"
#include <memory>
namespace cydsp {
struct TempoPoint {
    double beat = 0, bpm = 120;
    unsigned numerator = 4, denominator = 4;
};
struct MusicalPosition {
    double beat = 0, bpm = 120;
    std::int64_t bar = 0;
    double beatInBar = 0;
    unsigned numerator = 4, denominator = 4;
};
class TempoMap {
  public:
    Status prepare(double sampleRate, std::span<const TempoPoint>) noexcept;
    [[nodiscard]] MusicalPosition position(std::uint64_t frame) const noexcept;
    Status frameAtBeat(double beat, std::uint64_t&) const noexcept;
    [[nodiscard]] double sampleRate() const noexcept {
        return rate_;
    }

  private:
    struct Point {
        TempoPoint tempo;
        double frame = 0;
        std::int64_t bar = 0;
        double barBeat = 0;
    };
    std::vector<Point> points_;
    double rate_ = 0;
};
enum class TransportAction { play, pause, stop, seek, setLoop };
struct TransportCommand {
    TransportAction action = TransportAction::pause;
    std::uint64_t first = 0, end = 0;
    bool enabled = false;
};
struct TransportPosition {
    std::uint64_t frame = 0;
    bool playing = false, looping = false;
    std::uint32_t underruns = 0;
};
enum class UnderrunPolicy { hold, silenceAndAdvance };
template <Sample T> class AudioTransport {
  public:
    AudioTransport();
    ~AudioTransport();
    AudioTransport(AudioTransport&&) noexcept;
    AudioTransport& operator=(AudioTransport&&) noexcept;
    // Borrowed source must outlive transport use. Rates/channels must match the prepared output.
    Status prepare(const ProcessSpec&, AudioSource<T>&, std::size_t commandCapacity = 64,
                   CpuBackend = CpuBackend::automatic) noexcept;
    // Single control producer, single audio consumer. Commands apply at the next callback boundary.
    Status enqueue(const TransportCommand&) noexcept;
    Status process(AudioBlock<T>, UnderrunPolicy = UnderrunPolicy::hold) noexcept;
    // Control-thread snapshot: bounded retries, never a lock on the audio thread.
    Status position(TransportPosition&) const noexcept;
    void reset() noexcept; // Callback owner only, retained state/storage.
  private:
    struct State;
    std::unique_ptr<State> state_;
};
extern template class AudioTransport<float>;
extern template class AudioTransport<double>;
} // namespace cydsp
