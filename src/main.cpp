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

// Extract a quoted filename from a command like: SAVE "foo.bas"
// Returns the filename, or empty string if quotes are malformed.
static std::string extract_quoted_filename(const std::string& line) {
    auto first = line.find('"');
    if (first == std::string::npos) return "";
    auto second = line.find('"', first + 1);
    if (second == std::string::npos) return "";
    return line.substr(first + 1, second - first - 1);
}

static void run_repl() {
    std::cout << "rocBAS v0.1 - BASIC interpreter with GPU extensions\n"
              << "Type BASIC lines with line numbers. Type RUN to execute.\n"
              << "Type LIST to show. LOAD/SAVE \"file\" for files. QUIT to exit.\n\n";

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

        if (upper.substr(0, 5) == "SAVE ") {
            std::string filename = extract_quoted_filename(line);
            if (filename.empty()) {
                std::cerr << "Usage: SAVE \"filename\"\n";
                continue;
            }
            if (program_lines.empty()) {
                std::cerr << "No program to save.\n";
                continue;
            }
            std::ofstream out(filename);
            if (!out.is_open()) {
                std::cerr << "Error: cannot open '" << filename << "' for writing\n";
                continue;
            }
            for (const auto& [num, text] : program_lines) {
                out << num << " " << text << "\n";
            }
            std::cout << "Program saved to " << filename << "\n";
            continue;
        }

        if (upper.substr(0, 5) == "LOAD ") {
            std::string filename = extract_quoted_filename(line);
            if (filename.empty()) {
                std::cerr << "Usage: LOAD \"filename\"\n";
                continue;
            }
            std::ifstream in(filename);
            if (!in.is_open()) {
                std::cerr << "Error: cannot open '" << filename << "'\n";
                continue;
            }
            program_lines.clear();
            std::string fline;
            while (std::getline(in, fline)) {
                // Skip empty lines
                size_t fs = fline.find_first_not_of(" \t");
                if (fs == std::string::npos) continue;
                fline = fline.substr(fs);
                // Parse numbered lines, skip others
                if (!std::isdigit(fline[0])) continue;
                size_t pos = 0;
                int num = std::stoi(fline, &pos);
                std::string rest = fline.substr(pos);
                size_t rst = rest.find_first_not_of(" \t");
                if (rst != std::string::npos) {
                    program_lines[num] = rest.substr(rst);
                }
            }
            std::cout << "Program loaded from " << filename << "\n";
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

        std::cerr << "Error: enter a line number, or RUN/LIST/NEW/LOAD/SAVE/QUIT\n";
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
