#pragma once
#include <type_traits>
#include <concepts>
#include <compare>
#include <cassert>
#include <utility>
#include "concept_extras.hpp"
#include "allocator_info.hpp"

namespace compile_time::allocator{
    	template<typename T>
	struct unique_ptr{
	    T* ptr{nullptr};

	    constexpr unique_ptr() = default;
	    constexpr unique_ptr(T* ptr):ptr(ptr){}
	    constexpr unique_ptr(const unique_ptr&) = delete;
	    constexpr unique_ptr(unique_ptr&& o):ptr(o.ptr){o.ptr = nullptr;}
	    
	    constexpr T& operator*(){
		return *ptr;
	    }

	    constexpr const T& operator*() const {
		return *ptr;
	    }

	    constexpr T* operator->(){
		return ptr;
	    }

	    constexpr const T* operator->() const {
		return ptr;
	    }

	    constexpr operator bool() const { return ptr; }

	    constexpr unique_ptr& operator=(const unique_ptr&) = delete;
	    
	    constexpr unique_ptr& operator=(unique_ptr&& o){
		if (ptr) delete ptr;
		ptr = o.ptr;
		o.ptr = nullptr;
		return *this;
	    }
	    
	    constexpr ~unique_ptr(){
		if (ptr) delete ptr;
	    }

	    constexpr void clear(){}
	};

    template<typename U, std::size_t amnt> struct single_allocator;
	template<typename U> struct single_allocator<U, 0>{

	    //initial allocations.  Find and track high water mark.

	    struct allocated_list_node{
		constexpr allocated_list_node() = default;
		constexpr ~allocated_list_node() = default;
		unique_ptr<U> this_payload;
		unique_ptr<allocated_list_node> next;
		constexpr allocated_list_node(unique_ptr<U> this_payload,
					      unique_ptr<allocated_list_node> next)
		    :this_payload(std::move(this_payload)),
		     next(std::move(next)){}
	    };

	    unique_ptr<allocated_list_node> allocated_list;
	    
	    template<typename ThisInfo, typename... Args>
	    constexpr U* alloc(ThisInfo &info, Args && ...args) {
		auto &this_info = info.template single<U>();
		this_info.current_amount++;
		if (this_info.current_amount > this_info.high_water_mark){
		    this_info.high_water_mark = this_info.current_amount;
		}
		allocated_list = new allocated_list_node(new U(std::forward<Args>(args)...),
						    std::move(allocated_list));
		return allocated_list->this_payload.ptr;
	    }

	    template<typename ThisInfo>
	    constexpr void dealloc(ThisInfo &info, U* tofree){
		auto *curr = &allocated_list;
		info.template single<U>().current_amount--;
		if (*curr){
		    if (!(*curr)->next){
			assert((*curr)->this_payload.ptr == tofree);
			curr->operator=(nullptr);
			return;
		    }
		    for (; (*curr)->next; curr = &(*curr)->next){
			if ((*curr)->next->this_payload.ptr == tofree){
			    (*curr)->next.operator=(std::move((*curr)->next->next));
			    //this should autofree.
			    return;
			}
		    }
		}
		assert(false && "Fatal error! Could not find allocated node to free.");
	    }

	    constexpr void clear(){
		allocated_list.operator=(nullptr);
	    }
	    
	};
}
