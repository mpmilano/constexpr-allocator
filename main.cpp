#include "allocator.hpp"

using namespace compile_time;
using namespace allocator;

using ts = typespace<int,double>;

constexpr auto allocator_actions(auto &a){
  auto one = a.template alloc<int>(5);
  auto two = a.template alloc<double>(2.0);
  auto tmp = *one + *two;
  auto three = a.template alloc<int>(5);
  auto four = a.template alloc<double>(2.0);
  one.clear();
  two.clear();
  three.clear();
  four.clear();
  return a.template alloc<double>(tmp);  
}

int main(){

  //dynamic test 1
  {
    ts::allocator<ts::ThisInfo{}> a;
    {
      auto result = allocator_actions(a);
    }
  }

  //static test
  
  constexpr static auto result = ts::pexec([](auto& a) constexpr {return allocator_actions(a);});
  return *result.result;//*/
}
