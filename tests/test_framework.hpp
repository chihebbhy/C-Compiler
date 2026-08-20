#pragma once
#include <iostream>
#include <string>
#include <functional>
#include <vector>

// ---- Minimal test framework (no external deps) ----

struct TestStats {
    int passed = 0;
    int failed = 0;
};

inline TestStats& stats() {
    static TestStats s;
    return s;
}

inline std::string& currentTestName() {
    static std::string name;
    return name;
}

// Records a pass/fail without stopping the test on failure.
inline void check_impl(bool cond, const std::string& expr,
                        const std::string& file, int line) {
    if (cond) {
        stats().passed++;
    } else {
        stats().failed++;
        std::cout << "  [FAIL] " << currentTestName() << " -> " << expr
                  << "  (" << file << ":" << line << ")" << std::endl;
    }
}

// Compares two values for equality; on failure prints expected/actual.
template <typename A, typename B>
inline void check_eq_impl(const A& actual, const B& expected,
                           const std::string& exprA, const std::string& exprB,
                           const std::string& file, int line) {
    if (actual == expected) {
        stats().passed++;
    } else {
        stats().failed++;
        std::cout << "  [FAIL] " << currentTestName() << " -> " << exprA
                  << " == " << exprB << "  (" << file << ":" << line << ")"
                  << "\n         expected: " << expected
                  << "\n         actual:   " << actual << std::endl;
    }
}

#define CHECK(cond) check_impl((cond), #cond, __FILE__, __LINE__)
#define CHECK_EQ(actual, expected) \
    check_eq_impl((actual), (expected), #actual, #expected, __FILE__, __LINE__)

#define TEST(name) void name(); \
    struct name##_registrar { \
        name##_registrar() { registerTest(#name, name); } \
    } name##_registrar_instance; \
    void name()

using TestFn = std::function<void()>;

inline std::vector<std::pair<std::string, TestFn>>& testRegistry() {
    static std::vector<std::pair<std::string, TestFn>> registry;
    return registry;
}

inline void registerTest(const std::string& name, TestFn fn) {
    testRegistry().push_back({name, fn});
}

inline int runAllTests() {
    std::cout << "Running " << testRegistry().size() << " test cases...\n" << std::endl;
    for (auto& [name, fn] : testRegistry()) {
        currentTestName() = name;
        int before = stats().failed;
        fn();
        if (stats().failed == before) {
            std::cout << "  [PASS] " << name << std::endl;
        }
    }
    std::cout << "\n------------------------------" << std::endl;
    std::cout << "Checks passed: " << stats().passed
               << " | Checks failed: " << stats().failed << std::endl;
    std::cout << "------------------------------" << std::endl;
    return stats().failed == 0 ? 0 : 1;
}
