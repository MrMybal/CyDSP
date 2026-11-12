# MIDI-CI Property Exchange

The portable `CyDSP::CyDSP` core supplies an original MIT responder
(`CiPropertyStore`), initiator/cache (`CiPropertyClient`) and bounded JSON workspace
(`CiPropertyPatch`). They compose the existing `MidiCiDevice`, UMP framing and
native transport queues. No new dependency or JUCE source is used.

## Responder resources

| Resource | Behavior |
| --- | --- |
| ResourceList | Generated capabilities, media/encodings, schema references, requireResId, SET and pagination flags; excludes itself |
| DeviceInfo | Discovery identity bytes plus supplied nonempty manufacturer/family/model/version and optional serial; UTF-8 names become escaped ASCII JSON |
| ChannelList | Typed CiChannelDescription records, optional bank/program and cluster information; locally replaced with publishChannels and subscribed through notify |
| JSONSchema | GET with resId resolves a generated midi+jsonschema://schemaN reference to its prepared schema |
| Custom X- resources | Owned bounded JSON or decoded binary data; optional instances, writes, pagination and subscriptions |

Channel numbers are **1..256 relative to the function block**, spanning MIDI groups.
The host supplies that mapping. Bank/program bytes are seven-bit. Channels must be
unique; title is nonempty. Cluster ranges must fit 1..256, mode 1..4 and type
`other`, `profile` or `mpe1`. Profile-specific instrument behavior remains host policy.

Custom resource names are unique, at most 36 ASCII characters, and start `X-`.
By default a definition has one record. A nonempty `instances` vector instead
requires an explicit `resId` for every remote request and local data/publication
operation. Instance IDs have 1..36 letters, digits or underscores. Base `data`
must then be empty; records and subscription revisions remain independent.
Instances are fixed during preparation, with at most 256 custom records in total.

GET supports ASCII JSON and Mcoded7; binary advertises Mcoded7 only. Media must
match. Writable resources support full replacement. `partial=true` additionally
advertises partial SET: the body maps existing **RFC6901 pointers** to replacement
strings, numbers or booleans. For example, `{"/gain":0.5,"/voices/0/name":"Bass"}`
updates existing members of the current document. This is the MIDI-CI pointer-map
format. Paths decode `~0`/`~1` and JSON Unicode escapes. Missing targets, duplicate
decoded keys, overlapping ancestor/child changes, null and container replacements
are rejected. An empty pointer replaces the root with a primitive.

The final document is checked for size, JSON syntax and the host's validation
callback before ACK queueing and commit. Failure preserves the old value.
`CiPropertyPatch` exposes the same prepared implementation for other control tasks;
its output must be disjoint from both inputs, and errors/undersized output produce
no partial write. Parsing/rendering normalizes container whitespace.

Paginated resources must be JSON arrays of objects with names ending `List`.
GET takes both `offset` (zero based) and `limit` (at least one), or neither for the
whole list. Replies always include `totalCount` for paginated resources.
ResourceList and ChannelList support pagination automatically. Out-of-range
offsets return `[]`, retaining the total. Instances may also contain lists.

Schemas are copied at preparation. Original minimal schemas describe built-ins;
the JSONSchema resource serves supplied custom schemas without fetching URLs.
Writable resources advertising a nontrivial schema require the host's `validate`
callback. A general JSON Schema evaluator and automatic preset/DSP mutation are
application policy.

## Subscriptions and retained replies

Subscriptions use start/end commands and unique eight-character IDs. Each custom
definition chooses `CiSubscriptionDelivery::notify`, `full` or `partial`.
Notify prompts a GET; full carries a snapshot; partial carries the last accepted
partial SET when it follows the subscriber's acknowledged revision immediately.
Local publication, full SET or multiple coalesced edits fall back to full delivery.
Each update requires a 0x39 acknowledgement. Multiple changes coalesce while an
update is outstanding. Failed/expired acknowledgements end the local subscription.

The store prepares a FIFO of reply buffers. A SET queues its acknowledgement before
committing. Core/native queue pressure retains accepted work until `poll` can
forward it. If the store's own FIFO is full, `process` returns `update_queue_full`
without changing a value. A host retry must retain the complete assembled request;
replaying only its last wire chunk is invalid.

Status replies include 400 malformed, 403 denied, 404 missing, 405 unsupported
write/subscription, 413 oversized, 415 unsupported media/encoding and 343 exhausted
subscriptions. Capacity includes the core's wire payload limit: Mcoded7 needs extra
space, and an oversized encoded GET returns 413. Acceptance means queued work,
without guaranteeing physical delivery.

## Initiator and cache

Prepare `CiPropertyClient` after its borrowed MidiCiDevice. `CiPropertyQuery`
identifies a peer, resource, optional instance, requested media and optional page.
`fetch(query, nowMs)` uses a fresh entry or sends GET; `force=true` bypasses its TTL.
Duplicate in-flight GETs return `transition_in_progress`. Callbacks report decoded
updates, request completion/status and subscription activation/deactivation.

The key includes peer/resource/resId/requested media/page; transfer encoding is
independent of the decoded value. `cached` returns a borrowed optional view with
media, optional total count, expiry and freshness. By default it hides stale data;
`allowStale=true` exposes the retained previous value explicitly. `cacheTime` is
converted from seconds to a saturating millisecond expiry; zero expires immediately.
An active acknowledged subscription keeps a received baseline fresh until an
update invalidates it. Pages cannot be subscribed or SET.

`subscribe` automatically GETs an initial baseline. Notify invalidates and coalesces
a refresh; full/partial updates decode and update the cache before the update
callback. A generation counter prevents an older in-flight GET from overwriting
a newer subscription update or local SET invalidation. A pending local SET leaves
the prior baseline available for a subsequent valid partial update. A partial
received without a safe baseline, or a malformed/unsupported update, receives a
negative ACK, deactivates the local subscription and schedules GET recovery.
The host may explicitly resubscribe after recovery. Full pushes without totalCount
clear any previously cached total rather than retaining an obsolete count.

The client retains a bounded ACK FIFO. Saturation returns `update_queue_full`
before changing the cache; retain the complete message if retrying. Pending and
subscribed entries cannot be evicted. Other entries use bounded least-recently-used
replacement; exhausted entries/transactions return `update_queue_full`.
`poll` flushes ACKs, advances the core, handles request timeout and schedules
coalesced refreshes. Use a monotonic caller-owned millisecond clock.

## Core cancellation and ownership

PE Notify 100 extends pending/reassembly deadlines. Notify 144 cancels matching
work; 408 reports timeout. Header-only Notify bypasses occupied assembly slots.
`notifyProperty` cancels locally immediately for 144/408; its return status
separately reports acceptance of the wire notification. Already accepted I/O bytes
cannot be recalled. A host may retry a failed wire notification.

Use one serialized control owner for CI/store/client APIs, including borrowed
reads and callbacks. Device addresses and callback contexts must remain stable;
the core must outlive both helpers. After re-preparing CI, re-prepare the helpers.
Callbacks must not reenter their owners or perform lifecycle operations.
Route `CiCallbacks::invalidated` into each associated helper. Peer invalidation
clears matching work/cache; local MUID replacement clears sessions on the next
process/poll (cached reads already reject the old session).

The [responder example](../examples/Midi2Session.cpp) and
[initiator example](../examples/Midi2ClientSession.cpp) show separate roles. When
combining roles on one core, the host must route subscription start/end inquiries
to its store, responder updates to its client, and replies/Notify to their matching
transaction owner. An end command's direction needs the host's subscription state;
its command text alone cannot distinguish the two roles.

Preparation, moves and destruction may allocate/free. Prepared processing,
publication, polling, invalidation, cache reads and reset retain memory; they do
not lock or perform I/O themselves. Send callbacks must copy/queue into the native
control-producer queue, and preserve those contracts. Never feed a synchronous
recursive peer call from send. Changed callbacks can publish prepared commands
through the existing audio SPSC; they must not mutate a DSP instance concurrently.

Store options bound subscriptions/reply slots (at most 128 each); up to 128 custom
definitions are accepted. Client entries are bounded to 256, transactions and
subscriptions to 128, and ACK slots to 256. Patch/list workspaces default to 4096
combined document/change nodes and 1024 decoded pointer/key bytes, each configurable
up to 65536. JSON depth is bounded to 32. Small workspaces reject complex documents
with an explicit status.

`data(name, resId)` and `cached` borrow storage until the next relevant mutation,
eviction, reset, move or reprepare. Store `publish` accepts disjoint decoded input,
including local changes to remotely read-only resources. Store reset retains
resource values; client reset clears cached values. Both abandon pending work
locally. Destruction/reset sends no wire unsubscribe; use explicit unsubscribe
before ending a live subscription. Reset the core when ending the whole session.

## JSON utilities and evidence

`CiPropertyHeader` validates flat PE headers up to 4096 bytes, 64 distinct camelCase
ASCII keys of at most 20 characters, first-key rules and primitive values. It rejects
nested values, null, duplicate decoded keys and exterior whitespace. Failed parse
preserves its previous view. Value tokens borrow the input.

`validateCiPropertyJson` checks complete ASCII wire JSON numbers, escapes and
surrogate pairs; opaque property data may contain duplicate keys. Patch/list
workspaces reject ambiguous duplicate decoded object keys. `ciPropertyJsonString`
converts valid UTF-8 to quoted escaped ASCII JSON, transactionally.

Independent regressions cover Unicode/RFC6901 vectors, atomic rejection, capacity,
aliasing, typed ChannelList, schema lookup, instance isolation, pagination totals,
partial SET validation, full/partial fallback, TTL boundaries, binary cache values,
stale GET races, notification refresh, timeout, eviction/ACK pressure and zero
allocations/frees through prepared request/patch/subscription processing.
[Validation](Validation.md) records actual builds/runs. macOS/Linux desktop tests
are deferred by the owner; physical MIDI2, Apple compilation, SVE hardware and
remaining platform limits are recorded in [MIDI 2](Midi2.md) and [Coverage](Coverage.md).
Zlib and profile-specific instrument/preset semantics remain outside this helper.
No full MIDI-CI conformance certification is claimed.

Wire contracts follow the [Common Property Exchange Rules](https://amei.or.jp/midistandardcommittee/MIDI2.0/MIDI2.0-DOCS/M2-103-UM_v1-1_Common_Rules_for_MIDI-CI_Property_Exchange.pdf)
and [Foundational Resources](https://amei-music.github.io/midi2.0-docs/amei-pdf/M2-105-UM_v1-1-1_Property_Exchange_Foundational_Resources.pdf).
No specification PDF or third-party implementation is bundled.
