#pragma once
#include <type_traits>
#include <concepts>
#include <compare>
#include <cassert>
#include <utility>
#include "concept_extras.hpp"


namespace compile_time::allocator{

	template<typename T>
	struct single_info{
	    std::size_t current_amount = 0;
	    std::size_t high_water_mark = 0;
	    constexpr single_info& single(){
		return *this;
	    }

	    constexpr const single_info& single() const {
		return *this;
	    }
	    constexpr single_info() = default;
	    constexpr single_info(single_info&&) = default;
	    constexpr void advance_to(const single_info &i){
		current_amount = i.current_amount;
		high_water_mark = i.high_water_mark;
	    }

	protected:
	    constexpr single_info(const single_info&) = default;
	};
	
	template<typename... T>
	struct Info : public single_info<T>... {
	    
	    template<PackMember<T...> U>
	    constexpr std::size_t max() const {
		return single_info<U>::single().high_water_mark;
	    }

	    template<PackMember<T...> U>
	    constexpr const auto& single() const {
		return single_info<U>::single();
	    }

	    template<PackMember<T...> U>
	    constexpr auto& single() {
		return single_info<U>::single();
	    }

	    constexpr std::size_t maxes() const {
		return ((single<T>().high_water_mark) + ...);
	    }

	    constexpr void advance_to(const Info& i) {
		(single_info<T>::advance_to(i),...);
	    }

	    constexpr Info() = default;
	    constexpr Info(Info&&) = default;

	    constexpr Info(const Info&) = default;

	};    
    
}
