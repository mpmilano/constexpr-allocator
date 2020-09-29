#pragma once
#include <utility>

namespace compile_time {

template <typename T> struct dynamic_list {

  T *payload{nullptr};
  dynamic_list *next{nullptr};
  constexpr dynamic_list() = default;
  dynamic_list(const dynamic_list &) = delete;
  dynamic_list(dynamic_list &&) = delete;
  constexpr ~dynamic_list() {
    if (payload)
      delete payload;
    if (next)
      delete next;
  }
  constexpr dynamic_list(T *payload) : payload(payload) {}
  constexpr dynamic_list(dynamic_list *next) : next(next) {}
  constexpr dynamic_list(T *payload, dynamic_list *next) : payload(payload), next(next) {}
  constexpr dynamic_list(T payload, dynamic_list *next) : payload(new T(payload)), next(next) {}

  template <typename... Trest>
  constexpr dynamic_list(const T &t1, Trest &&... trest)
      : payload(new T(t1)), next(new dynamic_list(std::forward<Trest>(trest)...)) {}

  struct iterator {
    dynamic_list *curr;
    constexpr iterator() = default;
    constexpr iterator(dynamic_list *c) : curr(c) {}
    constexpr iterator &operator++() {
      if (curr)
        curr = curr->next;
      return *this;
    }
    constexpr bool operator==(const iterator &o) const { return curr == o.curr; }
    constexpr bool operator!=(const iterator &o) const { return !(*this == o); }
    constexpr T &operator*() { return *curr->payload; }
    constexpr T *operator->() { return &operator*(); }
  };

  constexpr iterator begin() { return (payload ? iterator{this} : end()); }
  constexpr iterator end() { return iterator{}; }
};

} // namespace compile_time
