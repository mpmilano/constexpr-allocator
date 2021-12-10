#pragma once
#include <type_traits>
#include <concepts>
#include <compare>
#include <cassert>
#include <utility>
#include "concept_extras.hpp"
#include "allocator_info.hpp"
#include "single_allocator.hpp"


namespace compile_time::allocator{


	template<typename... T>
	struct typespace{
	    using ThisInfo = Info<T...>;

    
	    template<ThisInfo original_info = ThisInfo{}>
	    struct allocator : public single_allocator<T, original_info.template max<T>()>...{

		template<PackMember<T...> U>
		    constexpr single_allocator<U, original_info.template max<U>()>&
		    single(){
		    return *this;
		}

		static const constexpr ThisInfo old_info {original_info};

		//we're not going to collect stats on the second execution, just use the inherited ones
		ThisInfo new_info{old_info};
		
		template<PackMember<T...> U, typename... Args> 
		    constexpr allocated_ptr<U> alloc(Args&& ...args){
		    return
			single<U>().
			alloc(new_info,std::forward<Args>(args)...);
		}

		template<PackMember<T...> U>
		    constexpr allocated_ptr<U> rewrap(U* tofree){
		    return single<U>().rewrap(new_info,tofree);
		}

		constexpr allocator() = default;
	    };

	    template<PackMember<T...> U, ThisInfo info> 
	    struct execution_result{
		allocator<info> allocations;
	      allocated_ptr<U> result;

		using ret_t = U;
		using allocator_t = allocator<info>;
		
		template<typename F>
		constexpr execution_result(F&& f)
		    :result(f(allocations)){}
	      constexpr ~execution_result() {
		//want to make extra sure this is empty before we try
		//to do any allocator destruction!
		result.clear();
	      }
	    };

	  template<typename F, F f, typename U>
	  static constexpr decltype(auto) exec_ap(allocated_ptr<U> const * const){
	    //run it the first time to see what the allocation totals are
	    constexpr auto tic =
		[&]() constexpr {
		    ThisInfo info;
		    {
			execution_result<U,ThisInfo{}> result{f};
			info.advance_to(result.allocations.new_info);
		    }
		    return info;
		}();
	    return new execution_result<U,ThisInfo{tic}>{f};
	  }
	  
	  template<typename F, F f>
	  static constexpr decltype(auto) exec(){
	    //trampoline to find + constrain return result
	    using Uptr =
		std::decay_t<decltype(f(std::declval<allocator<ThisInfo{}>&>()))>;
	    Uptr *null{nullptr};
	    return exec_ap<F,f>(null);
	  }

	    template<typename F>
	    static constexpr decltype(auto) pexec(F&&){
		return exec<F,F{}>();
	    }

	    template<auto f>
	    static constexpr decltype(auto) cexec(){
		return exec<decltype(f),f>();
	    }

	};
}
