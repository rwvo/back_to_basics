#pragma once

#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace rocbas {

// Forward declarations
struct Expression;
struct Statement;

using ExprPtr = std::unique_ptr<Expression>;
using StmtPtr = std::unique_ptr<Statement>;

// --- Expressions ---

struct NumberLiteral {
    double value;
};

struct StringLiteral {
    std::string value;
};

struct Variable {
    std::string name;  // uppercase, e.g. "X", "COUNT", "N$"
};

struct ArrayAccess {
    std::string name;
    std::vector<ExprPtr> indices;  // supports multi-dimensional: A(I,J)
};

enum class BinaryOp {
    ADD, SUB, MUL, DIV, POW,   // arithmetic
    EQ, NE, LT, GT, LE, GE,   // comparison
    AND, OR,                    // logical
};

struct BinaryExpr {
    BinaryOp op;
    ExprPtr left;
    ExprPtr right;
};

enum class UnaryOp {
    NEGATE,  // -expr
    NOT,     // NOT expr
};

struct UnaryExpr {
    UnaryOp op;
    ExprPtr operand;
};

struct FunctionCall {
    std::string name;  // e.g. "ABS", "LEFT$"
    std::vector<ExprPtr> args;
};

// GPU kernel intrinsic: THREAD_IDX(d), BLOCK_IDX(d), BLOCK_DIM(d), GRID_DIM(d)
enum class GpuIntrinsicKind {
    THREAD_IDX,
    BLOCK_IDX,
    BLOCK_DIM,
    GRID_DIM,
};

struct GpuIntrinsic {
    GpuIntrinsicKind kind;
    ExprPtr dimension;  // 1, 2, or 3 (evaluated at codegen time)
};

// MPI intrinsic: MPI RANK, MPI SIZE
enum class MpiIntrinsicKind {
    RANK,
    SIZE,
};

struct MpiIntrinsic {
    MpiIntrinsicKind kind;
};

struct Expression {
    std::variant<
        NumberLiteral,
        StringLiteral,
        Variable,
        ArrayAccess,
        BinaryExpr,
        UnaryExpr,
        FunctionCall,
        GpuIntrinsic,
        MpiIntrinsic
    > expr;

    template <typename T>
    Expression(T&& val) : expr(std::forward<T>(val)) {}
};

// Convenience constructors
inline ExprPtr make_expr(double value) {
    return std::make_unique<Expression>(NumberLiteral{value});
}

inline ExprPtr make_expr(const std::string& str_val) {
    return std::make_unique<Expression>(StringLiteral{str_val});
}

inline ExprPtr make_var(const std::string& name) {
    return std::make_unique<Expression>(Variable{name});
}

// --- Statements ---

struct LetStmt {
    std::string var_name;
    std::vector<ExprPtr> indices;  // empty for scalar, non-empty for array element
    ExprPtr value;
};

struct PrintStmt {
    struct Item {
        ExprPtr expr;
        bool suppress_newline;  // true if followed by ';'
    };
    std::vector<Item> items;
    bool trailing_newline;  // false if last item has ';'
};

struct InputStmt {
    std::string prompt;      // optional prompt string
    std::string var_name;    // variable to read into
};

struct GotoStmt {
    int target_line;
};

struct GosubStmt {
    int target_line;
};

struct ReturnStmt {};

struct IfStmt {
    ExprPtr condition;
    StmtPtr then_stmt;       // single statement after THEN
    int goto_line;           // if THEN is followed by a line number (GOTO shorthand)
    bool has_goto;           // true if THEN <line_number> form
};

struct ForStmt {
    std::string var_name;
    ExprPtr start;
    ExprPtr end;
    ExprPtr step;            // nullptr means default step of 1
};

struct NextStmt {
    std::string var_name;
};

struct DimStmt {
    struct ArrayDecl {
        std::string name;
        std::vector<ExprPtr> dimensions;
    };
    std::vector<ArrayDecl> arrays;
};

struct EndStmt {};

struct RemStmt {
    std::string comment;
};

struct DataStmt {
    std::vector<std::string> values;
};

struct ReadStmt {
    std::vector<std::string> var_names;
};

struct RestoreStmt {};

// --- GPU Statements ---

struct GpuDimStmt {
    struct ArrayDecl {
        std::string name;
        std::vector<ExprPtr> dimensions;
    };
    std::vector<ArrayDecl> arrays;
};

struct GpuCopyStmt {
    std::string src;
    std::string dst;
    // Direction inferred: if src is GPU array → device-to-host, else host-to-device
};

struct GpuFreeStmt {
    std::string name;
};

struct GpuKernelStmt {
    std::string name;
    std::vector<std::string> params;
    std::vector<StmtPtr> body;  // kernel body statements (span multiple BASIC lines)
};

struct OptionBaseStmt {
    bool is_gpu;   // true for OPTION GPU BASE, false for OPTION BASE
    double base;   // 0, 0.5, or 1
};

struct GpuGosubStmt {
    std::string kernel_name;
    std::vector<ExprPtr> args;
    // Grid dimensions (1D: single expr, 3D: three exprs)
    std::vector<ExprPtr> grid_dims;   // number of blocks
    std::vector<ExprPtr> block_dims;  // threads per block
};

// --- MPI Statements ---

struct MpiInitStmt {};

struct MpiFinalizeStmt {};

struct MpiSendStmt {
    std::string array_name;
    ExprPtr lo_index;    // nullptr for whole-array
    ExprPtr hi_index;    // nullptr for whole-array
    ExprPtr dest;
    ExprPtr tag;
};

struct MpiRecvStmt {
    std::string array_name;
    ExprPtr lo_index;    // nullptr for whole-array
    ExprPtr hi_index;    // nullptr for whole-array
    ExprPtr src;
    ExprPtr tag;
};

struct MpiBarrierStmt {};

struct Statement {
    int line_number;  // BASIC line number (10, 20, 30, ...)
    std::variant<
        LetStmt,
        PrintStmt,
        InputStmt,
        GotoStmt,
        GosubStmt,
        ReturnStmt,
        IfStmt,
        ForStmt,
        NextStmt,
        DimStmt,
        EndStmt,
        RemStmt,
        DataStmt,
        ReadStmt,
        RestoreStmt,
        GpuDimStmt,
        GpuCopyStmt,
        GpuFreeStmt,
        GpuKernelStmt,
        GpuGosubStmt,
        OptionBaseStmt,
        MpiInitStmt,
        MpiFinalizeStmt,
        MpiSendStmt,
        MpiRecvStmt,
        MpiBarrierStmt
    > stmt;

    template <typename T>
    Statement(int line, T&& val) : line_number(line), stmt(std::forward<T>(val)) {}
};

// A complete BASIC program
struct Program {
    std::vector<StmtPtr> lines;  // sorted by line number
};

} // namespace rocbas
