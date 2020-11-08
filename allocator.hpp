#pragma once
#include <type_traits>
#include <concepts>
#include <compare>
#include <cassert>
#include <utility>
#include "concept_extras.hpp"
#include "allocator_info.hpp"
#include "single_allocator_initial.hpp"
#include "single_allocator_second.hpp"

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
		    constexpr U* alloc(Args&& ...args){
		    return
			single<U>().
			alloc(new_info,std::forward<Args>(args)...);
		}

		template<PackMember<T...> U>
		    constexpr void dealloc(U* tofree){
		    return single<U>().dealloc(new_info,tofree);
		}

		constexpr void clear(){
		    (single<T>().clear(),...);
		}
		
	    };

	    template<PackMember<T...> U, ThisInfo info> 
	    struct execution_result{
		allocator<info> allocations;
		U *result = nullptr;
		template<typename F>
		constexpr execution_result(F&& f):result(f(allocations)){}
		constexpr ~execution_result() = default;
	    };

	    template<typename F, F f>
	    static constexpr decltype(auto) exec(){
		using U = std::remove_pointer_t<
		    std::invoke_result_t<F, allocator<ThisInfo{}>&>>;
		//run it the first time to see what the allocation totals are
		constexpr auto tic = execution_result<U,ThisInfo{}>(f).allocations.new_info;
		return execution_result<U,ThisInfo{tic}>{f};
	    }

	    template<typename F>
	    static constexpr decltype(auto) pexec(F&& f){
		return exec<F,F{}>();
	    }
	    
	};
}
