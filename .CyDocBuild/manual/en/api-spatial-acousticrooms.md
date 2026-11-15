---
id: api-spatial-acousticrooms
title: AcousticRooms
description: Room/portal transfer, response-grid interpolation and retained room-reverb buses.
section: API reference
order: 1169
revision: 1
parent: api-spatial
symbol: cydsp::AcousticRoom
---

Room/portal transfer, response-grid interpolation and retained room-reverb buses.

## Include {#include}

```cpp
#include <cydsp/spatial/AcousticRooms.h>
```

[Usage and contracts](doc:acoustics) · [Common processing rules](doc:contracts)

## `AcousticRoom` {#symbol-acousticroom}

```cpp
struct AcousticRoom;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `id` | `std::uint32_t` | `1` | Value/configuration in the declared type; apply the linked module contract. |
| `volumeCubicMetres` | `double` | `100` | Length in metres. |
| `surfaceSquareMetres` | `double` | `120` | Length in metres. |
| `meanAbsorption` | `std::array<double, 3>` | `{.2, .3, .4}` | Value/configuration in the declared type; apply the linked module contract. |

## `RoomDecayModel` {#symbol-roomdecaymodel}

```cpp
enum class RoomDecayModel { sabine, eyring };
```

## `RoomResponse` {#symbol-roomresponse}

```cpp
struct RoomResponse;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `decaySeconds` | `std::array<double, 3>` | `{1, 1, 1}` | Duration or position in seconds. |
| `preDelaySeconds` | `double` | `0` | Duration or position in seconds. |
| `diffusion` | `double` | `.7` | Value/configuration in the declared type; apply the linked module contract. |
| `wet` | `double` | `1` | Linear amplitude multiplier; follow the processor bounds and mixing law. |

## `estimateRoomResponse` {#symbol-estimateroomresponse}

```cpp
Status estimateRoomResponse(const AcousticRoom&, const AcousticMedium&, RoomResponse&, RoomDecayModel = RoomDecayModel::eyring) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const AcousticRoom &` | — | Value/configuration in the declared type; apply the linked module contract. |
| `argument 2` | `const AcousticMedium &` | — | Value/configuration in the declared type; apply the linked module contract. |
| `argument 3` | `RoomResponse &` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |
| `argument 4` | `RoomDecayModel` | `RoomDecayModel::eyring` | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

## `AcousticPortal` {#symbol-acousticportal}

```cpp
struct AcousticPortal;
```

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `id` | `std::uint32_t` | `1` | Value/configuration in the declared type; apply the linked module contract. |
| `roomA` | `std::uint32_t` | `1` | Value/configuration in the declared type; apply the linked module contract. |
| `roomB` | `std::uint32_t` | `2` | Value/configuration in the declared type; apply the linked module contract. |
| `lengthMetres` | `double` | `1` | Length in metres. |
| `opening` | `double` | `1` | Value/configuration in the declared type; apply the linked module contract. |
| `bandGains` | `std::array<double, 3>` | `{1, 1, 1}` | Value/configuration in the declared type; apply the linked module contract. |
| `medium` | `AcousticMedium` | — | Value/configuration in the declared type; apply the linked module contract. |
| `bidirectional` | `bool` | `true` | Explicit true/false option or predicate. |

## `AcousticPortalGraph` {#symbol-acousticportalgraph}

```cpp
class AcousticPortalGraph;
```

### `AcousticPortalGraph::AcousticPortalGraph` {#symbol-acousticportalgraph-acousticportalgraph}

```cpp
AcousticPortalGraph();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `AcousticPortalGraph::~AcousticPortalGraph` {#symbol-acousticportalgraph-acousticportalgraph-2}

```cpp
~AcousticPortalGraph();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `AcousticPortalGraph::AcousticPortalGraph` {#symbol-acousticportalgraph-acousticportalgraph-3}

```cpp
AcousticPortalGraph(AcousticPortalGraph&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AcousticPortalGraph &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `AcousticPortalGraph::operator=` {#symbol-acousticportalgraph-operator}

```cpp
AcousticPortalGraph& operator=(AcousticPortalGraph&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AcousticPortalGraph &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `AcousticPortalGraph &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `AcousticPortalGraph::prepare` {#symbol-acousticportalgraph-prepare}

```cpp
Status prepare(std::span<const AcousticRoom>, std::span<const AcousticPortal>, double lossWeightSecondsPerDb = 0) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `std::span<const AcousticRoom>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `argument 2` | `std::span<const AcousticPortal>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `lossWeightSecondsPerDb` | `double` | `0` | Logarithmic level in the indicated dB reference. |

Return: `Status` — Check Status; ok reports success.

### `AcousticPortalGraph::route` {#symbol-acousticportalgraph-route}

```cpp
Status route(std::uint32_t sourceRoom, std::uint32_t destinationRoom, std::span<std::uint32_t> portalIds, std::size_t& count, AcousticTravel&) const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `sourceRoom` | `std::uint32_t` | — | Value/configuration in the declared type; apply the linked module contract. |
| `destinationRoom` | `std::uint32_t` | — | Value/configuration in the declared type; apply the linked module contract. |
| `portalIds` | `std::span<std::uint32_t>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |
| `count` | `std::size_t &` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |
| `argument 5` | `AcousticTravel &` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `Status` — Check Status; ok reports success.

## `RoomCellGrid` {#symbol-roomcellgrid}

```cpp
class RoomCellGrid;
```

### `RoomCellGrid::RoomCellGrid` {#symbol-roomcellgrid-roomcellgrid}

```cpp
RoomCellGrid();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `RoomCellGrid::~RoomCellGrid` {#symbol-roomcellgrid-roomcellgrid-2}

```cpp
~RoomCellGrid();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `RoomCellGrid::RoomCellGrid` {#symbol-roomcellgrid-roomcellgrid-3}

```cpp
RoomCellGrid(RoomCellGrid&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `RoomCellGrid &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `RoomCellGrid::operator=` {#symbol-roomcellgrid-operator}

```cpp
RoomCellGrid& operator=(RoomCellGrid&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `RoomCellGrid &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `RoomCellGrid &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `RoomCellGrid::prepare` {#symbol-roomcellgrid-prepare}

```cpp
Status prepare(Vec3 origin, Vec3 spacing, std::array<std::size_t, 3> dimensions, std::span<const RoomResponse>) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `origin` | `Vec3` | — | Value/configuration in the declared type; apply the linked module contract. |
| `spacing` | `Vec3` | — | Value/configuration in the declared type; apply the linked module contract. |
| `dimensions` | `std::array<std::size_t, 3>` | — | Value/configuration in the declared type; apply the linked module contract. |
| `argument 4` | `std::span<const RoomResponse>` | — | Borrowed contiguous range with the element type and constness shown; preserve its lifetime. |

Return: `Status` — Check Status; ok reports success.

### `RoomCellGrid::sample` {#symbol-roomcellgrid-sample}

```cpp
Status sample(Vec3 position, RoomResponse&) const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `position` | `Vec3` | — | Value/configuration in the declared type; apply the linked module contract. |
| `argument 2` | `RoomResponse &` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `Status` — Check Status; ok reports success.

## `RoomReverbBank` {#symbol-roomreverbbank}

```cpp
template <Sample T> class RoomReverbBank;
```

### `RoomReverbBank::RoomReverbBank<T>` {#symbol-roomreverbbank-roomreverbbank-t}

```cpp
RoomReverbBank();
```

Construct an instance; prepare it before processing when the API requires preparation.

### `RoomReverbBank::~RoomReverbBank<T>` {#symbol-roomreverbbank-roomreverbbank-t-2}

```cpp
~RoomReverbBank();
```

Destroy only when no processing or borrowed users remain; destruction may release storage.

### `RoomReverbBank::RoomReverbBank<T>` {#symbol-roomreverbbank-roomreverbbank-t-3}

```cpp
RoomReverbBank(RoomReverbBank&&) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `RoomReverbBank<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `RoomReverbBank::operator=` {#symbol-roomreverbbank-operator}

```cpp
RoomReverbBank& operator=(RoomReverbBank&&) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `RoomReverbBank<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `RoomReverbBank<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `RoomReverbBank::prepare` {#symbol-roomreverbbank-prepare}

```cpp
Status prepare(const ProcessSpec&, std::size_t rooms = 8, const FdnConfig& = {}, CpuBackend = CpuBackend::automatic) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `rooms` | `std::size_t` | `8` | Time in milliseconds. |
| `argument 3` | `const FdnConfig &` | `{}` | Value/configuration in the declared type; apply the linked module contract. |
| `argument 4` | `CpuBackend` | `CpuBackend::automatic` | Compiled CPU/OS-supported kernel choice; automatic selects during preparation. |

Return: `Status` — Check Status; ok reports success.

### `RoomReverbBank::setRoom` {#symbol-roomreverbbank-setroom}

```cpp
Status setRoom(std::size_t, const FdnParameters&, std::size_t rampFrames = 256) noexcept;
```

Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `std::size_t` | — | Value/configuration in the declared type; apply the linked module contract. |
| `argument 2` | `const FdnParameters &` | — | Value/configuration in the declared type; apply the linked module contract. |
| `rampFrames` | `std::size_t` | `256` | Frame count or offset; one frame contains one sample per channel. |

Return: `Status` — Check Status; ok reports success.

### `RoomReverbBank::beginBlock` {#symbol-roomreverbbank-beginblock}

```cpp
Status beginBlock(std::size_t frames) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `frames` | `std::size_t` | — | Frame count or offset; one frame contains one sample per channel. |

Return: `Status` — Check Status; ok reports success.

### `RoomReverbBank::send` {#symbol-roomreverbbank-send}

```cpp
Status send(std::size_t room, AudioBlock<const T>, double gain = 1) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `room` | `std::size_t` | — | Value/configuration in the declared type; apply the linked module contract. |
| `argument 2` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `gain` | `double` | `1` | Linear amplitude multiplier; follow the processor bounds and mixing law. |

Return: `Status` — Check Status; ok reports success.

### `RoomReverbBank::render` {#symbol-roomreverbbank-render}

```cpp
Status render() noexcept;
```

Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.

Return: `Status` — Check Status; ok reports success.

### `RoomReverbBank::mix` {#symbol-roomreverbbank-mix}

```cpp
Status mix(AudioBlock<T>) const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

### `RoomReverbBank::roomOutput` {#symbol-roomreverbbank-roomoutput}

```cpp
AudioBlock<const T> roomOutput(std::size_t room) const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `room` | `std::size_t` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `AudioBlock<const T>` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `RoomReverbBank::reset` {#symbol-roomreverbbank-reset}

```cpp
void reset() noexcept;
```

Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.
