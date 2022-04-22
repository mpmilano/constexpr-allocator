#include <type_traits>
#include "smart_pointers.hpp"
#include <cassert>

namespace compile_time{
    namespace data{

	struct tree_node;
	struct leaf;
	
	struct tree{
	    const std::size_t payload{0};
	    constexpr virtual ~tree() = default;
	    constexpr virtual tree_node const * const as_node() const = 0;
	    constexpr virtual leaf const* const as_leaf() const = 0;
	    constexpr tree(std::size_t payload):payload(payload){}
	};
	
	struct tree_node : public tree {
	    unique_ptr<tree> left;
	    unique_ptr<tree> right;
	    constexpr tree_node(std::size_t payload,
				unique_ptr<tree> left,
				unique_ptr<tree> right)
		:tree(payload),left(std::move(left)),right(std::move(right)){}

	    constexpr tree_node const* const as_node() const override {
		return this;
	    }
	    constexpr leaf const* const as_leaf() const override {
		return nullptr;
	    }
	};

	struct leaf : public tree {
	    constexpr leaf(std::size_t payload):tree(payload){}
	    constexpr tree_node const* const as_node() const override {
		return nullptr;
	    }
	    constexpr leaf const* const as_leaf() const override {
		return this;
	    }
	};
	
	constexpr unique_ptr<tree> sample_tree (){
	    return new tree_node(1, new leaf(2), new leaf(3));
	}
    }
    
    namespace types{
	
	
	template<std::size_t payload> struct leaf;
	template<typename T> struct is_leaf : std::false_type{};
	template<std::size_t payload> struct is_leaf<leaf<payload>> : std::true_type{};
	template<typename T> concept Leaf = is_leaf<T>::value;
	
	template<typename T> struct is_tree_node : std::false_type{};
	template<typename T> concept TreeNode = is_tree_node<T>::value;
	
	template<typename T> concept Tree = Leaf<T> || TreeNode<T>;
    
	template<std::size_t payload, Tree left, Tree right> struct tree_node;
    }
    
    namespace fixed_size{

	struct tree_common{
	    std::size_t payload{0};
	    bool valid{false};
	    bool leaf{false};
	};
	
	template<std::size_t s> struct tree;
	template<> struct tree<0> : public tree_common {};
	template<std::size_t s> struct tree : public tree_common{
	    tree<s-1> left;
	    tree<s-1> right;
	};
	
	template<std::size_t size>
	constexpr tree<size> serialize(const data::tree& t) {
	    tree<size> end;
	    end.payload = t.payload;
	    if (auto *leaf = t.as_leaf()){
		end.leaf = true;
		end.valid = true;
	    }
	    else if (auto *node = t.as_node()) {
		if constexpr (size){
		    end.valid = true;
		    end.left = serialize<size-1>(*node->left);
		    end.right = serialize<size-1>(*node->right);
		}
	    }
	    return end;
	}
    }
    
    namespace data{
	
	template<std::size_t size>
	constexpr unique_ptr<tree> deserialize(const fixed_size::tree<size> &in){
	    assert(in.valid);
	    if (in.leaf) return new leaf(in.payload);
	    else if constexpr (size) return new tree_node(in.payload, deserialize(in.left), deserialize(in.right));
	    assert(false);
	}
    }
    
    namespace types{

	template<std::size_t size, fixed_size::tree<size> t>
	auto* deserialize_f(){
	    if constexpr (t.valid){
		if constexpr (t.leaf) return (leaf<t.payload>*) nullptr;
		else {
		    auto *left = deserialize_f<size-1,t.left>();
		    auto *right = deserialize_f<size-1,t.right>();
		    using left_t = std::decay_t<decltype(*left)>;
		    using right_t = std::decay_t<decltype(*right)>;
		    return (tree_node<t.payload,left_t,right_t>*) nullptr;
		}
	    }
	}

	template<std::size_t size, fixed_size::tree<size> t>
	using deserialize = std::decay_t<decltype(*deserialize_f<size,t>())>;
	
    }
}

using namespace compile_time;
    
int main(){
    constexpr auto serialized = fixed_size::serialize<4>(*data::sample_tree());
    using ret = types::deserialize<4,serialized>;
    ret::print();
}
