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

    struct destructable{
	constexpr virtual ~destructable() = default;
    };
    
    template<typename T> struct result_pair{
	allocator::allocated_ptr<T> result;
	const unique_ptr<destructable> allocator;
    };

    template<typename T> struct is_result_pair : std::false_type{};
    template<typename T> struct is_result_pair<result_pair<T>> : std::true_type{};
    template<typename T> concept ResultPair = is_result_pair<T>::value;

    template<typename T> concept AllocatorThunk = Thunk<T> && requires (const T& t) {{t()} -> ResultPair;};

    template<ResultPair rp> struct result_s;
    template<typename T> struct result_s<result_pair<T>> {using type = T;};
    template<AllocatorThunk rp> using result = typename result_s<std::decay_t<decltype(rp{}())>>::type;
    
    template<typename fa, typename F, typename T>
    constexpr auto temporarily_executed(const F& f, const result_pair<T>& pf){
	unique_ptr<fa> allocator{new fa()};
	auto &&res = f(*allocator,*pf.result);
	//the previous results might be incorporated into the new
	//results, so we should preserve the allocator if we can.
	struct keep_allocator : destructable{
	    const unique_ptr<destructable> old_allocator;
	    const unique_ptr<destructable> new_allocator;
	    
	};
	return result_pair<std::decay_t<decltype(*res)>>{
	    std::move(res),new keep_allocator(std::move(res.allocator),std::move(allocator))};
    }

    template<typename fa, typename F>
    constexpr auto temporarily_executed(const F& f = F{}){
	unique_ptr<fa> allocator{new fa()};
	auto &&res = f(*allocator);
	return result_pair<std::decay_t<decltype(*res)>>{
	    std::move(res),std::move(allocator)};
    }

    	template<typename... T>
	struct typespace;

    template<typename T> struct is_typespace : public std::false_type{};
    template<typename... T> struct is_typespace<typespace<T...>> : public std::true_type{};

    template<typename T>
    concept Typespace = is_typespace<T>::value;

    //specialized actions are functions that can take Allocators of specific info
    template<typename U, typename ts, typename T, typename ts::ThisInfo info>
    concept SpecializedAction = Typespace<ts> &&
	std::regular_invocable<U,typename ts::template allocator<info>&, const T&>;

    //actions are functions that can take Allocators generically on their info parameters
    template<typename U, typename ts, typename T>
    concept Action = Typespace<ts> && SpecializedAction<U,ts,T,ts::empty_info()> &&
	SpecializedAction<U,ts,T,ts::ThisInfo::set_all_to_constant(__COUNTER__ + 1)>;

	template<typename... T>
	struct typespace{
	    using ts = typespace;
	    using ThisInfo = Info<T...>;

	    static constexpr ThisInfo empty_info(){return ThisInfo{};}

    
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

	    using base_allocator = allocator<ThisInfo{}>;
	    
	    template<PackMember<T...> U, ThisInfo info, AllocatorThunk prev_stage_f,
		     Action<ts,result<prev_stage_f>> F>
	    struct execution_result{
		allocator<info> allocations;
		allocated_ptr<U> result;

		using ret_t = U;
		using allocator_t = allocator<info>;

	    private:
		template<typename prev_stage_result>
		constexpr execution_result(const F& f, result_pair<prev_stage_result>&& r)
		    :result(f(allocations,*r.result)){
		    //delete this b/c there's no safe way to keep it.
		    //REMEMBER! FINAL STAGE CANNOT USE OLD REFS! MUST COPY!
		    r.result = nullptr;
		}

	    public:
		constexpr execution_result(const F& f = F{}, const prev_stage_f& pf = prev_stage_f{})
		    :execution_result(f, pf()){}
		
	      constexpr ~execution_result() {
		//want to make extra sure this is empty before we try
		//to do any allocator destruction!
		result.clear();
	      }
	    };

	    template<AllocatorThunk previous_stages, Action<ts,result<previous_stages>> This_Stage,
		     PackMember<T...> this_stage_result>
	    static constexpr ThisInfo get_execution_info(){
		ThisInfo info;
		{
		    execution_result<this_stage_result,ThisInfo{},previous_stages, This_Stage> result;
		    info.advance_to(result.allocations.new_info);
		}
		return info;		
	    }
	    
	    template<AllocatorThunk previous_stages, Action<ts,result<previous_stages>> This_Stage,
		     PackMember<T...> this_stage_result>
	    using constexpr_executed = execution_result<this_stage_result,
		get_execution_info<previous_stages,This_Stage,this_stage_result>(),
		This_Stage,previous_stages>; //*/

	};
}
