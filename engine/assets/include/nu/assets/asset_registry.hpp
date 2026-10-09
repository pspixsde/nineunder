#pragma once

#include "nu/core/types.hpp"

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace nu {

template <typename Resource, typename HandleType>
class AssetRegistry {
public:
    HandleType add(Resource resource, std::string name = {}) {
        u32 index = 0;
        if (!freeList_.empty()) {
            index = freeList_.back();
            freeList_.pop_back();
            slots_[index].resource = std::move(resource);
            slots_[index].alive = true;
            ++slots_[index].generation;
        } else {
            index = static_cast<u32>(slots_.size());
            Slot slot;
            slot.resource = std::move(resource);
            slot.generation = 1;
            slot.alive = true;
            slots_.push_back(std::move(slot));
        }
        HandleType handle{};
        handle.index = index;
        handle.generation = slots_[index].generation;
        if (!name.empty()) {
            names_[std::move(name)] = handle;
        }
        return handle;
    }

    [[nodiscard]] Resource* get(HandleType handle) {
        if (!handle || handle.index >= slots_.size()) {
            return nullptr;
        }
        Slot& slot = slots_[handle.index];
        if (!slot.alive || slot.generation != handle.generation) {
            return nullptr;
        }
        return &slot.resource;
    }

    [[nodiscard]] std::optional<HandleType> find(const std::string& name) const {
        auto it = names_.find(name);
        if (it == names_.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    void remove(HandleType handle) {
        Resource* res = get(handle);
        if (!res) {
            return;
        }
        slots_[handle.index].alive = false;
        freeList_.push_back(handle.index);
    }

    template <typename Fn>
    void forEach(Fn&& fn) {
        for (u32 i = 0; i < slots_.size(); ++i) {
            if (slots_[i].alive) {
                HandleType h{};
                h.index = i;
                h.generation = slots_[i].generation;
                fn(h, slots_[i].resource);
            }
        }
    }

private:
    struct Slot {
        Resource resource{};
        u32 generation = 0;
        bool alive = false;
    };

    std::vector<Slot> slots_;
    std::vector<u32> freeList_;
    std::unordered_map<std::string, HandleType> names_;
};

} // namespace nu
