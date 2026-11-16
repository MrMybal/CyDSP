#include "sofa/SofaDetail.h"
#include <algorithm>
#include <climits>
#include <csetjmp>
#include <cstdlib>
#include <cstring>
#include <memory>
extern "C" {
#include "compact/Memory.h"
#include "compact/Prefix.h"
#include "compact/hrtf/mysofa.h"
#include "miniz.h"
}
namespace {
struct alignas(std::max_align_t) Block {
    Block *previous, *next;
    std::size_t size;
};
struct Context {
    std::jmp_buf jump;
    Block* head = nullptr;
    std::size_t live = 0, remaining = 0, steps = 0;
    unsigned depth = 0;
    const cydsp::SofaLoadOptions* options = nullptr;
    ~Context() {
        while (head) {
            auto* next = head->next;
            std::free(head);
            head = next;
        }
    }
};
thread_local Context* current = nullptr;
struct Current {
    Context* previous;
    explicit Current(Context* c) : previous(current) {
        current = c;
    }
    ~Current() {
        current = previous;
    }
};
} // namespace
extern "C" {
void cydsp_sofa_abort(int reason) {
    std::longjmp(current->jump, reason);
}
void cydsp_sofa_step(std::size_t n) {
    if (n > current->steps)
        cydsp_sofa_abort(1);
    current->steps -= n;
}
void cydsp_sofa_enter() {
    if (++current->depth > 64)
        cydsp_sofa_abort(1);
}
void cydsp_sofa_leave() {
    --current->depth;
}
void* cydsp_sofa_malloc(std::size_t n) {
    if (!n)
        n = 1;
    cydsp_sofa_step(1);
    if (n > current->remaining || n > SIZE_MAX - sizeof(Block))
        cydsp_sofa_abort(1);
    auto* b = static_cast<Block*>(std::malloc(n + sizeof(Block)));
    if (!b)
        cydsp_sofa_abort(3);
    b->size = n;
    b->previous = nullptr;
    b->next = current->head;
    if (b->next)
        b->next->previous = b;
    current->head = b;
    current->live += n;
    current->remaining -= n;
    return b + 1;
}
void cydsp_sofa_free(void* p) {
    if (!p)
        return;
    auto* b = static_cast<Block*>(p) - 1;
    if (b->previous)
        b->previous->next = b->next;
    else
        current->head = b->next;
    if (b->next)
        b->next->previous = b->previous;
    current->live -= b->size;
    current->remaining += b->size;
    std::free(b);
}
void* cydsp_sofa_calloc(std::size_t n, std::size_t size) {
    if (n && size > SIZE_MAX / n)
        cydsp_sofa_abort(1);
    auto* p = cydsp_sofa_malloc(n * size);
    std::memset(p, 0, n * size);
    return p;
}
void* cydsp_sofa_realloc(void* p, std::size_t n) {
    if (!p)
        return cydsp_sofa_malloc(n);
    auto* b = static_cast<Block*>(p) - 1;
    auto* next = cydsp_sofa_malloc(n);
    std::memcpy(next, p, std::min(n, b->size));
    cydsp_sofa_free(p);
    return next;
}
int cydsp_sofa_gunzip(int inlen, char* in, int* outlen, char* out) {
    if (inlen < 0 || !outlen || *outlen < 0)
        return -1;
    const auto expected = std::size_t(*outlen);
    if (current->options->inflate) {
        const auto status = current->options->inflate(
            {reinterpret_cast<const std::byte*>(in), std::size_t(inlen)},
            {reinterpret_cast<std::byte*>(out), expected}, current->options->inflateContext);
        return status == cydsp::Status::ok ? 0 : -1;
    }
    tinfl_decompressor decoder{};
    tinfl_init(&decoder);
    std::size_t input = std::size_t(inlen), output = expected;
    const auto status = tinfl_decompress(
        &decoder, reinterpret_cast<const mz_uint8*>(in), &input, reinterpret_cast<mz_uint8*>(out),
        reinterpret_cast<mz_uint8*>(out), &output,
        TINFL_FLAG_PARSE_ZLIB_HEADER | TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF);
    return status == TINFL_STATUS_DONE && input == std::size_t(inlen) && output == expected ? 0
                                                                                            : -1;
}
}
namespace cydsp::sofa_detail {
namespace {
std::string attr(const MYSOFA_ATTRIBUTE* a, const char* name, Budget& budget) {
    for (; a; a = a->next)
        if (a->name && a->value && std::strcmp(a->name, name) == 0) {
            const auto n = std::strlen(a->value);
            budget.consume(n);
            return {a->value, n};
        }
    return {};
}
void copy(Raw& raw, const char* name, const MYSOFA_ARRAY& a, std::vector<std::size_t> shape,
          Budget& budget, Budget& metadata) {
    if (!a.elements)
        return;
    std::size_t count = 1;
    for (auto d : shape)
        count = product(count, d, budget.remaining / sizeof(double));
    if (count != a.elements || !a.values)
        fail(SofaError::invalid_dimensions);
    budget.consume(product(count, sizeof(double), budget.remaining));
    Array result;
    result.shape = std::move(shape);
    result.precision = a.source_bits;
    result.type = attr(a.attributes, "Type", metadata);
    result.units = attr(a.attributes, "Units", metadata);
    result.values.assign(a.values, a.values + a.elements);
    raw.reduced = raw.reduced || a.source_bits > 32;
    raw.arrays.emplace(name, std::move(result));
}
} // namespace
Raw readCompact(std::span<const std::byte> bytes, const SofaLoadOptions& o) {
    if (bytes.size() > std::size_t(LONG_MAX))
        fail(SofaError::limit_exceeded);
    auto context = std::make_unique<Context>();
    context->remaining = o.maximumDecodedBytes;
    context->options = &o;
    context->steps = product(bytes.size() + 1, 128, SIZE_MAX);
    Current scope(context.get());
    // longjmp crosses C parser frames only. Owning C++ objects precede setjmp.
    MYSOFA_HRTF* h = nullptr;
    int error = 0;
    const int aborted = setjmp(context->jump);
    if (aborted)
        fail(aborted == 1   ? SofaError::limit_exceeded
             : aborted == 3 ? SofaError::allocation_failed
                            : SofaError::invalid_file);
    h = cydsp_sofa_mysofa_load_data(reinterpret_cast<const char*>(bytes.data()), bytes.size(),
                                    &error);
    if (!h || error)
        fail(error == MYSOFA_UNSUPPORTED_FORMAT ? SofaError::unsupported_storage
                                                : SofaError::invalid_file);
    if (h->R != 2 || h->E != 1 || !h->M || !h->N)
        fail(SofaError::invalid_dimensions);
    if (h->M > o.maximumPoints || h->N > o.maximumTaps)
        fail(SofaError::limit_exceeded);
    Raw raw;
    Budget budget{o.maximumDecodedBytes}, metadata{o.maximumMetadataBytes};
    for (const auto* name : {"Conventions", "SOFAConventions", "DataType", "Title", "DatabaseName",
                             "ListenerShortName", "License"})
        raw.attributes[name] = attr(h->attributes, name, metadata);
    const std::size_t m = h->M, n = h->N;
    copy(raw, "Data.IR", h->DataIR, {m, 2, n}, budget, metadata);
    copy(raw, "Data.SamplingRate", h->DataSamplingRate, {h->DataSamplingRate.elements}, budget,
         metadata);
    copy(raw, "Data.Delay", h->DataDelay, {h->DataDelay.elements / 2, 2}, budget, metadata);
    copy(raw, "SourcePosition", h->SourcePosition, {h->SourcePosition.elements / 3, 3}, budget,
         metadata);
    copy(raw, "ReceiverPosition", h->ReceiverPosition, {2, 3, h->ReceiverPosition.elements / 6},
         budget, metadata);
    copy(raw, "EmitterPosition", h->EmitterPosition, {1, 3, h->EmitterPosition.elements / 3},
         budget, metadata);
    copy(raw, "ListenerPosition", h->ListenerPosition, {h->ListenerPosition.elements / 3, 3},
         budget, metadata);
    copy(raw, "ListenerView", h->ListenerView, {h->ListenerView.elements / 3, 3}, budget, metadata);
    copy(raw, "ListenerUp", h->ListenerUp, {h->ListenerUp.elements / 3, 3}, budget, metadata);
    return raw;
}
} // namespace cydsp::sofa_detail
