---
id: api-core-spscqueue
title: SpscQueue
description: Fixed-capacity single-producer/single-consumer communication with retained storage.
section: API reference
order: 1116
revision: 1
parent: api-core
symbol: cydsp::SpscQueue
---

Fixed-capacity single-producer/single-consumer communication with retained storage.

## Include {#include}

```cpp
#include <cydsp/core/SpscQueue.h>
```

[Usage and contracts](doc:threading) · [Common processing rules](doc:contracts)

## `SpscQueue` {#symbol-spscqueue}

```cpp
template <class T> requires(std::is_trivially_copyable_v<T> && std::is_default_constructible_v<T>) class SpscQueue;
```

### `SpscQueue::prepare` {#symbol-spscqueue-prepare}

```cpp
Status prepare(std::size_t capacity) noexcept;
```

Prepare outside the audio callback; check Status before using the new configuration.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `capacity` | `std::size_t` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `Status` — Check Status; ok reports success.

### `SpscQueue::push` {#symbol-spscqueue-push}

```cpp
bool push(const T& value) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `value` | `const T &` | — | Value/configuration in the declared type; apply the linked module contract. |

Return: `bool` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `SpscQueue::peek` {#symbol-spscqueue-peek}

```cpp
const T* peek() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `const T *` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `SpscQueue::discard` {#symbol-spscqueue-discard}

```cpp
bool discard() noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `bool` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `SpscQueue::pop` {#symbol-spscqueue-pop}

```cpp
bool pop(T& value) noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `value` | `T &` | — | Mutable reference; inspect the declaration and guide for output/state ownership. |

Return: `bool` — Returned value/view; respect borrowed lifetime and the units named by this API.

### `SpscQueue::capacity` {#symbol-spscqueue-capacity}

```cpp
std::size_t capacity() const noexcept;
```

Use the declared inputs and returned type with the module contract below; getters do not synchronize access.

Return: `std::size_t` — Returned value/view; respect borrowed lifetime and the units named by this API.
