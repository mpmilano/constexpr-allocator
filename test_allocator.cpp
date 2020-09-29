#include "full_allocator.hpp"
#include "run_with_allocator.hpp"

using namespace compile_time;
using namespace allocator2;

struct A {
  virtual constexpr std::size_t get() const = 0;
  virtual constexpr ~A() = default;
};

struct B : public A {
  std::size_t field{0};
  constexpr std::size_t get() const override { return field; }
};

constexpr ct_ptr<A> allocator_test_step2(abstract_allocator &alloc) {
  auto ret = alloc.template create<B>();
  auto discard1 = alloc.template create<int>();
  auto discard2 = alloc.template create<double>();
  auto discard3 = alloc.template create<double>();
  *discard1 = 1;
  *discard2 = 2;
  *discard3 = 3;
  ret->field = *discard1 + *discard2 + *discard3;
  // alloc.destroy(discard1, discard2, discard3);
  return ct_ptr<A>{ret};
}

int main() {
  using allocator = allocator<B, int, double>;
  constexpr auto return_pair =
      allocator::run_return_allocator<decltype(allocator_test_step2), empty_walker, allocator_test_step2>();
  ct_ptr<const A> return_result = return_pair.returned;
  return_result.activate(return_pair.allocator);

  return return_result->get() +
         allocator::run_return_activated<decltype(allocator_test_step2), empty_walker, allocator_test_step2>()->get();
}
