// Standalone regression for assertions evaluated with doctest disabled.
#define DOCTEST_CONFIG_DISABLE
#define DOCTEST_CONFIG_EVALUATE_ASSERTS_EVEN_WHEN_DISABLED
#define DOCTEST_CONFIG_ASSERTS_RETURN_VALUES
#include "../doctest.h"
#include <cassert>
#include <stdexcept>

int main() {
    int evaluated = 0;
    assert(DOCTEST_CHECK_THROWS_MESSAGE((++evaluated, throw std::runtime_error("x")), "message"));
    assert(evaluated == 1);
    assert(DOCTEST_CHECK_THROWS_AS_MESSAGE(throw std::runtime_error("x"), const std::runtime_error&, "message"));
    assert(DOCTEST_CHECK_NOTHROW_MESSAGE(++evaluated, "message"));
    assert(evaluated == 2);
}
