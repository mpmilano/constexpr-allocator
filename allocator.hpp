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

		
	    };

	    template<PackMember<T...> U, ThisInfo info> 
	    struct execution_result{
		allocator<info> allocations;
	      allocated_ptr<U> result;
		template<typename F>
		constexpr execution_result(F&& f):result(f(allocations)){}
	      constexpr ~execution_result() {
		//want to make extra sure this is empty before we try
		//to do any allocator destruction!
		result.clear();
	      }
	    };

	  template<typename F, F f, typename U>
	  static constexpr decltype(auto) exec(allocated_ptr<U> const * const){
	    //run it the first time to see what the allocation totals are
	    constexpr auto tic = execution_result<U,ThisInfo{}>(f).allocations.new_info;
	    return execution_result<U,ThisInfo{tic}>{f};
	  }
	  
	  template<typename F, F f>
	  static constexpr decltype(auto) exec(){
	    //trampoline to find + constrain return result
	    using Uptr = typename 
	      std::invoke_result_t<F, allocator<ThisInfo{}>&>;
	    Uptr *null{nullptr};
	    return exec<F,f>(null);
	  }

	    template<typename F>
	    static constexpr decltype(auto) pexec(F&& f){
		return exec<F,F{}>();
	    }
	    
	};
}
