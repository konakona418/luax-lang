#pragma once

#include <cassert>
#include <chrono>
#include <iostream>
#include <utility>

#define DEBUG

namespace test_helper {
    class StopWatch {
        std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
        std::chrono::time_point<std::chrono::high_resolution_clock> end_time;

    public:
        StopWatch() = default;

        StopWatch(const StopWatch &) = delete;

        StopWatch(StopWatch &&) = delete;

        void start() {
            start_time = std::chrono::high_resolution_clock::now();
        }

        void end() {
            end_time = std::chrono::high_resolution_clock::now();
        }

        [[nodiscard]] double elapsed_ms() const {
            return static_cast<double>(
                           std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count()) /
                   1000.0;
        }
    };

#ifdef DEBUG
    template<typename... Args>
    void dbg_print(Args &&...args) {
        std::cout << "[DEBUG] ";
        (std::cout << ... << std::forward<Args>(args)) << std::endl;
    }
#else
    template<typename... Args>
    void dbg_print(Args &&...args) {}
#endif

#define begin_test(test_name)                              \
    std::cout << "begin test: " << test_name << std::endl; \
    do {                                                   \
        int _test_count = 0;                               \
        int _test_passed = 0;                              \
        do

#define test(fn)                                              \
    _test_count++;                                            \
    try {                                                     \
        std::cout << "testing: " << #fn << std::endl;         \
        fn();                                                 \
        std::cout << "test passed" << std::endl               \
                  << std::endl;                               \
        _test_passed++;                                       \
    } catch (const std::exception &e) {                       \
        std::cout << "test failed: " << e.what() << std::endl \
                  << std::endl;                               \
    }

#define end_test()                                                          \
    while (false);                                                          \
    std::cout << "test count: " << _test_count << std::endl;                \
    std::cout << "test passed: " << _test_passed << std::endl;              \
    std::cout << "test failed: " << _test_count - _test_passed << std::endl \
              << std::endl;                                                 \
    assert(_test_passed == _test_count);                                    \
    }                                                                       \
    while (false);
}// namespace test_helper