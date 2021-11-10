#pragma once
#include <type_traits>
#include <concepts>
#include <compare>
#include <cassert>
#include <utility>
#include "concept_extras.hpp"
#include "allocator_info.hpp"
#include "smart_pointers.hpp"

namespace compile_time::allocator {

 

    template<typename U, std::size_t amnt> struct single_allocator;
	template<typename U> struct single_allocator<U, 0>{
	    //initial allocations.  Find and track high water mark.

	  struct allocated_list_node : public destructor<U>{
	    
		constexpr allocated_list_node() = default;
		constexpr ~allocated_list_node() = default;
	    single_info<U> &info;
		unique_ptr<U> this_payload;
		unique_ptr<allocated_list_node> next;
	      unique_ptr<allocated_list_node> *owner;
	    constexpr allocated_list_node(single_info<U> &info,
					  unique_ptr<U> this_payload,
					  unique_ptr<allocated_list_node> next,
					  unique_ptr<allocated_list_node> *owner)
	      :info(info),
	       this_payload(std::move(this_payload)),
	       next(std::move(next)),
	       owner(owner)
	      {}
	      
	    constexpr void destroy(U* tofree){
	      assert(info.current_amount > 0);
	      assert(info.current_amount < 1239);
	      info.current_amount--;
	      assert(this_payload.ptr == tofree);
	      assert(owner->ptr = this);
	      if (next){
		next->owner = owner;
		//this will destroy the *current object*
		assert(owner != &next);
		owner->operator=(std::move(next));
	      }
	      else{
		//this will destroy the *current object*
		owner->clear();
	      }
	    }
	    
	  };

	  unique_ptr<allocated_list_node> allocated_list;

	    template<typename ThisInfo, typename... Args>
	    constexpr allocated_ptr<U> alloc(ThisInfo &info, Args && ...args) {
		auto &this_info = info.template single<U>();
		this_info.current_amount++;
		if (this_info.current_amount > this_info.high_water_mark){
		    this_info.high_water_mark = this_info.current_amount;
		}
		allocated_list.operator=(
		  new allocated_list_node(this_info,new U(std::forward<Args>(args)...),
					  std::move(allocated_list),&allocated_list));
		if (allocated_list->next){
		    allocated_list->next->owner = &allocated_list->next;
		}
		return allocated_ptr<U>{allocated_list->this_payload.ptr,allocated_list.ptr};
	    }

	    template<typename ThisInfo>
	    constexpr allocated_ptr<U> rewrap(U* tofree){
		for (auto &curr = allocated_list; curr; curr = curr->next){
		    if (curr->this_payload.ptr == tofree){
		      return allocated_ptr<U>{tofree,curr.ptr};
		    }
		}
		assert(false && "Fatal error! Could not find allocated node.");
	    }

	    constexpr void clear(){
		allocated_list.operator=(nullptr);
	    }
	};

    	template<typename U, std::size_t amnt> struct single_allocator{
	    //the exact-size case

	  struct free_list_node : public destructor<U>{
	      free_list_node* next {nullptr};
	      U this_payload;
	      single_allocator *parent{nullptr};
	      constexpr free_list_node() = default;
	      constexpr ~free_list_node() = default;

	    constexpr void destroy(U* _this){
	      assert(_this == &this_payload);
	      assert(parent);
	      next = parent->free_list;
	      parent->free_list = this;
	    }
	    };
	    
	    free_list_node free_list_storage[amnt] = {free_list_node{}};
	    //remember: guaranteed amnt > 0
	    free_list_node *free_list = &free_list_storage[0];
	    
	    constexpr single_allocator(){
		for (auto i = 0u; i + 1 < amnt; ++i){
		    free_list_storage[i].next = &free_list_storage[i+1];
		    free_list_storage[i].parent = this;
		}
	    }

	    template<typename... Args>
	    constexpr allocated_ptr<U> alloc(auto&&, Args && ...args) {
	      assert(free_list);
	      /*
		decided not to do the fancy version where we
		explicitly know which nodes are destined to survive,
		so there should always be enough pre-reserved space
		here :)
	      */
	      auto &selected_free_ln = *free_list;
	      free_list = selected_free_ln.next;
	      selected_free_ln.this_payload = U{std::forward<Args>(args)...};
	      return allocated_ptr<U>{&selected_free_ln.this_payload,&selected_free_ln};
	    }

	    constexpr allocated_ptr<U> rewrap(U* tofree){
		for (auto &potential_freed_node : free_list_storage){
		    if (tofree == &potential_freed_node.this_payload){
		      return allocated_ptr<U>{tofree,&potential_freed_node};
		    }
		}
	    }
	    
	};
}
