#pragma once

#include "single_allocator.hpp"

namespace compile_time::allocator2 {

struct abstract_allocator {
  template <typename T> constexpr ct_ptr<T> create() {
    return dynamic_cast<detail::abstract_single_allocator<T> &>(*this).create();
  }
  constexpr virtual ~abstract_allocator() = default;
};

template <typename T> struct fold_ptr_hldr {
  T *t{nullptr};
  constexpr fold_ptr_hldr() = default;
  constexpr fold_ptr_hldr(T *t) : t(t) {}
};

template <typename T> constexpr fold_ptr_hldr<T> operator<<(const fold_ptr_hldr<T> &a, const fold_ptr_hldr<T> &b) {
  if (a.t == nullptr) {
    return b.t;
  } else {
    assert(!b.t);
    return a.t;
  }
}

template <typename T> constexpr memento<T> operator<<(const memento<T> &a, const memento<T> &c) {
  if (a.initialized == memento<T>::initialized::yes) {
    assert(c.initialized == memento<T>::initialized::no);
    return a;
  } else
    return c;
}

template <typename... SA> struct full_allocator : public SA..., public abstract_allocator {

private:
  constexpr full_allocator(std::size_t index) : SA(index++)... {}

public:
  constexpr full_allocator() : full_allocator(1) {}
  constexpr full_allocator(const full_allocator &o) = default;
  constexpr full_allocator(full_allocator &&) = default;

  template <typename A> constexpr std::size_t live_objects() const { return A::live_objects; }

  struct deactivation_context : public detail::deactivation_context<typename SA::allocated_type>... {};

  template <typename T> constexpr T *activate_single(const memento<T> &m) {
    if constexpr (!std::is_abstract_v<T>) /*concrete type*/ {
      constexpr std::size_t max_size_of_t = get_allocator_max_size<T>();
      single_allocator<T, max_size_of_t> &_this = *this;
      assert(_this.allocator_id == m.allocator_id);
      return _this.activate(m);
    } else {
      return ((SA::allocator_id == m.allocator_id ? fold_ptr_hldr<T>(SA::activate(m)) : fold_ptr_hldr<T>{})
              << ... << fold_ptr_hldr<T>{})
          .t;
    }
  }

  template <typename T> constexpr T const *activate_single(const memento<T> &m) const {
    if constexpr (!std::is_abstract_v<T>) /*concrete type*/ {
      constexpr std::size_t max_size_of_t = get_allocator_max_size<T>();
      const single_allocator<T, max_size_of_t> &_this = *this;
      assert(_this.allocator_id == m.allocator_id);
      return _this.activate(m);
    } else {
      constexpr fold_ptr_hldr<const T> null_case{};
      return (... << (SA::allocator_id == m.allocator_id
                          ? (fold_ptr_hldr<const T>{SA::activate(m, std::is_abstract<T>{})})
                          : null_case))
          .t;
    }
  }

  template <typename T>
  constexpr memento<T> deactivate_single(T *ptr, std::size_t allocator_id, const memento<T> &m,
                                         deactivation_context &d) {
    assert((m.initialized == memento<T>::initialized::yes ? m.allocator_id == allocator_id : true));
    if constexpr (!std::is_abstract_v<T>) /*concrete type*/ {
      constexpr std::size_t max_size_of_t = get_allocator_max_size<T>();
      single_allocator<T, max_size_of_t> &_this = *this;
      return _this.deactivate(ptr, m, d, std::is_abstract<T>{});
    } else {
      return ((SA::allocator_id == allocator_id ? SA::deactivate(ptr, m, d, std::is_abstract<T>{}) : memento<T>{})
              << ... << memento<T>{});
    }
  }

  class status_changer {
  public:
    enum class role : bool { activate, deactivate };
    const role role;

  private:
    using fap = full_allocator *;
    const fap fa{nullptr};
    full_allocator const *const fac{nullptr};
    deactivation_context d;

  public:
    constexpr status_changer(const status_changer &) = delete;
    constexpr status_changer(enum role r, full_allocator &fa) : role(r), fa(&fa) {
      assert(((fa.SA::allocator_id <= fa.get_num_allocators()) && ... && true));
      if (this->role == role::activate) {
        assert(((fa.SA::mode == mode::memento) && ...));
        ((fa.SA::mode = mode::active), ...);
      } else {
        assert(this->role == role::deactivate);
        assert(((fa.SA::mode == mode::active) && ...));
        ((fa.SA::mode = mode::memento), ...);
      }
    }
    constexpr status_changer(enum role activate, const full_allocator &fa) : role(activate), fac(&fa) {
      assert(((fa.SA::allocator_id <= fa.get_num_allocators()) && ... && true));
      assert(activate == role::activate);
      assert(((fa.SA::mode == mode::memento) && ...));
    }
    template <typename T> constexpr void act(ct_ptr<T> &p) {
      if (this->role == role::activate) {
        assert(fa || (fac && std::is_const_v<T> &&
                      "Error: cannot form non-const reference"
                      "to object stored in const allocator"));
        if (fa)
          p.activate(*fa);
        else
          p.activate(*fac);
      } else {
        assert(this->role == role::deactivate);
        assert(fa);
        assert(!std::is_const_v<T>);
        if constexpr (!std::is_const_v<T>)
          p.deactivate(d, *fa);
      }
    }

    constexpr ~status_changer() {
      if (this->role == role::deactivate) {
        assert(fa);
        (fa->SA::finish_deactivation(), ...);
      }
    }
  };

  struct activator : public status_changer {
    activator(full_allocator &fa) : status_changer(true, fa) {}
    activator(const full_allocator &fa) : status_changer(true, fa) {}
  };
  struct deactivator : public status_changer {
    deactivator(full_allocator &fa) : status_changer(false, fa) {}
  };

  template <typename T, typename... Args> constexpr ct_ptr<T> create(Args &&... args) {
    constexpr auto size = full_allocator::template get_allocator_max_size<T>();
    return single_allocator<T, size>::create(std::forward<Args>(args)...);
  }

  template <typename T> constexpr void destroy(ct_ptr<T> t) {
    if constexpr (detail::has_destroyer<T>::value)
      t->destroy(*this);
    return detail::abstract_single_allocator<T>::destroy(t);
  }

  template <typename T1, typename T2, typename... Trest>
  constexpr void destroy(ct_ptr<T1> t1, ct_ptr<T2> t2, Trest &&... trest) {
    destroy<T1>(t1);
    destroy<T2>(t2, std::forward<Trest>(trest)...);
  }

  template <typename T> constexpr const T &ref(const ct_ptr<T> &p) const {
    ct_ptr<const T> copy = p;
    copy.activate(*this);
    return *copy;
  }

  template <typename T> static constexpr std::size_t get_allocator_max_size() {
    return (0 + ... + (std::is_same_v<T, typename SA::allocated_type> ? SA::max_size::value : 0));
  }

  static constexpr std::size_t get_num_allocators() { return sizeof...(SA); }

private:
  template <std::size_t s_target, std::size_t s_so_far, typename FA, typename SA1, typename... SA2>
  constexpr const static auto &allocator_at(const FA &this_full) {
    if constexpr (s_target == s_so_far) {
      const SA1 &_this = this_full;
      assert(_this.allocator_id == s_target);
      return _this;
    } else
      return allocator_at<s_target, s_so_far + 1, FA, SA2...>(this_full);
  }

public:
  template <std::size_t s> constexpr const auto &allocator_at() const {
    return allocator_at<s, 1, full_allocator, SA...>(*this);
  }
  template <std::size_t sa, std::size_t offset> constexpr const auto *ref() const {
    return allocator_at<sa>().template ref<offset>();
  }

  template <std::size_t n, typename T> static constexpr auto *extend_f(T const *const) {
    static_assert((std::is_abstract_v<T> ? (std::is_base_of_v<T, typename SA::allocated_type> || ... || false) : true),
                  "Error: attempt to extend this allocator with an abstract type"
                  "when no concrete type is present");

    if constexpr (std::is_abstract_v<T> || (std::is_same_v<T, typename SA::allocated_type> || ... || false)) {
      return (full_allocator *)nullptr;
    } else {
      return (full_allocator<SA..., single_allocator<T, n>> *)nullptr;
    }
  }

  template <std::size_t n, typename T> using extend = std::decay_t<decltype(*extend_f<n>((T *)nullptr))>;
};

template <std::size_t n, typename... T> using uniform_allocator = full_allocator<single_allocator<T, n>...>;

template <typename FA, FA const *const a, typename... SA>
full_allocator<single_allocator<typename SA::allocated_type, a->template live_objects<SA>()>...>
adjust_allocator_size_f(full_allocator<SA...> const *const = a);

template <typename FA, FA const *const a>
using adjust_allocator_size = std::decay_t<decltype(adjust_allocator_size_f<FA, a>(a))>;

} // namespace compile_time::allocator2
