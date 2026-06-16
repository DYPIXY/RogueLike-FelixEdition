#include "lib.hpp"

auto main() -> int
{
  auto const lib = library {};

  return lib.name == "m3" ? 0 : 1;
}
