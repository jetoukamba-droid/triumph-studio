#pragma once

#include <cstdlib>
#include <iostream>

namespace triumph::test
{
inline void require (bool condition,
                     const char* expression,
                     const char* file,
                     int line)
{
    if (condition)
        return;

    std::cerr << file << ':' << line << ": check failed: " << expression << '\n';
    std::exit (EXIT_FAILURE);
}
}

#define REQUIRE(expression) \
    ::triumph::test::require (static_cast<bool> (expression), #expression, __FILE__, __LINE__)
