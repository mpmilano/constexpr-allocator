#pragma once
#include <type_traits>
#include <concepts>
#include <compare>
#include <cassert>
#include <utility>
#include "concept_extras.hpp"
#include "allocator_info.hpp"

namespace compile_time::allocator{
    	template<typename U, std::size_t amnt> struct single_allocator{
	    //the exact-size case

	    struct free_list_node{
		free_list_node* next = nullptr;
		U this_payload;
		constexpr free_list_node() = default;
		constexpr ~free_list_node() = default;
	    };
	    
	    free_list_node free_list_storage[amnt] = {free_list_node{}};
	    //remember: guaranteed amnt > 0
	    free_list_node *free_list = &free_list_storage[0];
	    
	    constexpr single_allocator(){
		for (auto i = 0u; i + 1 < amnt; ++i){
		    free_list_storage[i].next = &free_list_storage[i+1];
		}
	    }

	    //arena allocations
	    template<typename... Args>
	    constexpr free_list_node* _alloc(Args && ...args){
		assert(free_list); /*
				     decided not to do the fancy
				     version where we explicitly know
				     which nodes are destined to
				     survive, so there should always
				     be enough pre-reserved space here
				     :)
				   */
		
		auto &selected_free_ln = *free_list;
		free_list = selected_free_ln.next;
		selected_free_ln.this_payload = U{std::forward<Args>(args)...};
		return &selected_free_ln;
	    }

	    template<typename... Args>
	    constexpr U* alloc(auto&&, Args && ...args) {
		return &_alloc(std::forward<Args>(args)...)->this_payload;
	    }

	    constexpr void _dealloc(free_list_node* tofree){
		tofree->next = free_list;
		free_list = tofree;
	    }

	    constexpr void dealloc(auto&&, U* tofree){
		//inefficient, probably, but w/e can't do better
		//without fancy pointers.
		for (auto &potential_freed_node : free_list_storage){
		    if (tofree == &potential_freed_node.this_payload){
			_dealloc(&potential_freed_node);
		    }
		}
	    }
	    
	};
}
