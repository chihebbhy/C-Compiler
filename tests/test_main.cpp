#include "test_framework.hpp"

int main() {
    int result = runAllTests();
    if (result == 0) {
        std::cout << "\nALL TESTS PASSED" << std::endl;
    } else {
        std::cout << "\nTESTS FAILED" << std::endl;
    }
    return result;
}
