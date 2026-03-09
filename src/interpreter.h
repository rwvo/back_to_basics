#pragma once

#include <iostream>
#include <string>
#include <vector>
#include "ast.h"
#include "environment.h"

namespace rocbas {

class Interpreter {
public:
    explicit Interpreter(std::ostream& out = std::cout,
                         std::istream& in = std::cin);
    void run(const std::string& source);
    void run(const Program& program);

private:
    std::ostream& out_;
    std::istream& in_;
    Environment env_;
    std::vector<int> gosub_stack_;  // return line indices for GOSUB

    // Statement execution
    void execute(const Statement& stmt, const Program& program, size_t& pc);
    void exec_let(const LetStmt& stmt);
    void exec_print(const PrintStmt& stmt);
    void exec_input(const InputStmt& stmt);
    void exec_goto(const GotoStmt& stmt, const Program& program, size_t& pc);
    void exec_gosub(const GosubStmt& stmt, const Program& program, size_t& pc);
    void exec_return(size_t& pc);
    void exec_if(const IfStmt& stmt, const Program& program, size_t& pc);
    void exec_for(const ForStmt& stmt, const Program& program, size_t& pc);
    void exec_next(const NextStmt& stmt, const Program& program, size_t& pc);
    void exec_dim(const DimStmt& stmt);

    // Expression evaluation
    Value eval(const Expression& expr);
    Value eval_binary(const BinaryExpr& expr);
    Value eval_unary(const UnaryExpr& expr);
    Value eval_function(const FunctionCall& call);

    // Helpers
    double to_number(const Value& v) const;
    std::string to_string_val(const Value& v) const;
    std::string format_number(double v) const;
    size_t find_line(const Program& program, int line_number) const;

    // FOR loop tracking
    struct ForContext {
        std::string var_name;
        double end_val;
        double step_val;
        size_t body_pc;  // index of first statement after FOR
    };
    std::vector<ForContext> for_stack_;
};

} // namespace rocbas
