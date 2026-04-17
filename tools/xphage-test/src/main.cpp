// ============================================================
// xphage-test — Test Runner v3.5.0
//
// Runs .xp0 test files from tests/run-pass/ and compile-fail/
// Supports: parallel execution, colored output, filter by name
//
// Usage:
//   xphage-test [--filter <name>] [--jobs N] [--verbose]
//   xphage-test tests/run-pass/hello.xp0
// ============================================================
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <thread>
#include <mutex>
#include <atomic>

namespace fs = std::filesystem;

// ── Colors ───────────────────────────────────────────────────
static const char* GREEN  = "\033[1;32m";
static const char* RED    = "\033[1;31m";
static const char* YELLOW = "\033[1;33m";
static const char* CYAN   = "\033[1;36m";
static const char* DIM    = "\033[2m";
static const char* NC     = "\033[0m";

// ── Test result ──────────────────────────────────────────────
enum class TestStatus { Pass, Fail, Skip, Timeout };

struct TestResult {
    std::string  name;
    std::string  path;
    TestStatus   status   = TestStatus::Fail;
    double       elapsed  = 0.0;
    std::string  output;
    bool         expected_fail = false;
};

// ── Config ───────────────────────────────────────────────────
struct Config {
    std::string tests_dir   = "tests";
    std::string filter;
    int         jobs        = 4;
    int         timeout_sec = 30;
    bool        verbose     = false;
    bool        no_color    = false;
    std::vector<std::string> specific_files;
};

// ── Run a single test ────────────────────────────────────────
static TestResult run_test(const std::string& path,
                            bool expected_fail,
                            const Config& cfg) {
    TestResult res;
    res.path          = path;
    res.expected_fail = expected_fail;

    // Name = relative path without extension
    fs::path p(path);
    res.name = p.stem().string();

    auto t0 = std::chrono::steady_clock::now();

    // Find xphage binary
    std::string xphage = "./build/xphage";
    if (!fs::exists(xphage)) xphage = "xphage";

    // Run: xphage run <file> and capture output
    std::string tmp_out = "/tmp/xptest_" + res.name + ".out";
    std::string cmd = xphage + " run " + path
                    + " >" + tmp_out + " 2>&1";

    // Timeout wrapper
    std::string full_cmd;
#ifdef _WIN32
    full_cmd = cmd;
#else
    full_cmd = "timeout " + std::to_string(cfg.timeout_sec) + " " + cmd;
#endif

    int exit_code = std::system(full_cmd.c_str());

    auto t1 = std::chrono::steady_clock::now();
    res.elapsed = std::chrono::duration<double, std::milli>(t1 - t0).count();

    // Read output
    std::ifstream out_f(tmp_out);
    if (out_f.is_open()) {
        std::ostringstream ss; ss << out_f.rdbuf();
        res.output = ss.str();
    }
    std::remove(tmp_out.c_str());

    // Check timeout (exit code 124 from timeout)
    if (exit_code == 124) {
        res.status = TestStatus::Timeout;
        return res;
    }

    bool succeeded = (exit_code == 0);

    // For compile-fail tests: success means it FAILED to run (as expected)
    if (expected_fail) {
        res.status = !succeeded ? TestStatus::Pass : TestStatus::Fail;
    } else {
        res.status = succeeded ? TestStatus::Pass : TestStatus::Fail;
    }

    return res;
}

// ── Collect test files ────────────────────────────────────────
static std::vector<std::pair<std::string, bool>> collect_tests(
        const Config& cfg) {

    std::vector<std::pair<std::string, bool>> tests; // {path, expected_fail}

    if (!cfg.specific_files.empty()) {
        for (auto& f : cfg.specific_files)
            tests.emplace_back(f, false);
        return tests;
    }

    std::error_code ec;
    auto collect = [&](const std::string& dir, bool exp_fail) {
        if (!fs::is_directory(dir, ec)) return;
        for (auto& entry : fs::recursive_directory_iterator(dir, ec)) {
            if (!entry.is_regular_file()) continue;
            if (entry.path().extension() != ".xp0") continue;
            std::string name = entry.path().stem().string();
            if (!cfg.filter.empty() &&
                name.find(cfg.filter) == std::string::npos) continue;
            tests.emplace_back(entry.path().string(), exp_fail);
        }
    };

    collect(cfg.tests_dir + "/run-pass",     false);
    collect(cfg.tests_dir + "/compile-fail", true);

    std::sort(tests.begin(), tests.end(),
        [](auto& a, auto& b){ return a.first < b.first; });

    return tests;
}

// ── Print result line ─────────────────────────────────────────
static std::mutex print_mutex;

static void print_result(const TestResult& r, const Config& cfg) {
    std::lock_guard<std::mutex> lock(print_mutex);

    const char* status_str;
    const char* color;

    switch (r.status) {
        case TestStatus::Pass:
            status_str = "PASS"; color = GREEN; break;
        case TestStatus::Fail:
            status_str = "FAIL"; color = RED; break;
        case TestStatus::Skip:
            status_str = "SKIP"; color = YELLOW; break;
        case TestStatus::Timeout:
            status_str = "TIMEOUT"; color = RED; break;
    }

    if (cfg.no_color) {
        std::cout << "[" << status_str << "] " << r.name
                  << " (" << r.elapsed << " ms)\n";
    } else {
        std::cout << color << "[" << status_str << "]" << NC
                  << " " << r.name
                  << DIM << " (" << r.elapsed << " ms)" << NC << "\n";
    }

    if (cfg.verbose || r.status == TestStatus::Fail) {
        if (!r.output.empty()) {
            std::cout << DIM << "  output:\n";
            std::istringstream ss(r.output);
            std::string line;
            while (std::getline(ss, line))
                std::cout << "    " << line << "\n";
            std::cout << NC;
        }
    }
}

// ── Main ─────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    Config cfg;

    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--filter") && i + 1 < argc)
            cfg.filter = argv[++i];
        else if (!std::strcmp(argv[i], "--jobs") && i + 1 < argc)
            cfg.jobs = std::stoi(argv[++i]);
        else if (!std::strcmp(argv[i], "--verbose") || !std::strcmp(argv[i], "-v"))
            cfg.verbose = true;
        else if (!std::strcmp(argv[i], "--no-color"))
            cfg.no_color = true;
        else if (!std::strcmp(argv[i], "--tests-dir") && i + 1 < argc)
            cfg.tests_dir = argv[++i];
        else if (argv[i][0] != '-')
            cfg.specific_files.emplace_back(argv[i]);
    }

    if (!cfg.no_color) {
        std::cout << CYAN << "🧬 X-Phage Test Runner v3.5.0" << NC << "\n";
        std::cout << DIM << "   Tests dir: " << cfg.tests_dir
                  << "  Jobs: " << cfg.jobs << NC << "\n\n";
    }

    auto tests = collect_tests(cfg);
    if (tests.empty()) {
        std::cout << YELLOW << "No tests found." << NC << "\n";
        return 0;
    }

    std::cout << DIM << "Running " << tests.size() << " test(s)...\n" << NC;

    // Run in parallel (simple thread pool)
    std::vector<TestResult> results(tests.size());
    std::atomic<size_t> next_test(0);

    auto worker = [&]() {
        while (true) {
            size_t idx = next_test.fetch_add(1);
            if (idx >= tests.size()) break;
            results[idx] = run_test(tests[idx].first,
                                    tests[idx].second, cfg);
            print_result(results[idx], cfg);
        }
    };

    int actual_jobs = std::min(cfg.jobs, (int)tests.size());
    std::vector<std::thread> threads;
    for (int i = 0; i < actual_jobs; i++)
        threads.emplace_back(worker);
    for (auto& t : threads) t.join();

    // Summary
    int passed = 0, failed = 0, skipped = 0, timedout = 0;
    double total_ms = 0.0;
    for (auto& r : results) {
        total_ms += r.elapsed;
        switch (r.status) {
            case TestStatus::Pass:    passed++;   break;
            case TestStatus::Fail:    failed++;   break;
            case TestStatus::Skip:    skipped++;  break;
            case TestStatus::Timeout: timedout++; break;
        }
    }

    std::cout << "\n";
    if (!cfg.no_color) std::cout << "─────────────────────────────────\n";

    std::cout << (failed == 0 ? GREEN : RED)
              << "Results: " << passed << " passed, "
              << failed << " failed";
    if (skipped)  std::cout << ", " << skipped  << " skipped";
    if (timedout) std::cout << ", " << timedout << " timed out";
    std::cout << NC << "\n";
    std::cout << DIM << "Total time: " << total_ms << " ms" << NC << "\n";

    return failed > 0 ? 1 : 0;
}
