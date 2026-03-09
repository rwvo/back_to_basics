#include "interpreter.h"
#include "gpu_codegen.h"
#include "gpu_runtime.h"
#include "lexer.h"
#include "parser.h"
#include <cmath>
#include <cstdlib>
#include <sstream>
#include <stdexcept>

namespace rocbas {

Interpreter::Interpreter(std::ostream& out, std::istream& in)
    : out_(out), in_(in) {}

Interpreter::~Interpreter() = default;

GpuRuntime& Interpreter::gpu() {
    if (!gpu_) {
        gpu_ = std::make_unique<GpuRuntime>();
        if (!gpu_->is_available()) {
            throw std::runtime_error("No GPU device available");
        }
    }
    return *gpu_;
}

void Interpreter::run(const std::string& source) {
    Lexer lexer(source);
    auto tokens = lexer.tokenize();
    Parser parser(tokens);
    auto program = parser.parse();
    run(program);
}

void Interpreter::run(const Program& program) {
    if (program.lines.empty()) return;

    size_t pc = 0;
    while (pc < program.lines.size()) {
        execute(*program.lines[pc], program, pc);
    }
}

// --- Statement execution ---

void Interpreter::execute(const Statement& stmt, const Program& program, size_t& pc) {
    std::visit([&](const auto& s) {
        using T = std::decay_t<decltype(s)>;

        if constexpr (std::is_same_v<T, LetStmt>) {
            exec_let(s); pc++;
        } else if constexpr (std::is_same_v<T, PrintStmt>) {
            exec_print(s); pc++;
        } else if constexpr (std::is_same_v<T, InputStmt>) {
            exec_input(s); pc++;
        } else if constexpr (std::is_same_v<T, GotoStmt>) {
            exec_goto(s, program, pc);
        } else if constexpr (std::is_same_v<T, GosubStmt>) {
            exec_gosub(s, program, pc);
        } else if constexpr (std::is_same_v<T, ReturnStmt>) {
            exec_return(pc);
        } else if constexpr (std::is_same_v<T, IfStmt>) {
            exec_if(s, program, pc);
        } else if constexpr (std::is_same_v<T, ForStmt>) {
            exec_for(s, program, pc);
        } else if constexpr (std::is_same_v<T, NextStmt>) {
            exec_next(s, program, pc);
        } else if constexpr (std::is_same_v<T, DimStmt>) {
            exec_dim(s); pc++;
        } else if constexpr (std::is_same_v<T, EndStmt>) {
            pc = program.lines.size();  // halt
        } else if constexpr (std::is_same_v<T, RemStmt>) {
            pc++;  // skip comments
        } else if constexpr (std::is_same_v<T, DataStmt>) {
            pc++;  // DATA is processed separately
        } else if constexpr (std::is_same_v<T, ReadStmt>) {
            pc++;  // TODO
        } else if constexpr (std::is_same_v<T, RestoreStmt>) {
            pc++;  // TODO
        } else if constexpr (std::is_same_v<T, GpuDimStmt>) {
            exec_gpu_dim(s); pc++;
        } else if constexpr (std::is_same_v<T, GpuCopyStmt>) {
            exec_gpu_copy(s); pc++;
        } else if constexpr (std::is_same_v<T, GpuFreeStmt>) {
            exec_gpu_free(s); pc++;
        } else if constexpr (std::is_same_v<T, GpuKernelStmt>) {
            exec_gpu_kernel(s); pc++;
        } else if constexpr (std::is_same_v<T, GpuGosubStmt>) {
            exec_gpu_gosub(s); pc++;
        } else if constexpr (std::is_same_v<T, OptionBaseStmt>) {
            exec_option(s); pc++;
        }
    }, stmt.stmt);
}

void Interpreter::exec_let(const LetStmt& stmt) {
    Value val = eval(*stmt.value);
    if (stmt.indices.empty()) {
        env_.set(stmt.var_name, val);
    } else {
        std::vector<int> indices;
        for (const auto& idx : stmt.indices) {
            indices.push_back(static_cast<int>(to_number(eval(*idx))));
        }
        env_.set_array(stmt.var_name, indices, val);
    }
}

void Interpreter::exec_print(const PrintStmt& stmt) {
    if (stmt.items.empty()) {
        out_ << "\n";
        return;
    }

    for (size_t i = 0; i < stmt.items.size(); i++) {
        Value val = eval(*stmt.items[i].expr);
        if (std::holds_alternative<double>(val)) {
            out_ << format_number(std::get<double>(val));
        } else {
            out_ << std::get<std::string>(val);
        }
    }

    if (stmt.trailing_newline) {
        out_ << "\n";
    }
}

void Interpreter::exec_input(const InputStmt& stmt) {
    if (!stmt.prompt.empty()) {
        out_ << stmt.prompt << "? ";
    } else {
        out_ << "? ";
    }

    std::string line;
    std::getline(in_, line);

    // If string variable, store as string; otherwise parse as number
    if (stmt.var_name.back() == '$') {
        env_.set(stmt.var_name, line);
    } else {
        try {
            double val = std::stod(line);
            env_.set(stmt.var_name, val);
        } catch (...) {
            env_.set(stmt.var_name, 0.0);
        }
    }
}

void Interpreter::exec_goto(const GotoStmt& stmt, const Program& program, size_t& pc) {
    pc = find_line(program, stmt.target_line);
}

void Interpreter::exec_gosub(const GosubStmt& stmt, const Program& program, size_t& pc) {
    gosub_stack_.push_back(pc + 1);
    pc = find_line(program, stmt.target_line);
}

void Interpreter::exec_return(size_t& pc) {
    if (gosub_stack_.empty()) {
        throw std::runtime_error("RETURN without GOSUB");
    }
    pc = gosub_stack_.back();
    gosub_stack_.pop_back();
}

void Interpreter::exec_if(const IfStmt& stmt, const Program& program, size_t& pc) {
    Value cond = eval(*stmt.condition);
    bool is_true = to_number(cond) != 0.0;

    if (is_true) {
        if (stmt.has_goto) {
            pc = find_line(program, stmt.goto_line);
        } else {
            // Execute the THEN statement in-place
            execute(*stmt.then_stmt, program, pc);
            return;  // pc already updated by the nested execute
        }
    } else {
        pc++;
    }
}

void Interpreter::exec_for(const ForStmt& stmt, const Program& program, size_t& pc) {
    double start_val = to_number(eval(*stmt.start));
    double end_val = to_number(eval(*stmt.end));
    double step_val = stmt.step ? to_number(eval(*stmt.step)) : 1.0;

    env_.set(stmt.var_name, start_val);

    // Check if loop should execute at all
    if ((step_val > 0 && start_val > end_val) ||
        (step_val < 0 && start_val < end_val)) {
        // Skip to matching NEXT
        size_t depth = 1;
        pc++;
        while (pc < program.lines.size() && depth > 0) {
            if (std::holds_alternative<ForStmt>(program.lines[pc]->stmt)) depth++;
            if (std::holds_alternative<NextStmt>(program.lines[pc]->stmt)) depth--;
            if (depth > 0) pc++;
        }
        pc++;  // skip past NEXT
        return;
    }

    for_stack_.push_back(ForContext{stmt.var_name, end_val, step_val, pc + 1});
    pc++;
}

void Interpreter::exec_next(const NextStmt& stmt, const Program& program, size_t& pc) {
    if (for_stack_.empty()) {
        throw std::runtime_error("NEXT without FOR");
    }

    auto& ctx = for_stack_.back();
    if (ctx.var_name != stmt.var_name) {
        throw std::runtime_error("NEXT " + stmt.var_name + " doesn't match FOR " +
                                 ctx.var_name);
    }

    double current = to_number(env_.get(ctx.var_name));
    current += ctx.step_val;
    env_.set(ctx.var_name, current);

    bool done = (ctx.step_val > 0 && current > ctx.end_val) ||
                (ctx.step_val < 0 && current < ctx.end_val);

    if (done) {
        for_stack_.pop_back();
        pc++;
    } else {
        pc = ctx.body_pc;
    }
}

void Interpreter::exec_dim(const DimStmt& stmt) {
    for (const auto& decl : stmt.arrays) {
        std::vector<int> dims;
        for (const auto& d : decl.dimensions) {
            dims.push_back(static_cast<int>(to_number(eval(*d))));
        }
        env_.dim_array(decl.name, dims);
    }
}

// --- GPU statement execution ---

void Interpreter::exec_gpu_dim(const GpuDimStmt& stmt) {
    for (const auto& decl : stmt.arrays) {
        // Evaluate dimension (should be a single 1D size)
        if (decl.dimensions.size() != 1) {
            throw std::runtime_error("GPU DIM only supports 1D arrays");
        }
        size_t size = static_cast<size_t>(to_number(eval(*decl.dimensions[0])));
        gpu().gpu_dim(decl.name, size);
    }
}

void Interpreter::exec_gpu_copy(const GpuCopyStmt& stmt) {
    // Determine direction: if src is a host array, copy H2D; if src is a GPU array, copy D2H
    bool src_is_host = env_.has_array(stmt.src);

    if (src_is_host) {
        // Host → Device: GPU COPY A TO GA
        auto data = env_.get_array_data(stmt.src);
        gpu().gpu_copy_host_to_device(stmt.src, stmt.dst, data.data(), data.size());
    } else {
        // Device → Host: GPU COPY GA TO A
        size_t size = gpu().get_device_array_size(stmt.src);
        std::vector<double> data(size);
        gpu().gpu_copy_device_to_host(stmt.src, stmt.dst, data.data(), size);
        env_.set_array_data(stmt.dst, data);
    }
}

void Interpreter::exec_gpu_free(const GpuFreeStmt& stmt) {
    gpu().gpu_free(stmt.name);
}

void Interpreter::exec_gpu_kernel(const GpuKernelStmt& stmt) {
    // Register the kernel definition (don't compile yet — compile on first GOSUB)
    kernel_defs_[stmt.name] = &stmt;
}

void Interpreter::exec_option(const OptionBaseStmt& stmt) {
    if (stmt.is_gpu) {
        gpu_base_ = stmt.base;
    } else {
        env_.set_array_base(stmt.base);
    }
}

void Interpreter::exec_gpu_gosub(const GpuGosubStmt& stmt) {
    // Find the kernel definition
    auto it = kernel_defs_.find(stmt.kernel_name);
    if (it == kernel_defs_.end()) {
        throw std::runtime_error("Undefined GPU kernel: " + stmt.kernel_name);
    }
    const GpuKernelStmt& kernel = *it->second;

    // Generate and compile the kernel (compile on first call)
    std::string source = generate_kernel_source(kernel, gpu_base_);
    gpu().compile_kernel(kernel.name, source);

    // Build argument lists: identify which args are GPU arrays vs scalars
    std::vector<std::string> arg_names;
    std::vector<double> scalar_args;

    for (const auto& arg_expr : stmt.args) {
        // Evaluate the argument expression
        Value val = eval(*arg_expr);

        // Check if it's a variable name that refers to a GPU array
        if (std::holds_alternative<Variable>(arg_expr->expr)) {
            const auto& var_name = std::get<Variable>(arg_expr->expr).name;
            try {
                gpu().get_device_ptr(var_name);
                // It's a GPU array
                arg_names.push_back(var_name);
                continue;
            } catch (...) {
                // Not a GPU array, fall through to scalar
            }
        }

        // It's a scalar value
        arg_names.push_back("__scalar_" + std::to_string(scalar_args.size()));
        scalar_args.push_back(to_number(val));
    }

    // Evaluate grid and block dimensions
    std::vector<size_t> grid_dims, block_dims;
    for (const auto& g : stmt.grid_dims) {
        grid_dims.push_back(static_cast<size_t>(to_number(eval(*g))));
    }
    for (const auto& b : stmt.block_dims) {
        block_dims.push_back(static_cast<size_t>(to_number(eval(*b))));
    }

    // Launch the kernel
    gpu().launch_kernel(kernel.name, arg_names, scalar_args, grid_dims, block_dims);
}

// --- Expression evaluation ---

Value Interpreter::eval(const Expression& expr) {
    return std::visit([&](const auto& e) -> Value {
        using T = std::decay_t<decltype(e)>;

        if constexpr (std::is_same_v<T, NumberLiteral>) {
            return e.value;
        } else if constexpr (std::is_same_v<T, StringLiteral>) {
            return e.value;
        } else if constexpr (std::is_same_v<T, Variable>) {
            return env_.get(e.name);
        } else if constexpr (std::is_same_v<T, ArrayAccess>) {
            std::vector<int> indices;
            for (const auto& idx : e.indices) {
                indices.push_back(static_cast<int>(to_number(eval(*idx))));
            }
            return env_.get_array(e.name, indices);
        } else if constexpr (std::is_same_v<T, BinaryExpr>) {
            return eval_binary(e);
        } else if constexpr (std::is_same_v<T, UnaryExpr>) {
            return eval_unary(e);
        } else if constexpr (std::is_same_v<T, FunctionCall>) {
            return eval_function(e);
        } else if constexpr (std::is_same_v<T, GpuIntrinsic>) {
            throw std::runtime_error("GPU intrinsics can only be used inside GPU KERNEL");
        }
    }, expr.expr);
}

Value Interpreter::eval_binary(const BinaryExpr& expr) {
    Value left = eval(*expr.left);
    Value right = eval(*expr.right);

    // String concatenation: string + string
    if (expr.op == BinaryOp::ADD &&
        std::holds_alternative<std::string>(left) &&
        std::holds_alternative<std::string>(right)) {
        return std::get<std::string>(left) + std::get<std::string>(right);
    }

    // String comparison
    if (std::holds_alternative<std::string>(left) &&
        std::holds_alternative<std::string>(right)) {
        const auto& l = std::get<std::string>(left);
        const auto& r = std::get<std::string>(right);
        switch (expr.op) {
            case BinaryOp::EQ: return (l == r) ? 1.0 : 0.0;
            case BinaryOp::NE: return (l != r) ? 1.0 : 0.0;
            case BinaryOp::LT: return (l < r) ? 1.0 : 0.0;
            case BinaryOp::GT: return (l > r) ? 1.0 : 0.0;
            case BinaryOp::LE: return (l <= r) ? 1.0 : 0.0;
            case BinaryOp::GE: return (l >= r) ? 1.0 : 0.0;
            default: throw std::runtime_error("Invalid operation on strings");
        }
    }

    // Numeric operations
    double l = to_number(left);
    double r = to_number(right);

    switch (expr.op) {
        case BinaryOp::ADD: return l + r;
        case BinaryOp::SUB: return l - r;
        case BinaryOp::MUL: return l * r;
        case BinaryOp::DIV:
            if (r == 0.0) throw std::runtime_error("Division by zero");
            return l / r;
        case BinaryOp::POW: return std::pow(l, r);
        case BinaryOp::EQ:  return (l == r) ? 1.0 : 0.0;
        case BinaryOp::NE:  return (l != r) ? 1.0 : 0.0;
        case BinaryOp::LT:  return (l < r) ? 1.0 : 0.0;
        case BinaryOp::GT:  return (l > r) ? 1.0 : 0.0;
        case BinaryOp::LE:  return (l <= r) ? 1.0 : 0.0;
        case BinaryOp::GE:  return (l >= r) ? 1.0 : 0.0;
        case BinaryOp::AND: return (l != 0.0 && r != 0.0) ? 1.0 : 0.0;
        case BinaryOp::OR:  return (l != 0.0 || r != 0.0) ? 1.0 : 0.0;
    }
    throw std::runtime_error("Unknown binary operator");
}

Value Interpreter::eval_unary(const UnaryExpr& expr) {
    Value operand = eval(*expr.operand);
    switch (expr.op) {
        case UnaryOp::NEGATE: return -to_number(operand);
        case UnaryOp::NOT:    return (to_number(operand) == 0.0) ? 1.0 : 0.0;
    }
    throw std::runtime_error("Unknown unary operator");
}

Value Interpreter::eval_function(const FunctionCall& call) {
    const auto& name = call.name;
    const auto& args = call.args;

    // Math functions (one argument)
    if (name == "ABS") return std::abs(to_number(eval(*args.at(0))));
    if (name == "INT") return std::floor(to_number(eval(*args.at(0))));
    if (name == "SQR") return std::sqrt(to_number(eval(*args.at(0))));
    if (name == "SIN") return std::sin(to_number(eval(*args.at(0))));
    if (name == "COS") return std::cos(to_number(eval(*args.at(0))));
    if (name == "TAN") return std::tan(to_number(eval(*args.at(0))));

    // RND: returns random number 0..1 (argument is ignored per tradition)
    if (name == "RND") {
        return static_cast<double>(std::rand()) / RAND_MAX;
    }

    // String functions
    if (name == "LEN") {
        return static_cast<double>(std::get<std::string>(eval(*args.at(0))).length());
    }
    if (name == "LEFT$") {
        std::string s = std::get<std::string>(eval(*args.at(0)));
        int n = static_cast<int>(to_number(eval(*args.at(1))));
        return s.substr(0, n);
    }
    if (name == "RIGHT$") {
        std::string s = std::get<std::string>(eval(*args.at(0)));
        int n = static_cast<int>(to_number(eval(*args.at(1))));
        if (n >= static_cast<int>(s.length())) return s;
        return s.substr(s.length() - n);
    }
    if (name == "MID$") {
        std::string s = std::get<std::string>(eval(*args.at(0)));
        int start = static_cast<int>(to_number(eval(*args.at(1)))) - 1;  // 1-indexed
        int len = (args.size() > 2) ? static_cast<int>(to_number(eval(*args.at(2))))
                                     : static_cast<int>(s.length()) - start;
        if (start < 0) start = 0;
        return s.substr(start, len);
    }
    if (name == "CHR$") {
        int code = static_cast<int>(to_number(eval(*args.at(0))));
        return std::string(1, static_cast<char>(code));
    }
    if (name == "ASC") {
        std::string s = std::get<std::string>(eval(*args.at(0)));
        if (s.empty()) throw std::runtime_error("ASC of empty string");
        return static_cast<double>(static_cast<unsigned char>(s[0]));
    }
    if (name == "VAL") {
        std::string s = std::get<std::string>(eval(*args.at(0)));
        try { return std::stod(s); } catch (...) { return 0.0; }
    }
    if (name == "STR$") {
        return format_number(to_number(eval(*args.at(0))));
    }
    if (name == "TAB") {
        int n = static_cast<int>(to_number(eval(*args.at(0))));
        return std::string(n, ' ');
    }

    throw std::runtime_error("Unknown function: " + name);
}

// --- Helpers ---

double Interpreter::to_number(const Value& v) const {
    if (std::holds_alternative<double>(v)) {
        return std::get<double>(v);
    }
    throw std::runtime_error("Type mismatch: expected number, got string");
}

std::string Interpreter::to_string_val(const Value& v) const {
    if (std::holds_alternative<std::string>(v)) {
        return std::get<std::string>(v);
    }
    return format_number(std::get<double>(v));
}

std::string Interpreter::format_number(double v) const {
    // Display integers without decimal point
    if (v == std::floor(v) && std::abs(v) < 1e15) {
        std::ostringstream ss;
        ss << static_cast<long long>(v);
        return ss.str();
    }
    std::ostringstream ss;
    ss << v;
    return ss.str();
}

size_t Interpreter::find_line(const Program& program, int line_number) const {
    for (size_t i = 0; i < program.lines.size(); i++) {
        if (program.lines[i]->line_number == line_number) {
            return i;
        }
    }
    throw std::runtime_error("Undefined line number: " + std::to_string(line_number));
}

} // namespace rocbas
