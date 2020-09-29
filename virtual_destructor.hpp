#pragma once

namespace compile_time {
struct virtual_destructor {
  constexpr virtual ~virtual_destructor() = default;
};
} // namespace compile_time