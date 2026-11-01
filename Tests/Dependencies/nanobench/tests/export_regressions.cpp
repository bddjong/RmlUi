#define ANKERL_NANOBENCH_IMPLEMENT
#include "../nanobench.h"

#include <cassert>
#include <sstream>
#include <stdexcept>

int main() {
    using namespace ankerl::nanobench;
    std::ostringstream out;
    bool threw = false;
    try {
        render("{{title}}", std::vector<Result>{}, out);
    } catch (std::runtime_error const&) {
        threw = true;
    }
    assert(threw);
    out.str("");
    render(templates::json(), std::vector<Result>{}, out);
    assert(out.str().find("\"results\": [") != std::string::npos);

    Config config;
    config.mBenchmarkTitle = "quote\" slash\\ newline\n</script>";
    config.mBenchmarkName = "single'\"";
    config.mUnit = "u\"nit";
    Result result(config);
    detail::PerformanceCounters counters;
    result.add(std::chrono::nanoseconds(100), 1, counters);
    std::vector<Result> results{result};
    out.str("");
    render(templates::json(), results, out);
    assert(out.str().find("quote\\\" slash\\\\ newline\\u000a\\u003c/script>") != std::string::npos);
    assert(out.str().find("u\\\"nit") != std::string::npos);
    out.str("");
    render(templates::pyperf(), results, out);
    assert(out.str().find("quote\\\" slash\\\\ newline\\u000a\\u003c/script>") != std::string::npos);
    out.str("");
    render(templates::htmlBoxplot(), results, out);
    assert(out.str().find("name: \"single'\\\"\"") != std::string::npos);
    assert(out.str().find("\\u003c/script>") != std::string::npos);
    out.str("");
    render(templates::csv(), results, out);
    assert(out.str().find("\"u\"\"nit\"") != std::string::npos);
    assert(out.str().find("quote\"\" slash\\ newline\n</script>") != std::string::npos);
    out.str("");
    render("{{title}}", results, out);
    assert(out.str() == config.mBenchmarkTitle);
}
