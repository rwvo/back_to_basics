#pragma once

#include <string>

namespace rocbas {

class Interpreter {
public:
    Interpreter() = default;
    void run(const std::string& source);
};

} // namespace rocbas
