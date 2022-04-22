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

struct tree{
  int payload;
  allocated_ptr<tree> left;
  allocated_ptr<tree> right;
  constexpr tree(int p, allocated_ptr<tree> left, allocated_ptr<tree> right)
    :payload(p),
     left(std::move(left)),
     right(std::move(right)){}
  constexpr ~tree() = default;
};

constexpr auto tree_test(auto &a){
  #define TREE  a.template alloc<tree>
  return TREE(0,TREE(1,nullptr,nullptr),TREE(2,nullptr,nullptr));
}

template<typename T> struct just_delete : public destructor<T>{
    ~just_delete() = default;
    just_delete() = default;
    void destroy(T *t) override {delete t;}
};

int main(){

  //dynamic test 1
  {
    ts::allocator<ts::ThisInfo{}> a;
    {
      auto result = allocator_actions(a);
    }
  }

  //dynamic test 2
  {
    using tree_ts = typespace<tree>;
    tree_ts::allocator<tree_ts::ThisInfo{}> a;
    {
      auto result = tree_test(a);
    }
  }

  //static test
  
  //constexpr static auto result = ts::pexec([](auto& a) constexpr {return allocator_actions(a);});
  //return *result.result;//*/
  constexpr auto five_f = []() constexpr {static constexpr return result_pair<int>{};};

  constexpr auto aa_wrapper = [](auto &a, int) constexpr {return allocator_actions(a);};
  using foo = ts::template constexpr_executed<decltype(five_f),decltype(aa_wrapper),double>;
  return foo{}.asint();
}
