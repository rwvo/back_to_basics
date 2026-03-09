#include "gpu_codegen.h"
#include <functional>
#include <sstream>
#include <stdexcept>

namespace rocbas {

namespace {

// Module-level gpu_base for the current codegen invocation
static double g_gpu_base = 0;

// Format a base value without trailing zeros (avoids "0.500000")
static std::string format_base(double b) {
    if (b == static_cast<int>(b)) return std::to_string(static_cast<int>(b));
    // For 0.5, just return "0.5"
    std::ostringstream ss;
    ss << b;
    return ss.str();
}

// Emit HIP C++ for an expression.
// In kernel context, variables are kernel parameters (double or double*).
std::string emit_expr(const Expression& expr) {
    return std::visit([&](const auto& e) -> std::string {
        using T = std::decay_t<decltype(e)>;

        if constexpr (std::is_same_v<T, NumberLiteral>) {
            std::ostringstream ss;
            ss << e.value;
            return ss.str();
        } else if constexpr (std::is_same_v<T, Variable>) {
            return e.name;
        } else if constexpr (std::is_same_v<T, ArrayAccess>) {
            if (e.indices.size() != 1) {
                throw std::runtime_error(
                    "GPU kernels only support 1D array access, got " +
                    std::to_string(e.indices.size()) + "D for " + e.name);
            }
            std::string inner = emit_expr(*e.indices[0]);
            std::string idx;
            if (g_gpu_base != 0) {
                idx = "(int)(" + inner + " - " + format_base(g_gpu_base) + ")";
            } else {
                idx = "(int)(" + inner + ")";
            }
            return e.name + "[" + idx + "]";
        } else if constexpr (std::is_same_v<T, BinaryExpr>) {
            std::string left = emit_expr(*e.left);
            std::string right = emit_expr(*e.right);
            switch (e.op) {
                case BinaryOp::ADD: return "(" + left + " + " + right + ")";
                case BinaryOp::SUB: return "(" + left + " - " + right + ")";
                case BinaryOp::MUL: return "(" + left + " * " + right + ")";
                case BinaryOp::DIV: return "(" + left + " / " + right + ")";
                case BinaryOp::POW: return "pow(" + left + ", " + right + ")";
                case BinaryOp::EQ:  return "((" + left + ") == (" + right + ") ? 1.0 : 0.0)";
                case BinaryOp::NE:  return "((" + left + ") != (" + right + ") ? 1.0 : 0.0)";
                case BinaryOp::LT:  return "((" + left + ") < (" + right + ") ? 1.0 : 0.0)";
                case BinaryOp::GT:  return "((" + left + ") > (" + right + ") ? 1.0 : 0.0)";
                case BinaryOp::LE:  return "((" + left + ") <= (" + right + ") ? 1.0 : 0.0)";
                case BinaryOp::GE:  return "((" + left + ") >= (" + right + ") ? 1.0 : 0.0)";
                case BinaryOp::AND: return "((" + left + " != 0.0 && " + right + " != 0.0) ? 1.0 : 0.0)";
                case BinaryOp::OR:  return "((" + left + " != 0.0 || " + right + " != 0.0) ? 1.0 : 0.0)";
            }
            throw std::runtime_error("Unknown binary operator in kernel codegen");
        } else if constexpr (std::is_same_v<T, UnaryExpr>) {
            std::string operand = emit_expr(*e.operand);
            switch (e.op) {
                case UnaryOp::NEGATE: return "(-" + operand + ")";
                case UnaryOp::NOT:   return "((" + operand + " == 0.0) ? 1.0 : 0.0)";
            }
            throw std::runtime_error("Unknown unary operator in kernel codegen");
        } else if constexpr (std::is_same_v<T, GpuIntrinsic>) {
            // Dimension must be a number literal (1, 2, or 3)
            if (!std::holds_alternative<NumberLiteral>(e.dimension->expr)) {
                throw std::runtime_error("GPU intrinsic dimension must be a constant");
            }
            int dim = static_cast<int>(std::get<NumberLiteral>(e.dimension->expr).value);
            std::string suffix;
            switch (dim) {
                case 1: suffix = ".x"; break;
                case 2: suffix = ".y"; break;
                case 3: suffix = ".z"; break;
                default:
                    throw std::runtime_error("GPU intrinsic dimension must be 1, 2, or 3");
            }
            std::string base;
            switch (e.kind) {
                case GpuIntrinsicKind::THREAD_IDX: base = "threadIdx" + suffix; break;
                case GpuIntrinsicKind::BLOCK_IDX:  base = "blockIdx" + suffix; break;
                case GpuIntrinsicKind::BLOCK_DIM:  return "blockDim" + suffix;
                case GpuIntrinsicKind::GRID_DIM:   return "gridDim" + suffix;
            }
            // THREAD_IDX and BLOCK_IDX are offset by gpu_base (indices, not sizes)
            if (g_gpu_base != 0) {
                return "(" + base + " + " + format_base(g_gpu_base) + ")";
            }
            return base;
            throw std::runtime_error("Unknown GPU intrinsic kind");
        } else if constexpr (std::is_same_v<T, FunctionCall>) {
            // Support basic math functions in kernels
            if (e.name == "ABS" && e.args.size() == 1)
                return "fabs(" + emit_expr(*e.args[0]) + ")";
            if (e.name == "SQR" && e.args.size() == 1)
                return "sqrt(" + emit_expr(*e.args[0]) + ")";
            if (e.name == "SIN" && e.args.size() == 1)
                return "sin(" + emit_expr(*e.args[0]) + ")";
            if (e.name == "COS" && e.args.size() == 1)
                return "cos(" + emit_expr(*e.args[0]) + ")";
            if (e.name == "TAN" && e.args.size() == 1)
                return "tan(" + emit_expr(*e.args[0]) + ")";
            if (e.name == "INT" && e.args.size() == 1)
                return "floor(" + emit_expr(*e.args[0]) + ")";
            throw std::runtime_error("Unsupported function in GPU kernel: " + e.name);
        } else if constexpr (std::is_same_v<T, StringLiteral>) {
            // String literals are allowed for PRINT in GPU kernels
            // Escape for C string literal
            std::string escaped;
            for (char c : e.value) {
                switch (c) {
                    case '\\': escaped += "\\\\"; break;
                    case '"':  escaped += "\\\""; break;
                    case '\n': escaped += "\\n"; break;
                    case '\t': escaped += "\\t"; break;
                    default:   escaped += c; break;
                }
            }
            return "\"" + escaped + "\"";
        } else {
            throw std::runtime_error("Unsupported expression type in GPU kernel codegen");
        }
    }, expr.expr);
}

// Emit HIP C++ for a statement inside the kernel body.
void emit_stmt(const Statement& stmt, std::ostringstream& out, const std::string& indent) {
    std::visit([&](const auto& s) {
        using T = std::decay_t<decltype(s)>;

        if constexpr (std::is_same_v<T, LetStmt>) {
            if (s.indices.empty()) {
                // Simple variable assignment: LET X = expr → double X = expr;
                // But the variable might already be declared, so just assign
                out << indent << s.var_name << " = " << emit_expr(*s.value) << ";\n";
            } else {
                // Array element assignment: LET A(I) = expr
                if (s.indices.size() != 1) {
                    throw std::runtime_error(
                        "GPU kernels only support 1D array assignment");
                }
                std::string inner = emit_expr(*s.indices[0]);
                std::string idx;
                if (g_gpu_base != 0) {
                    idx = "(int)(" + inner + " - " + format_base(g_gpu_base) + ")";
                } else {
                    idx = "(int)(" + inner + ")";
                }
                out << indent << s.var_name << "[" << idx << "] = "
                    << emit_expr(*s.value) << ";\n";
            }
        } else if constexpr (std::is_same_v<T, PrintStmt>) {
            // PRINT in GPU kernel → single printf() call for atomic output
            // Build format string and collect argument expressions
            std::string fmt;
            std::vector<std::string> args;
            for (const auto& item : s.items) {
                if (std::holds_alternative<StringLiteral>(item.expr->expr)) {
                    // Inline string literal into format string (escape % for printf)
                    const auto& str = std::get<StringLiteral>(item.expr->expr).value;
                    for (char c : str) {
                        if (c == '%') fmt += "%%";
                        else if (c == '\\') fmt += "\\\\";
                        else if (c == '"') fmt += "\\\"";
                        else if (c == '\n') fmt += "\\n";
                        else if (c == '\t') fmt += "\\t";
                        else fmt += c;
                    }
                } else {
                    // Numeric expression → %g placeholder
                    // Cast to double so printf %g always matches the argument type
                    // (GPU intrinsics like threadIdx.x return unsigned int, not double)
                    fmt += "%g";
                    args.push_back("(double)(" + emit_expr(*item.expr) + ")");
                }
            }
            if (s.trailing_newline) {
                fmt += "\\n";
            }
            out << indent << "printf(\"" << fmt << "\"";
            for (const auto& arg : args) {
                out << ", " << arg;
            }
            out << ");\n";
        } else if constexpr (std::is_same_v<T, IfStmt>) {
            // IF cond THEN stmt → if (cond != 0.0) { stmt }
            out << indent << "if (" << emit_expr(*s.condition) << " != 0.0) {\n";
            if (!s.has_goto && s.then_stmt) {
                emit_stmt(*s.then_stmt, out, indent + "    ");
            } else {
                throw std::runtime_error("GOTO not supported inside GPU kernels");
            }
            out << indent << "}\n";
        } else {
            throw std::runtime_error("Unsupported statement type in GPU kernel body");
        }
    }, stmt.stmt);
}

} // anonymous namespace

std::string generate_kernel_source(const GpuKernelStmt& kernel, double gpu_base) {
    g_gpu_base = gpu_base;
    std::ostringstream out;

    // Generate extern "C" __global__ function
    out << "extern \"C\" __global__ void " << kernel.name << "(";

    // All parameters: we don't know at codegen time which are arrays vs scalars.
    // Convention: all params are double* or double. The interpreter will determine
    // which is which based on whether the arg matches a GPU array name.
    // For codegen, we need to know the types. We'll infer from usage in the body:
    // params used with array access → double*, params used as scalars → double.
    //
    // Simpler approach: params that appear in ArrayAccess nodes in the body are arrays.
    // All others are scalars.

    // Scan body to find which params are used as arrays
    std::vector<bool> is_array(kernel.params.size(), false);

    // Helper to scan expressions for array accesses
    std::function<void(const Expression&)> scan_expr = [&](const Expression& expr) {
        std::visit([&](const auto& e) {
            using T = std::decay_t<decltype(e)>;
            if constexpr (std::is_same_v<T, ArrayAccess>) {
                for (size_t i = 0; i < kernel.params.size(); i++) {
                    if (kernel.params[i] == e.name) {
                        is_array[i] = true;
                    }
                }
                for (const auto& idx : e.indices) scan_expr(*idx);
            } else if constexpr (std::is_same_v<T, BinaryExpr>) {
                scan_expr(*e.left);
                scan_expr(*e.right);
            } else if constexpr (std::is_same_v<T, UnaryExpr>) {
                scan_expr(*e.operand);
            } else if constexpr (std::is_same_v<T, FunctionCall>) {
                for (const auto& arg : e.args) scan_expr(*arg);
            } else if constexpr (std::is_same_v<T, GpuIntrinsic>) {
                scan_expr(*e.dimension);
            }
        }, expr.expr);
    };

    // Helper to scan statements
    std::function<void(const Statement&)> scan_stmt = [&](const Statement& stmt) {
        std::visit([&](const auto& s) {
            using T = std::decay_t<decltype(s)>;
            if constexpr (std::is_same_v<T, LetStmt>) {
                // Check if LHS is an array assignment on a parameter
                if (!s.indices.empty()) {
                    for (size_t i = 0; i < kernel.params.size(); i++) {
                        if (kernel.params[i] == s.var_name) {
                            is_array[i] = true;
                        }
                    }
                    for (const auto& idx : s.indices) scan_expr(*idx);
                }
                scan_expr(*s.value);
            } else if constexpr (std::is_same_v<T, PrintStmt>) {
                for (const auto& item : s.items) scan_expr(*item.expr);
            } else if constexpr (std::is_same_v<T, IfStmt>) {
                scan_expr(*s.condition);
                if (s.then_stmt) scan_stmt(*s.then_stmt);
            }
        }, stmt.stmt);
    };

    for (const auto& body_stmt : kernel.body) {
        scan_stmt(*body_stmt);
    }

    // Emit parameter list
    for (size_t i = 0; i < kernel.params.size(); i++) {
        if (i > 0) out << ", ";
        if (is_array[i]) {
            out << "double* " << kernel.params[i];
        } else {
            out << "double " << kernel.params[i];
        }
    }

    out << ") {\n";

    // Collect local variable names (LET targets that aren't params and aren't array writes)
    std::vector<std::string> locals;
    std::function<void(const Statement&)> collect_locals = [&](const Statement& stmt) {
        std::visit([&](const auto& s) {
            using T = std::decay_t<decltype(s)>;
            if constexpr (std::is_same_v<T, LetStmt>) {
                if (s.indices.empty()) {
                    // Check it's not a parameter name
                    bool is_param = false;
                    for (const auto& p : kernel.params) {
                        if (p == s.var_name) { is_param = true; break; }
                    }
                    if (!is_param) {
                        // Check not already collected
                        bool found = false;
                        for (const auto& l : locals) {
                            if (l == s.var_name) { found = true; break; }
                        }
                        if (!found) locals.push_back(s.var_name);
                    }
                }
            } else if constexpr (std::is_same_v<T, IfStmt>) {
                if (s.then_stmt) collect_locals(*s.then_stmt);
            }
        }, stmt.stmt);
    };

    for (const auto& body_stmt : kernel.body) {
        collect_locals(*body_stmt);
    }

    // Declare locals
    for (const auto& local : locals) {
        out << "    double " << local << " = 0.0;\n";
    }

    // Emit body statements
    for (const auto& body_stmt : kernel.body) {
        emit_stmt(*body_stmt, out, "    ");
    }

    out << "}\n";

    return out.str();
}

} // namespace rocbas
