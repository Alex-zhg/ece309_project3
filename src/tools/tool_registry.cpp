// src/tools/tool_registry.cpp — YOU IMPLEMENT (spec §4.3).
//
// A hand-written hash table. No std::unordered_map / std::map / std::set.
// Every function below is a stub so the starter compiles. Replace each body.

#include "tools/tool_registry.h"
#include <cstdint>
#include <utility>


ToolRegistry::ToolRegistry() : buckets_(kInitialBuckets) {}

ToolRegistry::~ToolRegistry() = default; // unique_ptrs in your buckets free the tools

std::size_t ToolRegistry::hash(std::string_view key) noexcept {
    std::uint64_t h = 1469598103934665603ULL; // offset basis
    for (unsigned char c : key) {
        h ^= c;
        h *= 1099511628211ULL; // FNV prime
    }
    return static_cast<std::size_t>(h);}

bool ToolRegistry::add(std::unique_ptr<Tool> tool) {
    if (!tool) return false;

    std::string name = tool->name();
    const std::size_t h = hash(name);

    for (const Entry& e : buckets_[index_for(h, buckets_.size())])

        if (e.hash == h && e.name == name) return false;
    if (size_ + 1 > buckets_.size()) grow();

    buckets_[index_for(h, buckets_.size())].push_back(
        Entry{h, std::move(name), std::move(tool)});
    ++size_;
    return true;
}

Tool* ToolRegistry::find(std::string_view name) const {
    const std::size_t h = hash(name);
    for (const Entry& e : buckets_[index_for(h, buckets_.size())])
        if (e.hash == h && e.name == name) return e.tool.get();
    return nullptr;
}

std::size_t ToolRegistry::size() const noexcept { return size_; }


void ToolRegistry::for_each(const std::function<void(const Tool&)>& fn) const {
    for (const Bucket& b : buckets_)
        for (const Entry& e : b) fn(*e.tool);
}

std::size_t ToolRegistry::bucket_count() const noexcept { return buckets_.size(); }

double ToolRegistry::load_factor() const noexcept {
    return buckets_.empty() ? 0.0 : static_cast<double>(size_) / static_cast<double>(buckets_.size());
}

std::size_t ToolRegistry::max_probe_length() const {
    std::size_t longest = 0;
    for (const Bucket& b : buckets_)
        if (b.size() > longest) longest = b.size();
    return longest;
}

void ToolRegistry::grow() {
    const std::size_t new_count = buckets_.size() * 2;
    std::vector<Bucket> fresh(new_count);

    std::vector<std::size_t> counts(new_count, 0);
    for (const Bucket& b : buckets_)
        for (const Entry& e : b) ++counts[index_for(e.hash, new_count)];
    for (std::size_t i = 0; i < new_count; ++i) fresh[i].reserve(counts[i]);

    for (Bucket& b : buckets_)
        for (Entry& e : b) fresh[index_for(e.hash, new_count)].push_back(std::move(e));

    buckets_.swap(fresh);
}