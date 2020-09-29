#pragma once
#include "dynamic-list.hpp"
#include <cassert>
#include <utility>

namespace compile_time {

template <typename K, typename V> struct dynamic_map {
  struct pair {
    K key{};
    V value{};
    constexpr pair() = default;
    constexpr pair(K k, V v) : key(std::move(k)), value(std::move(v)) {}
  };

  struct key_iterator {
    struct index {
      dynamic_list<pair> *elems{nullptr};

      constexpr K &operator*() { return elems->payload->key; }
      constexpr index &operator++() {
        elems = elems->next;
        return *this;
      }
      constexpr bool operator==(const index &i) const { return elems == i.elems; }
      constexpr bool operator!=(const index &i) const { return !((*this) == i); }

      constexpr index(dynamic_list<pair> *elems) : elems(elems) {}
    };

    dynamic_list<pair> *elems{nullptr};
    constexpr key_iterator(dynamic_list<pair> *elems) : elems(elems) {}
    constexpr index begin() { return index{elems}; }
    constexpr index end() { return index{nullptr}; }
  };

  dynamic_list<pair> *elems{nullptr};

  constexpr dynamic_map() = default;

  constexpr bool contains(const K &target) const {
    for (auto *curr = elems; curr; curr = curr->next)
      if (curr->payload->key == target)
        return true;
    return false;
  }

  constexpr V &at(const K &k) {
    for (auto *curr = elems; curr; curr = curr->next) {
      if (curr->payload->key == k)
        return curr->payload->value;
    }
    assert(false && "Error: ran off end of list");
    return *new V();
  }

  constexpr void insert(K k, V v) { elems = new dynamic_list<pair>(new pair(k, v), elems); }
  constexpr key_iterator keys() { return elems; }
  constexpr ~dynamic_map() { delete elems; }
};

} // namespace compile_time
