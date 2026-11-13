#pragma once
#include "cydsp/spatial/FdnReverb.h"
#include "cydsp/spatial/SpatialMath.h"
#include <cstdint>
#include <memory>
namespace cydsp {
struct AcousticRoom {
    std::uint32_t id = 1;
    double volumeCubicMetres = 100, surfaceSquareMetres = 120;
    std::array<double, 3> meanAbsorption{.2, .3, .4};
};
enum class RoomDecayModel { sabine, eyring };
struct RoomResponse {
    std::array<double, 3> decaySeconds{1, 1, 1};
    double preDelaySeconds = 0, diffusion = .7, wet = 1;
};
[[nodiscard]] Status estimateRoomResponse(const AcousticRoom&, const AcousticMedium&, RoomResponse&,
                                          RoomDecayModel = RoomDecayModel::eyring) noexcept;
struct AcousticPortal {
    std::uint32_t id = 1, roomA = 1, roomB = 2;
    double lengthMetres = 1, opening = 1;
    std::array<double, 3> bandGains{1, 1, 1};
    AcousticMedium medium;
    bool bidirectional = true;
};
// Immutable prepared transfer topology. Edge lengths include the travel distances
// supplied by the host. It does not trace engine geometry or infer room obstacles.
class AcousticPortalGraph {
  public:
    AcousticPortalGraph();
    ~AcousticPortalGraph();
    AcousticPortalGraph(AcousticPortalGraph&&) noexcept;
    AcousticPortalGraph& operator=(AcousticPortalGraph&&) noexcept;
    [[nodiscard]] Status prepare(std::span<const AcousticRoom>, std::span<const AcousticPortal>,
                                 double lossWeightSecondsPerDb = 0) noexcept;
    [[nodiscard]] Status route(std::uint32_t sourceRoom, std::uint32_t destinationRoom,
                               std::span<std::uint32_t> portalIds, std::size_t& count,
                               AcousticTravel&) const noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
// Immutable regular grid with trilinear interpolation; positions clamp to the
// grid boundary. Values are indexed (z*height+y)*width+x.
class RoomCellGrid {
  public:
    RoomCellGrid();
    ~RoomCellGrid();
    RoomCellGrid(RoomCellGrid&&) noexcept;
    RoomCellGrid& operator=(RoomCellGrid&&) noexcept;
    [[nodiscard]] Status prepare(Vec3 origin, Vec3 spacing, std::array<std::size_t, 3> dimensions,
                                 std::span<const RoomResponse>) noexcept;
    [[nodiscard]] Status sample(Vec3 position, RoomResponse&) const noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
// One network per fixed room slot. The owner begins a block, adds all sends, then
// renders once. Inputs clear on begin; reverb states/tails persist. Room outputs
// can be routed separately to listeners, or summed with mix(). No slots are stolen.
template <Sample T> class RoomReverbBank {
  public:
    RoomReverbBank();
    ~RoomReverbBank();
    RoomReverbBank(RoomReverbBank&&) noexcept;
    RoomReverbBank& operator=(RoomReverbBank&&) noexcept;
    [[nodiscard]] Status prepare(const ProcessSpec&, std::size_t rooms = 8, const FdnConfig& = {},
                                 CpuBackend = CpuBackend::automatic) noexcept;
    [[nodiscard]] Status setRoom(std::size_t, const FdnParameters&,
                                 std::size_t rampFrames = 256) noexcept;
    [[nodiscard]] Status beginBlock(std::size_t frames) noexcept;
    [[nodiscard]] Status send(std::size_t room, AudioBlock<const T>, double gain = 1) noexcept;
    [[nodiscard]] Status render() noexcept;
    [[nodiscard]] Status mix(AudioBlock<T>) const noexcept;
    [[nodiscard]] AudioBlock<const T> roomOutput(std::size_t room) const noexcept;
    void reset() noexcept;

  private:
    struct State;
    std::unique_ptr<State> state_;
};
extern template class RoomReverbBank<float>;
extern template class RoomReverbBank<double>;
} // namespace cydsp
