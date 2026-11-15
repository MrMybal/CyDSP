---
id: api-core-audiobuffer
title: AudioBuffer
description: Owning zero-initialized, 64-byte-aligned planar storage prepared outside the callback.
section: API reference
order: 1111
revision: 1
parent: api-core
symbol: cydsp::AudioBuffer
---

Owning zero-initialized, 64-byte-aligned planar storage prepared outside the callback.

## Include {#include}

```cpp
#include <cydsp/core/AudioBuffer.h>
```

[Usage and contracts](doc:buffers) · [Common processing rules](doc:contracts)

## `AudioBuffer` {#symbol-audiobuffer}

```cpp
template <Sample T> requires(!std::is_const_v<T>) class AudioBuffer;
```

### `AudioBuffer::AudioBuffer<T>` {#symbol-audiobuffer-audiobuffer-t}

```cpp
AudioBuffer() = default;
```

Construct an instance; prepare it before processing when the API requires preparation.

### `AudioBuffer::AudioBuffer<T>` {#symbol-audiobuffer-audiobuffer-t-2}

```cpp
AudioBuffer(const AudioBuffer&) = delete;
```

This operation is explicitly deleted; do not copy this owning object.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const AudioBuffer<T> &` | — | Value/configuration in the declared type; apply the linked module contract. |

### `AudioBuffer::operator=` {#symbol-audiobuffer-operator}

```cpp
AudioBuffer& operator=(const AudioBuffer&) = delete;
```

This operation is explicitly deleted; do not copy this owning object.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `argument 1` | `const AudioBuffer<T> &` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `AudioBuffer<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `AudioBuffer::AudioBuffer<T>` {#symbol-audiobuffer-audiobuffer-t-3}

```cpp
AudioBuffer(AudioBuffer&& other) noexcept;
```

Construct an instance; prepare it before processing when the API requires preparation.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `other` | `AudioBuffer<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `AudioBuffer::operator=` {#symbol-audiobuffer-operator-2}

```cpp
AudioBuffer& operator=(AudioBuffer&& other) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `other` | `AudioBuffer<T> &&` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `AudioBuffer<T> &` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `AudioBuffer::resize` {#symbol-audiobuffer-resize}

```cpp
Status resize(std::size_t channels, std::size_t frames) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `channels` | `std::size_t` | — | Channel index/count in the declared planar layout. |
| `frames` | `std::size_t` | — | Frame count or offset; one frame contains one sample per channel. |

Return: `Status` — Check Status; ok reports success.

### `AudioBuffer::swap` {#symbol-audiobuffer-swap}

```cpp
void swap(AudioBuffer& other) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `other` | `AudioBuffer<T> &` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

### `AudioBuffer::block` {#symbol-audiobuffer-block}

```cpp
AudioBlock<T> block() noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `AudioBlock<T>` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `AudioBuffer::block` {#symbol-audiobuffer-block-2}

```cpp
AudioBlock<const T> block() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `AudioBlock<const T>` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `AudioBuffer::block` {#symbol-audiobuffer-block-3}

```cpp
AudioBlock<T> block(std::size_t frames) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `frames` | `std::size_t` | — | Frame count or offset; one frame contains one sample per channel. |

Return: `AudioBlock<T>` — Returned value/view; respect borrowed lifetime and the units named by this API.
