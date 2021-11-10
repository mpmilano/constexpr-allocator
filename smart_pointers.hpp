#pragma once

namespace compile_time{
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

//trouble here: we want to be able to recursively destruct, which
//means an argument-free destructor; but in order to do that, we need
//to capture enough info to actualy *do* the destruction. It's very
//unclear if that info would have been constexpr in the first place.

    //Best solution: turns out it's constexpr and we do capture it. 
    
    namespace allocator {

      template<typename T> struct destructor{
	constexpr virtual void destroy(T* t) = 0;
	constexpr virtual ~destructor() = default;
      };
	
	template<typename T>
	struct allocated_ptr{
	  T* ptr{nullptr};
	  destructor *destroyer{nullptr};

	    constexpr allocated_ptr() = default;
	  constexpr allocated_ptr(T* ptr, destructor* destroyer):ptr(ptr),destroyer(destroyer){}
	    constexpr allocated_ptr(const allocated_ptr&) = delete;
	  constexpr allocated_ptr(allocated_ptr&& o):ptr(o.ptr),destroyer(o.destroyer){
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
	      destroyer->destroy(ptr);
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

	}
    }

}
