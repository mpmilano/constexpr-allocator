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

    namespace allocator {
	
    }

}
