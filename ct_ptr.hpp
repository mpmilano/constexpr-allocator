/*
Here's the idea.  Our compile-time pointers contain a "real" pointer
value, and a memento value which can be combined with an allocator to
produce this real pointer value.  They support an "enter" method
which, when given an allocator, attempts to reconstruct the pointer
from a combination of its memento and that allocator's storage, and an
"exit" method, which (when given an allocator) mementoifies the
pointer. Enter and Exit both need to walk the pointer tree; we'll
probably use an external visitor for this purpose.

 */

#pragma once
#include "ct-dynamic-map.hpp"
#include "memento.hpp"

namespace compile_time::allocator2 {

enum class mode { active, memento };

template <typename T> struct deactivation_context;

template <typename... SA> struct full_allocator;

template <typename T> struct ct_ptr_impl {

  mode mode{mode::active};
  memento<std::decay_t<T>> memento{};
  std::size_t allocator_id{0};
  T *ptr{nullptr};

  constexpr ct_ptr_impl() = default;
  constexpr ct_ptr_impl(T *ptr, std::size_t aid) : ptr(ptr), allocator_id(aid) {}

protected:
  constexpr ct_ptr_impl(const ct_ptr_impl &o) = default;
  template <typename T2>
  constexpr ct_ptr_impl(const ct_ptr_impl<T2> &o)
      : mode(o.mode), memento(o.memento), allocator_id(o.allocator_id), ptr(o.ptr) {
    static_assert(std::is_same_v<std::decay_t<T2>, std::decay_t<T>> ||
                  std::is_base_of_v<std::decay_t<T>, std::decay_t<T2>>);
  }

  constexpr ct_ptr_impl(ct_ptr_impl &&) = delete;

  template <typename T2> constexpr ct_ptr_impl &operator=(const ct_ptr_impl<T2> &o) {
    static_assert(std::is_same_v<std::decay_t<T2>, std::decay_t<T>> ||
                  std::is_base_of_v<std::decay_t<T>, std::decay_t<T2>>);
    assert((o.mode == mode::memento ? (mode == o.mode || !ptr) : o.mode == mode::active));
    mode = o.mode;
    memento = o.memento;
    ptr = o.ptr;
    allocator_id = o.allocator_id;
    return *this;
  }

  constexpr ct_ptr_impl &operator=(const ct_ptr_impl &o) { return operator=<T>(o); }

  template <typename T2> constexpr ct_ptr_impl &operator=(ct_ptr_impl<T2> &&o) {
    const ct_ptr_impl<T2> &o2 = o;
    return *this = o2;
  }

  constexpr ct_ptr_impl &operator=(ct_ptr_impl &&o) { return operator=<T>(o); }

private:
  template <typename... SA> constexpr void activate(full_allocator<SA...> &fa) {
    assert(memento.initialized == decltype(memento)::initialized::yes);
    assert(mode == mode::memento);
    if (memento.offset)
      ptr = fa.activate_single(memento);
    else
      ptr = nullptr;
    mode = mode::active;
  }
  // Const version: for activations in constant (non-constexpr) contexts
  //(e.g. with cexpr allocators)
  template <typename... SA> void activate(const full_allocator<SA...> &fa) {
    assert(std::is_const_v<T> && "Error: cannot form non-const reference to object stored in "
                                 "const allocator. Consider copying the allocator or "
                                 "constructing a const-ptr from this non-const ptr.");
    if constexpr (std::is_const_v<T>) {
      if (memento.initialized == decltype(memento)::initialized::yes) {
        assert(mode == mode::memento);
        if (memento.offset)
          ptr = fa.activate_single(memento);
        else
          ptr = nullptr;
        mode = mode::active;
      } else {
        // don't try to activate null things.
        throw uninintialized_memento_exception{};
      }
    }
  }

  template <typename... SA>
  constexpr void deactivate(typename full_allocator<SA...>::deactivation_context &dc, full_allocator<SA...> &fa) {
    static_assert(!std::is_const_v<T>, "Error: deactivate will likely involve a delete operator. "
                                       "It requires non-const access to T.");
    assert(mode == mode::active);
    if (*this) {
      memento = fa.deactivate_single(ptr, this->allocator_id, memento, dc);
      assert(memento.allocator_id == this->allocator_id);
    } else {
      assert(ptr == nullptr);
      memento.offset = 0;
      memento.allocator_id = 0;
      memento.initialized = decltype(memento)::initialized::yes;
    }
    mode = mode::memento;
    ptr = nullptr;
  }

public:
  constexpr bool operator==(const ct_ptr_impl &o) const {
    assert(this->allocator_id == o.allocator_id || !(*this) || !o);
    if (!(*this) || !o)
      return !(*this) && !o;
    if (mode == o.mode) {
      return (mode == mode::active ? ptr == o.ptr : memento == o.memento);
    } else if (memento.initialized == decltype(memento.initialized)::yes &&
               o.memento.initialized == decltype(o.memento.initialized)::yes) {
      return memento == o.memento;
    } else
      return false;
  }

  constexpr bool operator!=(const ct_ptr_impl &o) const { return !(operator==(o)); }

  constexpr T *get() {
    assert(mode == mode::active);
    return ptr;
  }

  constexpr T const *get() const {
    assert(mode == mode::active);
    return ptr;
  }

  constexpr operator bool() const {
    return ptr || mode == mode::memento && (memento.allocator_id > 0) && (memento.offset > 0);
  }
  constexpr T &operator*() { return *get(); }
  constexpr const T &operator*() const { return *get(); }
  constexpr T *operator->() { return get(); }
  constexpr T const *operator->() const { return get(); }

  template <typename... SA> friend struct full_allocator;
};

#define CT_PTR_DOWNCAST_METHOD                                                                                         \
  template <typename T2> constexpr ct_ptr<T2> downcast() const {                                                       \
    static_assert(std::is_base_of_v<std::decay_t<T>, std::decay_t<T2>>, "Error: no subtyping relationship found");     \
    if constexpr (std::is_same_v<std::decay_t<T>, std::decay_t<T2>>) {                                                 \
      return *this;                                                                                                    \
    } else {                                                                                                           \
      auto ptr2 = dynamic_cast<T2 *>(this->ptr);                                                                       \
      assert(ptr2);                                                                                                    \
      ct_ptr<T2> _this{ptr2};                                                                                          \
      _this.memento = this->memento;                                                                                   \
      _this.mode = this->mode;                                                                                         \
      return _this;                                                                                                    \
    }                                                                                                                  \
  }

template <typename T> struct ct_ptr : public ct_ptr_impl<T> {

  constexpr ct_ptr() = default;
  constexpr ct_ptr(T *ptr, std::size_t allocator_id) : ct_ptr_impl<T>(ptr, allocator_id) {}

  template <typename T2> constexpr ct_ptr(const ct_ptr<T2> &o) : ct_ptr_impl<T>(o) {
    static_assert(!std::is_const_v<T2>, "Error: const cannot convert to "
                                        "non-const");
    static_assert(std::is_base_of_v<T, std::decay_t<T2>>, "Error: no subtyping relationship found");
    static_assert(std::is_same_v<std::decay_t<T>, std::decay_t<T2>> || std::is_abstract_v<std::decay_t<T>>,
                  "Error: can only use abstract supertypes for now.");
  }

  template <typename T2> constexpr ct_ptr &operator=(const ct_ptr<T2> &o) {
    static_assert(!std::is_const_v<T2>, "Error: const cannot convert to "
                                        "non-const");
    static_assert(std::is_base_of_v<T, std::decay_t<T2>>, "Error: no subtyping relationship found");
    static_assert(std::is_same_v<std::decay_t<T>, std::decay_t<T2>> || std::is_abstract_v<std::decay_t<T>>,
                  "Error: can only use abstract supertypes for now.");
    const ct_ptr_impl<T2> &oimpl = o;
    ct_ptr_impl<T>::template operator=<T2>(oimpl);
    return *this;
  }

  template <typename T2> constexpr ct_ptr &operator=(ct_ptr<T2> &&o) {
    static_assert(!std::is_const_v<T2>, "Error: const cannot convert to "
                                        "non-const");
    static_assert(std::is_base_of_v<T, std::decay_t<T2>>, "Error: no subtyping relationship found");
    static_assert(std::is_same_v<std::decay_t<T>, std::decay_t<T2>> || std::is_abstract_v<std::decay_t<T>>,
                  "Error: can only use abstract supertypes for now.");

    const ct_ptr &o2 = o;
    return *this = o2;
  }

  constexpr ct_ptr<const T> as_const() const { return ct_ptr<const T>{*this}; }

  CT_PTR_DOWNCAST_METHOD;
};

template <typename T> struct ct_ptr<const T> : public ct_ptr_impl<const T> {

  constexpr ct_ptr() = default;
  constexpr ct_ptr(T const *ptr, std::size_t allocator_id) : ct_ptr_impl<const T>(ptr, allocator_id) {}

  template <typename T2> constexpr ct_ptr(const ct_ptr<T2> &o) : ct_ptr_impl<const T>(o) {
    static_assert(std::is_base_of_v<T, std::decay_t<T2>>, "Error: no subtyping relationship found");
    static_assert(std::is_same_v<std::decay_t<T>, std::decay_t<T2>> || std::is_abstract_v<std::decay_t<T>>,
                  "Error: can only use abstract supertypes for now.");
  }

  template <typename T2> constexpr ct_ptr &operator=(const ct_ptr<T2> &o) {
    static_assert(std::is_base_of_v<T, std::decay_t<T2>>, "Error: no subtyping relationship found");
    static_assert(std::is_same_v<std::decay_t<T>, std::decay_t<T2>> || std::is_abstract_v<std::decay_t<T>>,
                  "Error: can only use abstract supertypes for now.");

    ct_ptr_impl<const T>::template operator=<T>(o);
    return *this;
  }

  template <typename T2> constexpr ct_ptr &operator=(ct_ptr<T2> &&o) {
    static_assert(std::is_base_of_v<T, std::decay_t<T2>>, "Error: no subtyping relationship found");
    static_assert(std::is_same_v<std::decay_t<T>, std::decay_t<T2>> || std::is_abstract_v<std::decay_t<T>>,
                  "Error: can only use abstract supertypes for now.");

    const ct_ptr<T> &o2 = o;
    return *this = o2;
  }

  constexpr ct_ptr as_const() const { return *this; }

  CT_PTR_DOWNCAST_METHOD;
};

template <typename T> struct is_ct_ptr : public std::integral_constant<bool, false> {};
template <typename T> struct is_ct_ptr<ct_ptr<T>> : public std::integral_constant<bool, true> {};
template <typename T> inline constexpr bool is_ct_ptr_v = is_ct_ptr<std::decay_t<T>>::value;

namespace detail {

template <typename T> struct constify { using type = T; };
template <typename T> struct constify<ct_ptr<const T>> { using type = ct_ptr<const T>; };
template <typename T> struct constify<ct_ptr<T>> : public constify<ct_ptr<const T>> {};
} // namespace detail

template <typename T> using constify = typename detail::template constify<T>::type;

} // namespace compile_time::allocator2
