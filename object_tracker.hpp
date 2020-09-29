#pragma once
#include "ct-dynamic-map.hpp"

namespace compile_time::allocator2 {
template <typename T> struct object_tracker {
  dynamic_list<T *> *added{nullptr};
  dynamic_map<T *, bool> removed;
  dynamic_list<T *> *remaining{nullptr};
  constexpr void track(T *t) { added = new dynamic_list<T *>(t, added); }
  constexpr void untrack(T *t) { removed.insert(t, true); }

  constexpr typename dynamic_list<T *>::iterator begin() {
    if (remaining)
      delete remaining;
    remaining = nullptr;
    if (added) {
      for (auto *ptr : *added) {
        if (!(removed.contains(ptr) && removed.at(ptr))) {
          remaining = new dynamic_list<T *>(ptr, remaining);
        }
      }
    }
    if (remaining)
      return remaining->begin();
    else
      return dynamic_list<T *>{}.end();
  }

  constexpr auto end() {
    if (remaining)
      return remaining->end();
    else
      return dynamic_list<T *>{}.end();
  }

  constexpr ~object_tracker() {
    if (added)
      delete added;
    if (remaining)
      delete remaining;
  }
}; // namespace compile_time::allocator3
} // namespace compile_time::allocator2