#include <iomanip>
#include <iostream>

#include "main.hpp"
#include "diff.hpp"


//! workaround to prevent r2 internal data structures from being detected as
//! memory leaks
#ifdef __cplusplus
extern "C" {
#endif
const char *__asan_default_options() {
    return "detect_leaks=0";
}
#ifdef __cplusplus
}
#endif

int main(int argc, const char **argv) {
    if (argc != 3) {
        exit_with_usage(argv[0]);
    }

    const std::string file1(argv[1]), file2(argv[2]);

    BinaryFile bin1(file1), bin2(file2);

    std::cout << std::fixed << std::setprecision(3);

    diff(bin1, bin2);
}

__attribute__((noreturn)) void exit_with_usage(const char *exe) {
    std::cerr << "Usage: " << exe << " <file1> <file2>\n";
    exit(1);
}
