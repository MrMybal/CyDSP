#include "cydsp/midi/CiProperty.h"
#include "detail/Alias.h"
#include "detail/CiJsonTree.h"
namespace cydsp {
struct CiPropertyPatch::State {
    detail::CiJsonTree tree;
};
CiPropertyPatch::CiPropertyPatch() = default;
CiPropertyPatch::~CiPropertyPatch() = default;
CiPropertyPatch::CiPropertyPatch(CiPropertyPatch&&) noexcept = default;
CiPropertyPatch& CiPropertyPatch::operator=(CiPropertyPatch&&) noexcept = default;
Status CiPropertyPatch::prepare(std::size_t nodes, std::size_t pointer) noexcept {
    try {
        auto next = std::make_unique<State>();
        const auto result = next->tree.prepare(nodes, pointer);
        if (result == Status::ok)
            state_.swap(next);
        return result;
    } catch (...) {
        return Status::allocation_failed;
    }
}
Status CiPropertyPatch::apply(std::span<const std::uint8_t> doc,
                              std::span<const std::uint8_t> patch, std::span<std::uint8_t> output,
                              std::size_t& written) noexcept {
    written = 0;
    if (!state_)
        return Status::not_prepared;
    auto& t = state_->tree;
    t.reset();
    std::size_t root, changes;
    auto result = t.parse(doc, root);
    if (result != Status::ok)
        return result;
    const auto oldCount = t.used;
    result = t.parse(patch, changes);
    if (result != Status::ok)
        return result;
    if (t.nodes[changes].kind != '{')
        return Status::invalid_argument;
    for (auto child = t.nodes[changes].child; child != detail::CiJsonTree::none;
         child = t.nodes[child].next) {
        const auto& change = t.nodes[child];
        if (change.kind == '{' || change.kind == '[' || change.kind == 'n')
            return Status::invalid_argument;
        const auto target = t.pointer(root, change.key);
        if (target == detail::CiJsonTree::none)
            return t.error == Status::ok ? Status::invalid_argument : t.error;
        if (t.nodes[target].replaced)
            return Status::invalid_argument;
        t.nodes[target].replacement = change.raw;
        t.nodes[target].replaced = true;
    }
    for (std::size_t i = 0; i < oldCount; ++i)
        if (t.nodes[i].replaced)
            for (auto parent = t.nodes[i].parent; parent != detail::CiJsonTree::none;
                 parent = t.nodes[parent].parent)
                if (t.nodes[parent].replaced)
                    return Status::invalid_argument;
    detail::CiJsonWriter sizing{{}};
    if (!t.render(root, sizing))
        return Status::invalid_argument;
    written = sizing.position;
    if (written > output.size())
        return Status::block_too_large;
    if (detail::overlaps(doc.data(), doc.size(), output.data(), written) ||
        detail::overlaps(patch.data(), patch.size(), output.data(), written))
        return Status::partial_overlap;
    detail::CiJsonWriter w{output};
    return t.render(root, w) && w.good ? Status::ok : Status::invalid_argument;
}
Status CiPropertyPatch::list(std::span<const std::uint8_t> doc, std::uint32_t offset,
                             std::uint32_t limit, std::span<std::uint8_t> output,
                             std::size_t& written, std::uint32_t& total) noexcept {
    written = 0;
    total = 0;
    if (!state_)
        return Status::not_prepared;
    if (!limit)
        return Status::invalid_argument;
    auto& t = state_->tree;
    t.reset();
    std::size_t root;
    const auto parsed = t.parse(doc, root);
    if (parsed != Status::ok)
        return parsed;
    if (t.nodes[root].kind != '[')
        return Status::invalid_argument;
    for (auto child = t.nodes[root].child; child != detail::CiJsonTree::none;
         child = t.nodes[child].next) {
        if (t.nodes[child].kind != '{')
            return Status::invalid_argument;
        ++total;
    }
    auto render = [&](detail::CiJsonWriter& w) {
        w.character('[');
        std::uint32_t position = 0, count = 0;
        bool comma = false;
        for (auto child = t.nodes[root].child; child != detail::CiJsonTree::none;
             child = t.nodes[child].next, ++position) {
            if (position < offset || count == limit)
                continue;
            if (comma)
                w.character(',');
            comma = true;
            ++count;
            if (!t.render(child, w))
                return false;
        }
        w.character(']');
        return true;
    };
    detail::CiJsonWriter sizing{{}};
    if (!render(sizing))
        return Status::invalid_argument;
    written = sizing.position;
    if (written > output.size())
        return Status::block_too_large;
    if (detail::overlaps(doc.data(), doc.size(), output.data(), written))
        return Status::partial_overlap;
    detail::CiJsonWriter w{output};
    return render(w) && w.good ? Status::ok : Status::invalid_argument;
}
} // namespace cydsp
