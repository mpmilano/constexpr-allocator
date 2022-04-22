#pragma once
#include <concepts>

template<typename T, typename... U>
concept PackMember = (std::is_same_v<T,U> || ... || false);
template<typename T> concept Thunk = std::regular_invocable<T>;
//regular_invocable vs invocable
