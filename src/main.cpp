#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include "interpreter.h"

static void print_usage() {
    std::cout << "Usage: rocBAS [file.bas]\n"
              << "  file.bas    Run a BASIC program\n"
              << "  (no args)   Start interactive REPL\n";
}

int main(int argc, char* argv[]) {
    if (argc > 2) {
        print_usage();
        return 1;
    }

    if (argc == 2) {
        std::string filename = argv[1];
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Error: cannot open file '" << filename << "'\n";
            return 1;
        }
        std::ostringstream ss;
        ss << file.rdbuf();

        rocbas::Interpreter interpreter;
        try {
            interpreter.run(ss.str());
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << "\n";
            return 1;
        }
        return 0;
    }

    // REPL mode
    std::cerr << "rocBAS REPL not yet implemented\n";
    return 1;
}
