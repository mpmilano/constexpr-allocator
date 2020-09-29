#pragma once
#include "ct-dynamic-map.hpp"
#include "ct_ptr.hpp"
#include "memento.hpp"
#include "object_tracker.hpp"
#include "virtual_destructor.hpp"

/*
  Allocator is greatly simplified.  The Allocator switches between two
  modes.  In "active" mode, all allocations are performed using `new`
  and `delete`.  In "memento" mode allocations are disabled
  entirely---but "deactivate", which takes a ct_ptr and returns a
  memento, can perform an allocation-like behavior using the built-in
  bump pointer and array.
 */
namespace compile_time::allocator2 {

namespace detail {

template <typename T, std::size_t n> struct memento_data {
  T v[n] = {T{}};
  std::size_t bump_ptr{0};
};

template <typename T> struct memento_data<T, 0> {};

template <typename T> struct deactivation_context {
  bool allocator_overflow{false};
  dynamic_map<T *, memento<T>> allocated_mementos{};
};

template <typename T> class has_destroyer {
  typedef char one;
  struct two {
    char x[2];
  };

  template <typename C> static one test(decltype(&C::destroy));
  template <typename C> static two test(...);

public:
  enum { value = sizeof(test<T>(0)) == sizeof(char) };
};

template <typename T> struct abstract_single_allocator : public virtual_destructor {

  static_assert(std::is_class_v<T>, "Error: can only allocate class-types for now");

  const std::size_t allocator_id;
  constexpr abstract_single_allocator(std::size_t allocator_id) : allocator_id(allocator_id) {}
  constexpr abstract_single_allocator(const abstract_single_allocator &) = default;

  constexpr virtual ct_ptr<T> create() = 0;
  constexpr virtual void destroy(const ct_ptr<T> &o) = 0;
  constexpr virtual ~abstract_single_allocator() = default;
};
} // namespace detail

template <typename T, std::size_t n> struct single_allocator;

template <typename T, std::size_t n>
constexpr bool _is_single_allocator(single_allocator<T, n> const *const t = nullptr) {
  return true;
}

template <typename... T> constexpr bool _is_single_allocator(T...) { return false; }

template <typename T> constexpr bool is_single_allocator(T const *const t = nullptr) { return _is_single_allocator(t); }

template <typename... SA> struct full_allocator;

template <typename... T> constexpr bool _is_full_allocator(full_allocator<T...> const *const t = nullptr) {
  return true;
}

template <typename... T> constexpr bool _is_full_allocator(T...) { return false; }

template <typename T> constexpr bool is_full_allocator(T const *const t = nullptr) { return _is_full_allocator(t); }

template <typename T, std::size_t n> struct single_allocator : public detail::abstract_single_allocator<T> {

  static_assert(!is_single_allocator<T>(), "Error: Trying to allocate yourself...");
  static_assert(!is_full_allocator<T>(), "Error: Trying to allocate yourself...");
  detail::memento_data<T, n> mementos{};
  std::size_t live_objects{0};
  mode mode{mode::active};

  constexpr single_allocator(std::size_t allocator_id) : detail::abstract_single_allocator<T>(allocator_id) {}
  constexpr single_allocator(const single_allocator &o)
      : detail::abstract_single_allocator<T>(o), mementos(o.mementos), live_objects(o.live_objects), mode(o.mode) {
    assert(mode == mode::memento);
    assert(!o.super_allocators);
    assert(!o.live_dynamic_objects);
  }
  constexpr single_allocator(single_allocator &&o)
      : detail::abstract_single_allocator<T>(o), mementos(o.mementos), live_objects(o.live_objects), mode(o.mode) {
    assert(mode == mode::memento);
    assert(!o.super_allocators);
    assert(!o.live_dynamic_objects);
  }

  single_allocator &get_this() { return *this; }

  const single_allocator &get_this() const { return *this; }

  using allocated_type = T;
  using max_size = std::integral_constant<std::size_t, n>;

  dynamic_list<virtual_destructor> *super_allocators{nullptr};
  template <typename T2> detail::abstract_single_allocator<T2> &create_supertype_abstract_allocator() {
    struct super_single_allocator : public detail::abstract_single_allocator<T2> {
      single_allocator &_this;
      super_single_allocator(single_allocator &_this)
          : detail::abstract_single_allocator<T2>(_this.allocator_id), _this(_this) {}
      constexpr virtual ct_ptr<T2> create() { return _this.create(); }
      constexpr virtual void destroy(const ct_ptr<T2> &o) { return _this.destroy(o.template downcast<T>()); }
    };
    auto ret = new super_single_allocator(*this);
    super_allocators = new dynamic_list<virtual_destructor>(ret, super_allocators);
    return *ret;
  }; // namespace compile_time::allocator3

  object_tracker<T> *live_dynamic_objects{nullptr};

  template <typename... Args> constexpr ct_ptr<T> create(Args &&... args) {
    assert(mode == mode::active);
    if (!live_dynamic_objects)
      live_dynamic_objects = new object_tracker<T>();
    ++live_objects;
    auto ret = new T(std::forward<Args>(args)...);
    live_dynamic_objects->track(ret);
    return ct_ptr<T>(ret, this->allocator_id);
  }

  constexpr ct_ptr<T> create() override { return create<>(); }

  constexpr void destroy(const ct_ptr<T> &o) override {
    assert(mode == mode::active);
    --live_objects;
    assert(o.mode == mode);
    delete o.ptr;
    if (live_dynamic_objects)
      live_dynamic_objects->untrack(o.ptr);
  }

#define sa_activate_body(T)                                                                                            \
  assert(m.initialized == memento<T>::initialized::yes);                                                               \
  if constexpr (n > 0) {                                                                                               \
    if (m.offset)                                                                                                      \
      return &mementos.v[m.offset - 1];                                                                                \
    else                                                                                                               \
      return nullptr;                                                                                                  \
  } else {                                                                                                             \
    assert(!m.offset);                                                                                                 \
    return nullptr;                                                                                                    \
  }

#define sa_activate_body_abstract                                                                                      \
  if constexpr (std::is_base_of_v<std::decay_t<T2>, std::decay_t<T>>) {                                                \
    sa_activate_body(T2)                                                                                               \
  } else {                                                                                                             \
    assert((m.allocator_id == 1 && m.initialized == memento<T2>::initialized::no &&                                    \
            "Error: this line should be unreachable.  check if allocator can "                                         \
            "activate type before calling activate!"));                                                                \
    return nullptr;                                                                                                    \
  }

  constexpr T *activate(const memento<T> &m, std::false_type) {
    assert(mode == mode::active);
    sa_activate_body(T);
  }

  template <typename T2> constexpr T2 *activate(const memento<T2> &m, std::true_type) {
    assert(mode == mode::active);
    sa_activate_body_abstract;
  }

  constexpr T const *activate(const memento<T> &m, std::false_type) const {
    // this is for the const case anyway
    assert(mode == mode::memento);
    sa_activate_body(T);
  }

  template <typename T2> constexpr T2 const *activate(const memento<T2> &m, std::true_type) const {
    // this is for the const case anyway
    assert(mode == mode::memento);
    sa_activate_body_abstract;
  }

  template <typename T2> constexpr T2 const *activate(const memento<T2> &m) const {
    return activate(m, std::is_abstract<std::decay_t<T2>>{});
  }

  template <typename T2> constexpr T2 *activate(const memento<T2> &m) {
    return activate(m, std::is_abstract<std::decay_t<T2>>{});
  }

  template <std::size_t indx> constexpr auto *ref() const {
    memento<T> m{nullptr, this->allocator_id};
    m.offset = indx;
    return activate(m);
  }

  constexpr memento<T> create_deactivation(T *ptr, detail::deactivation_context<T> &d) {
    assert(mode == mode::memento);
    if constexpr (n > 0) {
      if (mementos.bump_ptr < n) {
        auto ret = memento<T>{this->allocator_id, ++mementos.bump_ptr};
        mementos.v[ret.offset - 1] = *ptr;
        return ret;
      }
    }
    d.allocator_overflow = true;
    return memento<T>{nullptr, this->allocator_id};
  }

  constexpr memento<T> deactivate(T *ptr, const memento<T> &m, detail::deactivation_context<T> &d, std::false_type) {
    assert(mode == mode::memento);
    assert(ptr);

    if (d.allocated_mementos.contains(ptr)) {
      return d.allocated_mementos.at(ptr);
    } else {
      if (m.initialized == memento<T>::initialized::yes) {
        d.allocated_mementos.insert(ptr, m);
        return m;
      } else {
        auto m2 = create_deactivation(ptr, d);
        d.allocated_mementos.insert(ptr, m2);
        delete ptr;
        if (live_dynamic_objects)
          live_dynamic_objects->untrack(ptr);
        return m2;
      }
    }
  }

  template <typename T2>
  constexpr memento<T2> deactivate(T2 *ptr, const memento<T2> &m, detail::deactivation_context<T> &d, std::true_type) {
    if constexpr (std::is_abstract_v<T2> && std::is_base_of_v<std::decay_t<T2>, std::decay_t<T>>) {
      assert(ptr);
      auto ptr2 = dynamic_cast<T *>(ptr);
      assert(ptr2);
      return deactivate(ptr2, m, d, std::is_abstract<T>{});
    } else {
      assert((m.allocator_id == 1 && m.initialized == memento<T2>::initialized::no &&
              "Error: this line should be unreachable.  check if allocator can "
              "deactivate type before calling deactivate!"));
      return memento<T2>{nullptr, this->allocator_id};
    }
  }

  template <typename T2> constexpr T2 *deactivate(T2 *ptr, const memento<T2> &m, detail::deactivation_context<T> &d) {
    return deactivate(ptr, m, d, std::is_abstract<std::decay_t<T2>>{});
  }

  constexpr void finish_deactivation() {
    if (live_dynamic_objects) {
      for (auto *ptr : *live_dynamic_objects) {
        delete ptr;
        --live_objects;
      }
      delete live_dynamic_objects;
      live_dynamic_objects = nullptr;
    }
    if (super_allocators)
      delete super_allocators;
    super_allocators = nullptr;
  }

protected:
  constexpr ~single_allocator() { finish_deactivation(); }
};

template <typename SSA, SSA const *ssa>
using adjust_single_allocator_size = single_allocator<typename SSA::allocated_type, ssa->live_objects>;

} // namespace compile_time::allocator2
