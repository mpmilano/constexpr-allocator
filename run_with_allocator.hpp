#pragma once

#include "full_allocator.hpp"

namespace compile_time::allocator2 {

template <typename ret, typename Allocator> struct run_with_allocator_result;
template <typename ret, typename... SA> struct run_with_allocator_result<ret, full_allocator<SA...>> {
  ret returned{};
  full_allocator<SA...> allocator{};
};

namespace detail {

template <typename F, template <typename> class Walker, F *f, typename... T> auto *correct_size_f() {
  static constexpr uniform_allocator<0, T...> zero_allocator{
      []() constexpr {using allocator_t = uniform_allocator<0, T...>;
  allocator_t allocator{};
  using Walker_t = Walker<allocator_t>;
  auto ptr = (*f)(allocator);
  if constexpr (is_ct_ptr_v<std::decay_t<decltype(ptr)>>) {
    Walker_t walker{Walker_t::role::deactivate, allocator};
    walker.act(ptr);
    // allocator.deactivate(ptr, walker);
  }
  return allocator;
}
()
}; // namespace detail
using resized_allocator = adjust_allocator_size<std::decay_t<decltype(zero_allocator)>, &zero_allocator>;
return (resized_allocator *)(nullptr);
} // namespace compile_time::allocator2

template <typename F, template <typename> class Walker, F *f, typename... T>
using correct_size = std::decay_t<decltype(*correct_size_f<F, Walker, f, T...>())>;
}

template <typename FA> struct empty_walker {
  typename FA::status_changer sc;
  using role = enum FA::status_changer::role;
  constexpr empty_walker(role r, FA &fa) : sc(r, fa) {}
  constexpr empty_walker(role r, const FA &fa) : sc(r, fa) {}

  template <typename T> constexpr void act(ct_ptr<T> &p) { sc.act(p); }
};

template <typename... T> struct allocator {

  template <typename F, template <typename> class Walker, F *f> static constexpr decltype(auto) run_return_allocator() {
    using allocator_t = detail::correct_size<F, Walker, f, T...>;
    using return_t = std::decay_t<decltype((*f)(std::declval<abstract_allocator &>()))>;
    using result_t = run_with_allocator_result<return_t, allocator_t>;
    result_t result{};
    result.returned = (*f)(result.allocator);
    if constexpr (is_ct_ptr_v<return_t>) {
      using allocator_t = std::decay_t<decltype(result.allocator)>;
      using Walker_t = Walker<allocator_t>;
      Walker_t w{Walker_t::role::deactivate, result.allocator};
      w.act(result.returned);
      // result.allocator.deactivate(result.returned, w);
    }
    return result;
  }

  template <typename F, F *f> static constexpr decltype(auto) run_return_allocator() {
    static_assert(!is_ct_ptr_v<decltype((*f)(std::declval<abstract_allocator &>()))>,
                  "Error: must provide a walker to correctly deactivate return "
                  "result of this invocation");
    return run_return_allocator<F, empty_walker, f>();
  }

  template <typename F, template <typename> class Walker, F *f> static auto run_return_activated() {
    constexpr static auto ret = run_return_allocator<F, Walker, f>();
    constify<decltype(ret.returned)> result = ret.returned;
    using allocator_t = std::decay_t<decltype(ret.allocator)>;
    Walker<allocator_t> w{Walker<allocator_t>::role::activate, ret.allocator};
    w.act(result);
    // ret.allocator.activate(result, w);
    return result;
  }

  template <typename F, F *f> static auto run_return_activated() {
    static_assert(!is_ct_ptr_v<decltype((*f)(std::declval<abstract_allocator &>()))>,
                  "Error: must provide a walker to correctly deactivate return "
                  "result of this invocation");
    return run_return_activated<F, empty_walker, f, T...>();
  }
};

template <typename... SA>
allocator<typename SA::allocated_type...> *context_from_full_allocator_f(full_allocator<SA...> const *const) {
  return nullptr;
}
template <typename A>
using context_from_full_allocator = std::decay_t<decltype(*context_from_full_allocator_f((A *)nullptr))>;
}
