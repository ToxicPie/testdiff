#include "main.hpp"
#include "cfg.hpp"

#include <iostream>


int main(int argc, const char **argv) {
    if (argc != 2) {
        exit_with_usage(argv[0]);
    }

    BinaryFile graph(argv[1]);
}

__attribute__((noreturn)) void exit_with_usage(const char *exe) {
    std::cerr << "Usage: " << exe << " <file>\n";
    exit(1);
}
