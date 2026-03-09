#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include "interpreter.h"

static void print_usage() {
    std::cout << "Usage: rocBAS [file.bas]\n"
              << "  file.bas    Run a BASIC program\n"
              << "  (no args)   Start interactive REPL\n";
}

static void run_repl() {
    std::cout << "rocBAS v0.1 - BASIC interpreter with GPU extensions\n"
              << "Type BASIC lines with line numbers. Type RUN to execute.\n"
              << "Type LIST to show program. Type NEW to clear. Type QUIT to exit.\n\n";

    std::map<int, std::string> program_lines;

    while (true) {
        std::cout << "> ";
        std::string line;
        if (!std::getline(std::cin, line)) break;

        // Trim
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos) continue;
        line = line.substr(start);

        // REPL commands (case-insensitive)
        std::string upper = line;
        for (auto& c : upper) c = toupper(c);

        if (upper == "QUIT" || upper == "EXIT") break;

        if (upper == "NEW") {
            program_lines.clear();
            std::cout << "Program cleared.\n";
            continue;
        }

        if (upper == "LIST") {
            if (program_lines.empty()) {
                std::cout << "(empty program)\n";
            } else {
                for (const auto& [num, text] : program_lines) {
                    std::cout << num << " " << text << "\n";
                }
            }
            continue;
        }

        if (upper == "RUN") {
            std::ostringstream source;
            for (const auto& [num, text] : program_lines) {
                source << num << " " << text << "\n";
            }
            rocbas::Interpreter interpreter;
            try {
                interpreter.run(source.str());
            } catch (const std::exception& e) {
                std::cerr << "Error: " << e.what() << "\n";
            }
            continue;
        }

        // Try to parse as a numbered line
        if (std::isdigit(line[0])) {
            size_t pos = 0;
            int num = std::stoi(line, &pos);
            std::string rest = line.substr(pos);
            // Trim leading whitespace from rest
            size_t rst = rest.find_first_not_of(" \t");
            if (rst == std::string::npos) {
                // Just a line number: delete that line
                program_lines.erase(num);
            } else {
                program_lines[num] = rest.substr(rst);
            }
            continue;
        }

        std::cerr << "Error: enter a line number, or RUN/LIST/NEW/QUIT\n";
    }
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
    run_repl();
    return 0;
}
