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

    struct destructor_chain{
	virtual constexpr ~destructor_chain() = default;
	destructor_chain(const destructor_chain&) = delete;
	
	template<typename T>
	struct single : public destructor_chain {
	    const unique_ptr<destructor_chain> prev;
	    const unique_ptr<T> payload;
	    constexpr single(unique_ptr<destructor_chain> prev, unique_ptr<T> payload)
		:prev(std::move(prev)),payload(std::move(payload)){}
	    constexpr ~single() = default;
	};
    };
    
    template<typename T> result_pair{
	allocator::allocated_ptr<T> result;
	const unique_ptr<destructor_chain> allocator;
    };
    
    template<typename fa, typename pfa, typename F, typename previous_F>
    constexpr auto temporarily_executed(const F& f = F{}, const previous_F& pf = previous_F{}){
	unique_ptr<pfa> previous_allocator{new pfa()};
	unique_ptr<fa> allocator{new fa()};
	auto rp = pf(*previous_allocator);
	auto &&res = f(*allocator,*rp.result);
	return result_pair<std::decay_t<decltype(*res.result)>>{
	    std::move(res),
	    new destructor_chain::single<fa>{nullptr,std::move(allocator)}};
    }

    template<typename fa, typename F>
    constexpr auto temporarily_executed(const F& f = F{}){
	unique_ptr<fa> allocator{new fa()};
	auto &&res = f(*allocator);
	return result_pair<std::decay_t<decltype(*res.result)>>{std::move(res),std::move(allocator)};
    }

	template<typename... T>
	struct typespace{
	    using ThisInfo = Info<T...>;

    
	    template<ThisInfo original_info = ThisInfo{}>
	    struct allocator : public destructable, public single_allocator<T, original_info.template max<T>()>...{

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

	    template<PackMember<T...> U, ThisInfo info, typename F, typename prev_stage_f> 
	    struct execution_result{
		allocator<info> allocations;
	      allocated_ptr<U> result;

		using ret_t = U;
		using allocator_t = allocator<info>;

	    private:
		template<typename prev_stage_result>
		constexpr execution_result(const F& f, const allocated_ptr<prev_stage_result>& r,int)
		    :result(f(allocations,*)){}

	    public:
		constexpr execution_result(const F& f, const prev_stage_f& pf)
		    :execution_result(f, pf().result){}
		
	      constexpr ~execution_result() {
		//want to make extra sure this is empty before we try
		//to do any allocator destruction!
		result.clear();
	      }
	    };

	    template<typename previous_stages, typename This_Stage, typename this_stage_result>
	    using constexpr_executed = execution_result<this_stage_result,
		ThisInfo{[]() constexpr {
		    ThisInfo info;
		    {
			execution_result<this_stage_result,ThisInfo{},This_Stage, previous_stages> result;
			info.advance_to(result.allocations.new_info);
		    }
		    return info;
		}()},This_Stage,previous_stages>;

	};
}
