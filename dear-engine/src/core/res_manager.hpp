#pragma once

#include <string>
#include <optional>
#include <unordered_map>

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Resource Manager

/// Base class for getting and loading any type of resouce
template<typename Key, typename Element>
class ResManager {
 public:
  /// Retrieve a resource from cache
  auto get(const Key& key) -> std::optional<Element> {
    auto it = map.find(key);
    if (it != map.end()) return it->second;
    else return std::nullopt;
  }

 protected:
  /// Load a resource into the cache
  template<typename E>
  auto load(const Key &key, E&& element) -> std::optional<Element> {
    return map.insert_or_assign(key, std::forward<E>(element)).first->second;
  }

 protected:
  std::unordered_map<Key, Element> map;
};

