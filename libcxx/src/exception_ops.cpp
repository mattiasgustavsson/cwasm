// Minimal std::exception operations for -fno-exceptions builds (no exception.cpp).
#include <exception>

namespace std {

int uncaught_exceptions() noexcept { return 0; }

bool uncaught_exception() noexcept { return false; }

} // namespace std
