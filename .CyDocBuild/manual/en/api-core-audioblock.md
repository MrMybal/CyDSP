---
id: api-core-audioblock
title: AudioBlock
description: Borrowed planar views, sub-blocks and shape/overlap validation.
section: API reference
order: 1110
revision: 1
parent: api-core
symbol: cydsp::AudioBlock
---

Borrowed planar views, sub-blocks and shape/overlap validation.

## Include {#include}

```cpp
#include <cydsp/core/AudioBlock.h>
```

[Usage and contracts](doc:buffers) · [Common processing rules](doc:contracts)

## `AudioBlock` {#symbol-audioblock}

```cpp
template <Sample T> class AudioBlock;
```

### `AudioBlock::AudioBlock<T>` {#symbol-audioblock-audioblock-t}

```cpp
constexpr AudioBlock() noexcept = default;
```

Construct an instance; prepare it before processing when the API requires preparation.

### `AudioBlock::AudioBlock<T>` {#symbol-audioblock-audioblock-t-2}

```cpp
constexpr AudioBlock(T* const* data, std::size_t channels, std::size_t frames) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `data` | `T *const *` | — | Value/configuration in the declared type; apply the linked module contract. |
| `channels` | `std::size_t` | — | Channel index/count in the declared planar layout. |
| `frames` | `std::size_t` | — | Frame count or offset; one frame contains one sample per channel. |

### `AudioBlock::AudioBlock<T>` {#symbol-audioblock-audioblock-t-3}

```cpp
template <Sample U> requires(std::is_const_v<T> && std::same_as<std::remove_const_t<T>, U>) constexpr AudioBlock(const AudioBlock<U>& other) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `other` | `const AudioBlock<U> &` | — | Borrowed planar audio view; its sample constness determines writable access. |

### `AudioBlock::channels` {#symbol-audioblock-channels}

```cpp
constexpr std::size_t channels() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `AudioBlock::frames` {#symbol-audioblock-frames}

```cpp
constexpr std::size_t frames() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `AudioBlock::frameOffset` {#symbol-audioblock-frameoffset}

```cpp
constexpr std::size_t frameOffset() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `AudioBlock::subBlock` {#symbol-audioblock-subblock}

```cpp
constexpr AudioBlock subBlock(std::size_t offset, std::size_t frames) const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `offset` | `std::size_t` | — | Value/configuration in the declared type; apply the linked module contract. |
| `frames` | `std::size_t` | — | Frame count or offset; one frame contains one sample per channel. |

Return: `AudioBlock<T>` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `AudioBlock::channelData` {#symbol-audioblock-channeldata}

```cpp
constexpr T* const* channelData() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `T *const *` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `AudioBlock::isValid` {#symbol-audioblock-isvalid}

```cpp
constexpr bool isValid() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `bool` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `AudioBlock::channel` {#symbol-audioblock-channel}

```cpp
constexpr std::span<T> channel(std::size_t index) const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `index` | `std::size_t` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `std::span<T>` — Returned value/view; respect borrowed lifetime and the units named by this API.

## `validateProcessBlocks` {#symbol-validateprocessblocks}

```cpp
template <Sample T> requires(!std::is_const_v<T>) [[nodiscard]] Status validateProcessBlocks(const ProcessSpec&, AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const ProcessSpec &` | — | Prepared rate, maximum block size and channel count. |
| `argument 2` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 3` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.

## `validateDisjointBlocks` {#symbol-validatedisjointblocks}

```cpp
template <Sample T> [[nodiscard]] Status validateDisjointBlocks(AudioBlock<const T>, AudioBlock<T>) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `AudioBlock<const T>` | — | Borrowed read-only planar audio; keep samples and pointer table alive. |
| `argument 2` | `AudioBlock<T>` | — | Borrowed planar audio view; its sample constness determines writable access. |

Return: `Status` — Check Status; ok reports success.
