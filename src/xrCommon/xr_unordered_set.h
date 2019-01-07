#pragma once
#include <unordered_set>
#include "xrCore/xrMemory.h"

template <typename K, class Hasher = std::hash<K>, class Traits = std::equal_to<K>,
    typename allocator = xr_allocator<K>>
using xr_unordered_set = std::unordered_set<K, Hasher, Traits, allocator>;

template <typename K, class Hasher = std::hash<K>, class Traits = std::equal_to<K>,
    typename allocator = xr_allocator<K>>
using xr_unordered_multiset = std::unordered_multiset<K, Hasher, Traits, allocator>;
