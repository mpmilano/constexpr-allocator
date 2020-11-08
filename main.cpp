#include "allocator.hpp"

using namespace compile_time;
using namespace allocator;

using ts = typespace<int,double>;

int main(){
    constexpr static auto result = ts::pexec([](auto& a){
	auto one = a.template alloc<int>(5);
	auto two = a.template alloc<double>(2.0);
	auto tmp = *one + *two;
	a.dealloc(one);
	a.dealloc(two);
	return a.template alloc<double>(tmp);
    });
    return *result.result;
}
