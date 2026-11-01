// Standalone regression executable for the bundled doctest implementation.
#define DOCTEST_CONFIG_IMPLEMENT
#include "../doctest.h"
#include <cassert>
#include <sstream>
#include <string>

TEST_CASE("empty throws-with must fail") { CHECK_THROWS_WITH((void)0, ""); }
TEST_CASE("expected failure" * doctest::should_fail()) { CHECK(false); }
TEST_CASE("missing expected failure" * doctest::should_fail()) { CHECK(true); }

TEST_CASE("junit count with no assertions") {}
TEST_CASE("junit count with subcases") {
    SUBCASE("first") { CHECK(true); CHECK(true); }
    SUBCASE("second") { CHECK(true); }
}

int main() {
    for (const char* value : {"abc", "abcdefghijklmnopqrstuvwxyz0123456789"}) {
        doctest::String s(value);
        s += s;
        assert(std::string(s.c_str()) == std::string(value) + value);
    }
    const doctest::String s("abcd");
    assert(std::string(s.substr(1).c_str()) == "bcd");
    assert(std::string(s.substr(99).c_str()).empty());
    assert(std::string(doctest::String("abcd").substr(1).c_str()) == "bcd");
    assert(std::string(doctest::String("abcd").substr(99).c_str()).empty());
    std::ostringstream xml;
    xml << doctest::XmlEncode("]]>");
    assert(xml.str() == "]]&gt;");
    {
        doctest::Context context;
        std::ostringstream output;
        context.setCout(&output);
        context.addFilter("test-case", "empty throws-with must fail");
        assert(context.run() != 0);
    }
    {
        doctest::Context context;
        std::ostringstream output;
        context.setCout(&output);
        context.setOption("reporters", "junit");
        context.addFilter("test-case", "junit count*");
        assert(context.run() == 0);
        const std::string report = output.str();
        assert(report.find("tests=\"3\"") != std::string::npos);
        size_t count = 0, pos = 0;
        while ((pos = report.find("<testcase ", pos)) != std::string::npos) { ++count; ++pos; }
        assert(count == 3);
    }
    {
        const char high[] = {static_cast<char>(0xff), 0};
        assert(doctest::stricmp(high, high) == 0);
        assert(doctest::stricmp("a", "A") == 0);
    }
    for (const char* name : {"expected failure", "missing expected failure"}) {
        doctest::Context context;
        std::ostringstream output;
        context.setCout(&output);
        context.setOption("reporters", "junit");
        context.addFilter("test-case", name);
        const bool expected_success = std::string(name) == "expected failure";
        assert((context.run() == 0) == expected_success);
        assert((output.str().find("<failure") == std::string::npos) == expected_success);
    }
}
