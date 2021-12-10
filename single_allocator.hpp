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

	  struct plain_delete : public destructor<U>{
	      single_info<U> *this_info{nullptr};
	    	    
	      constexpr plain_delete() = default;
	      constexpr ~plain_delete() = default;
	      
	    constexpr void destroy(U* tofree){
	      this_info->current_amount--;
	      delete tofree;
	    }
	    
	  };

	  plain_delete deleter;

	    template<typename ThisInfo, typename... Args>
	    constexpr allocated_ptr<U> alloc(ThisInfo &info, Args && ...args) {
		single_info<U> &this_info = info;
		if (!deleter.this_info) deleter.this_info = &this_info;
		this_info.current_amount++;
		if (this_info.current_amount > this_info.high_water_mark){
		    this_info.high_water_mark = this_info.current_amount;
		}
		return allocated_ptr<U>{
		  new U(std::forward<Args>(args)...),
		  &deleter
		};
	    }

	    template<typename ThisInfo>
	    constexpr allocated_ptr<U> rewrap(U* tofree){
	      return allocated_ptr<U>{tofree,deleter.ptr};
	    }

	};

    	template<typename U, std::size_t amnt> struct single_allocator{
	    //the exact-size case

	  struct free_list_node : public destructor<U>{
	      free_list_node* next {nullptr};
	      U this_payload;
	      std::size_t refcount{0};
	    free_list_node ** free_list{nullptr};
	    constexpr free_list_node() = default;
	    constexpr free_list_node(const free_list_node&) = delete;
	    constexpr free_list_node(free_list_node&&) = delete;
	      constexpr ~free_list_node() = default;

	    constexpr void destroy(U* _this){
	      assert(_this == &this_payload);
	      if (refcount == 0){
		  assert(free_list);
		  next = *free_list;
		  *free_list = this;
	      } else --refcount;
	    }

	      constexpr void share(U* _this){
		  assert(_this == &this_payload);
		  ++refcount;
	      }
	      
	    };
	    
	  free_list_node free_list_storage[amnt];
	    //remember: guaranteed amnt > 0
	    free_list_node *free_list = &free_list_storage[0];
	    
	    constexpr single_allocator(){
		for (auto i = 0u; i + 1 < amnt; ++i){
		    free_list_storage[i].next = &free_list_storage[i+1];
		}
		for (auto &ln : free_list_storage){
		  ln.free_list = &free_list;
		}
	    }

	    constexpr single_allocator(const single_allocator&) = delete;
	    constexpr single_allocator(single_allocator&&) = delete;
	    constexpr ~single_allocator() = default;

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
	      selected_free_ln.this_payload = U(std::forward<Args>(args)...);
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
