#pragma once
#include <stdexcept>
#include <type_traits>

namespace compile_time::allocator2 {

template <typename T> struct memento {
  enum class initialized { yes, no };
  initialized initialized{initialized::no};
  std::size_t allocator_id{0};
  std::size_t offset{0};
  constexpr memento() = default;
  constexpr memento(std::nullptr_t, std::size_t aid) : initialized(initialized::yes), allocator_id(aid) {}
  constexpr memento(const memento &) = default;
  template <typename T2>
  constexpr memento(const memento<T2> &o)
      : initialized(o.initialized == memento<T2>::initialized::yes ? initialized::yes : initialized::no),
        allocator_id(o.allocator_id), offset(o.offset) {
    static_assert(std::is_abstract_v<T> || std::is_abstract_v<T2>,
                  "Error: This constructor is only available to abstract mementos");
    static_assert(std::is_base_of_v<T, T2> || std::is_base_of_v<T2, T>, "Error: no subtyping relationship found");
  }

  constexpr bool operator==(const memento &o) const {
    if (o.initialized == initialized) {
      return allocator_id == o.allocator_id && offset == o.offset;
    } else
      return false;
  }

  constexpr bool operator!=(const memento &o) const { return !this->operator==(o); }

  constexpr memento &operator=(const memento &o) = default;
  template <typename T2> constexpr memento &operator=(const memento<T2> &o) {
    static_assert(std::is_abstract_v<T> || std::is_abstract_v<T2>, "Error: This assignment operator is only available "
                                                                   "to abstract mementos");
    static_assert(std::is_base_of_v<T, T2> || std::is_base_of_v<T2, T>, "Error: no subtyping relationship found");
    initialized = (o.initialized == memento<T2>::initialized::yes ? initialized::yes : initialized::no);
    allocator_id = o.allocator_id;
    offset = o.offset;
    return *this;
  }
  constexpr memento(std::size_t allocator_id, std::size_t offset)
      : initialized{initialized::yes}, allocator_id(allocator_id), offset(offset) {}
};

struct uninintialized_memento_exception : public std::runtime_error {
  uninintialized_memento_exception()
      : std::runtime_error("Typestate error: attempt to use memento while "
                           "memento has not been initialized") {}
};

} // namespace compile_time::allocator2
