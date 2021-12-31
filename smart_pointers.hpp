#pragma once
#include <iostream>
#include <type_traits>
#include <concepts>

namespace compile_time{
    	template<typename T>
	struct unique_ptr{
	    T* ptr{nullptr};

	    constexpr unique_ptr() = default;
	    constexpr unique_ptr(T* ptr):ptr(ptr){}
	    constexpr unique_ptr(const unique_ptr&) = delete;
	    constexpr unique_ptr(unique_ptr&& o):ptr(o.ptr){o.ptr = nullptr;}
	    
	    template<std::derived_from<T> U>
	    constexpr unique_ptr(unique_ptr<U> &&o):ptr(o.ptr){o.ptr = nullptr;}
	    
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

	  constexpr void clear(){
	    operator=(nullptr);
	  }
	    
	    constexpr ~unique_ptr(){
		if (ptr) delete ptr;
	    }
	};

//trouble here: we want to be able to recursively destruct, which
//means an argument-free destructor; but in order to do that, we need
//to capture enough info to actualy *do* the destruction. It's very
//unclear if that info would have been constexpr in the first place.

    //Best solution: turns out it's constexpr and we do capture it. 
    
    namespace allocator {

      template<typename T> struct destructor{
	  //DEBUG
	  bool alive{true};
	constexpr virtual void destroy(T* t) = 0;
	  //constexpr virtual void share(T* t) = 0;
	  constexpr virtual ~destructor() {
	      alive = false;
	  }
      };

	template<typename Sub, typename Super> concept subtype_destroying =
	    requires(Super &t, destructor<Sub> &d) { t.destroy(d); };
	
	template<typename U, std::derived_from<U> T> requires subtype_destroying<T,U>
	struct subclass_destructor : public destructor<U>,
				     public destructor<T> {
	    destructor<T> &destroyer;
	    constexpr subclass_destructor(destructor<T> &destroyer):destroyer(destroyer){}
	    constexpr void destroy(T* t){
		destroyer.destroy(t);
	    }
	    constexpr void destroy(U* u){
		assert(u);
		u->destroy(destroyer);
		delete this;
	    }
	    constexpr ~subclass_destructor() = default;
	    
	};

	template<typename T>
	struct allocated_ptr{
	  T* ptr{nullptr};
	  destructor<T> *destroyer{nullptr};

	    constexpr allocated_ptr() = default;
	  constexpr allocated_ptr(std::nullptr_t):allocated_ptr(){}
	  constexpr allocated_ptr(T* ptr, destructor<T>* destroyer):ptr(ptr),destroyer(destroyer){}
	    constexpr allocated_ptr(const allocated_ptr&) = delete;
	  constexpr allocated_ptr(allocated_ptr&& o):ptr(o.ptr),destroyer(o.destroyer){
	    o.ptr = nullptr;
	    o.destroyer = nullptr;
	  }

	    template<std::derived_from<T> U>
	    constexpr allocated_ptr(allocated_ptr<U> &&o):
		ptr(o.ptr),
		destroyer((o.destroyer ? 
			   new subclass_destructor<T,U>{*o.destroyer}
			   : nullptr)){
		o.ptr = nullptr;
		o.destroyer = nullptr;
	    }
	    
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

	  constexpr operator bool() const {
	    assert(ptr ? destroyer : true);
	    assert(destroyer ? ptr : true);
	    return ptr;
	  }

	    constexpr allocated_ptr& operator=(const allocated_ptr&) = delete;

	  constexpr void clear(){
	    if (ptr){
	      assert(destroyer);
	      assert(destroyer->alive);
	      auto *sptr = ptr;
	      auto *sdestroyer = destroyer;
	      ptr = nullptr;
	      destroyer = nullptr;
	      sdestroyer->destroy(sptr);
	    }
	    else {assert (!destroyer);}
	  }
	    
	  constexpr allocated_ptr& operator=(allocated_ptr&& o){
	    clear();
	    ptr = o.ptr;
	    destroyer = o.destroyer;
	    o.ptr = nullptr;
	    o.destroyer = nullptr;
	    return *this;
	  }
	  
	  constexpr ~allocated_ptr(){
	    clear();
	  }

	};
    }

}
