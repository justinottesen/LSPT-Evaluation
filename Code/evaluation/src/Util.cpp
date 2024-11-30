#include "Util.h"

#include <array>
#include <cstring>
#include <string>

static constexpr unsigned int STRERROR_BUFFER_SIZE = 128;

std::string my_strerror(int errnum) {
  std::array<char, STRERROR_BUFFER_SIZE> buf{};
  // NOLINTNEXTLINE(misc-include-cleaner): Gets stuck in an include loop
  return strerror_r(errnum, buf.data(), STRERROR_BUFFER_SIZE);
}
