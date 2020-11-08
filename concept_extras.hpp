#pragma once
#include <concepts>

template<typename T, typename... U>
concept PackMember = (std::is_same_v<T,U> || ... || false);
