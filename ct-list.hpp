#pragma once
#include "ct_ptr.hpp"

namespace compile_time::allocator2 {

namespace list_detail {
template <typename T, bool b = std::is_class_v<T>> struct list_node_payload;

template <typename T> struct list_node_payload<T, true> {
  static_assert(std::is_class_v<T>);
  using t = ct_ptr<T>;
  template <typename Alloc> static constexpr void create_and_place(t &dest, Alloc &a, const T &t) { //
    dest = a.template create<T>(std::move(t));
  }

  template <std::size_t n, typename Alloc> using extend_t = typename Alloc::template extend<n, T>;

  static constexpr T &get(ct_ptr<T> &p) { return *p; }

  static constexpr const T &get(const ct_ptr<T> &p) { return *p; }
};

template <typename T> struct list_node_payload<T, false> {
  static_assert(!std::is_class_v<T>);
  using t = T;
  template <typename Alloc> static constexpr void create_and_place(t &dest, Alloc &a, const T &t) {
    if constexpr (std::is_array_v<T>) {
      std::size_t i = 0;
      for (auto &e : t) {
        dest[i++] = e;
      }
    } else {
      dest = t;
    }
  }

  template <std::size_t, typename Alloc> using extend_t = Alloc;

  static constexpr T &get(T &p) { return p; }

  static constexpr const T &get(const T &p) { return p; }
};

template <typename T> using payload = typename list_node_payload<T>::t;
} // namespace list_detail

template <typename T> struct list {
  struct list_node {
    list_detail::payload<T> item{};
    ct_ptr<list_node> next{};

    constexpr list_node() {}

    template <typename alloc, typename T2, typename... T3>
    constexpr list_node(alloc &a, const T &t, const T2 &t2, const T3 &... t3)
        : next(a.template create<list_node>(a, t2, t3...)) {
      list_detail::list_node_payload<T>::create_and_place(a, t);
    }

    template <typename alloc> constexpr list_node(alloc &a, const T &t) {
      list_detail::list_node_payload<T>::create_and_place(item, a, t);
    }
    template <typename alloc> constexpr void destroy(alloc &a) {
      if constexpr (std::is_class_v<T>)
        if (item)
          a.destroy(item);
      item = decltype(item)();
      if (next)
        a.destroy(next);
      next = ct_ptr<list_node>();
    }

    template <std::size_t n, typename A>
    using extend_allocator = //
        typename list_detail::list_node_payload<T>::template extend_t<n, A>::template extend<n, list_node>;
  };
  ct_ptr<list_node> first{};
  constexpr list() = default;
  template <typename alloc, typename... T2>
  constexpr list(alloc &a, T t, T2 &&... t2)
      : first(a.template create<list_node>(a, std::move(t), std::forward<T2>(t2)...)) {}

  template <typename alloc> constexpr T &grow(alloc &a, T &&t = T{}) {
    auto old_first = first;
    first = a.template create<list_node>(a, std::move(t));
    first->next = old_first;
    // first = a.template create<T>(std::move(t));
    return list_detail::list_node_payload<T>::get(first->item);
  }

  template <typename alloc> constexpr void destroy(alloc &a) {
    if (first) {
      a.destroy(first);
    }
  }

  struct const_iterator {
    ct_ptr<list_node> next{};
    constexpr const T &operator*() const { return list_detail::list_node_payload<T>::get(next->item); }
    constexpr T const *operator->() const { return &operator*(); }

    constexpr bool operator==(const const_iterator &o) const { return next == o.next; }

    constexpr bool operator!=(const const_iterator &o) const { return !(this->operator==(o)); }
    constexpr const_iterator &operator++() {
      next = next->next;
      return *this;
    }
  };

  struct iterator {
    ct_ptr<list_node> next{};
    constexpr T &operator*() { return list_detail::list_node_payload<T>::get(next->item); }
    constexpr T *operator->() { return &operator*(); }

    constexpr bool operator==(const iterator &o) const { return next == o.next; }

    constexpr bool operator!=(const iterator &o) const { return !(this->operator==(o)); }
    constexpr iterator &operator++() {
      next = next->next;
      return *this;
    }
  };

  constexpr const_iterator begin() const { return const_iterator{first}; }

  constexpr const_iterator end() const { return const_iterator{}; }

  constexpr iterator begin() { return iterator{first}; }

  constexpr iterator end() { return iterator{}; }

  template <std::size_t n, typename A> using extend_allocator = typename list_node::template extend_allocator<n, A>;
};
} // namespace compile_time::allocator2
