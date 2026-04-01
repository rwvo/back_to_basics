#include <gtest/gtest.h>
#include "lexer.h"
#include "parser.h"
#include "ast.h"

using namespace rocbas;

static Program parse(const std::string& source) {
    Lexer lexer(source);
    auto tokens = lexer.tokenize();
    Parser parser(tokens);
    return parser.parse();
}

template <typename T>
const T& get_stmt(const Program& prog, size_t index) {
    return std::get<T>(prog.lines.at(index)->stmt);
}

// --- MPI INIT / FINALIZE / BARRIER ---

TEST(MpiParser, MpiInit) {
    auto prog = parse("10 MPI INIT\n");
    ASSERT_EQ(prog.lines.size(), 1u);
    get_stmt<MpiInitStmt>(prog, 0);  // should not throw
}

TEST(MpiParser, MpiFinalize) {
    auto prog = parse("10 MPI FINALIZE\n");
    ASSERT_EQ(prog.lines.size(), 1u);
    get_stmt<MpiFinalizeStmt>(prog, 0);
}

TEST(MpiParser, MpiBarrier) {
    auto prog = parse("10 MPI BARRIER\n");
    ASSERT_EQ(prog.lines.size(), 1u);
    get_stmt<MpiBarrierStmt>(prog, 0);
}

// --- MPI SEND ---

TEST(MpiParser, MpiSendWholeArray) {
    auto prog = parse("10 MPI SEND A TO 1 TAG 0\n");
    ASSERT_EQ(prog.lines.size(), 1u);
    auto& s = get_stmt<MpiSendStmt>(prog, 0);
    EXPECT_EQ(s.array_name, "A");
    EXPECT_EQ(s.lo_index, nullptr);
    EXPECT_EQ(s.hi_index, nullptr);
    ASSERT_TRUE(std::holds_alternative<NumberLiteral>(s.dest->expr));
    EXPECT_DOUBLE_EQ(std::get<NumberLiteral>(s.dest->expr).value, 1.0);
    ASSERT_TRUE(std::holds_alternative<NumberLiteral>(s.tag->expr));
    EXPECT_DOUBLE_EQ(std::get<NumberLiteral>(s.tag->expr).value, 0.0);
}

TEST(MpiParser, MpiSendRange) {
    auto prog = parse("10 MPI SEND A(1) THRU A(N) TO 1 TAG 0\n");
    ASSERT_EQ(prog.lines.size(), 1u);
    auto& s = get_stmt<MpiSendStmt>(prog, 0);
    EXPECT_EQ(s.array_name, "A");
    ASSERT_NE(s.lo_index, nullptr);
    ASSERT_NE(s.hi_index, nullptr);
    ASSERT_TRUE(std::holds_alternative<NumberLiteral>(s.lo_index->expr));
    ASSERT_TRUE(std::holds_alternative<Variable>(s.hi_index->expr));
    EXPECT_EQ(std::get<Variable>(s.hi_index->expr).name, "N");
}

TEST(MpiParser, MpiSendExprDest) {
    auto prog = parse("10 MPI SEND A TO RANK + 1 TAG 1\n");
    auto& s = get_stmt<MpiSendStmt>(prog, 0);
    ASSERT_TRUE(std::holds_alternative<BinaryExpr>(s.dest->expr));
    auto& bin = std::get<BinaryExpr>(s.dest->expr);
    EXPECT_EQ(bin.op, BinaryOp::ADD);
}

// --- MPI RECV ---

TEST(MpiParser, MpiRecvWholeArray) {
    auto prog = parse("10 MPI RECV B FROM 0 TAG 0\n");
    ASSERT_EQ(prog.lines.size(), 1u);
    auto& r = get_stmt<MpiRecvStmt>(prog, 0);
    EXPECT_EQ(r.array_name, "B");
    EXPECT_EQ(r.lo_index, nullptr);
    EXPECT_EQ(r.hi_index, nullptr);
    ASSERT_TRUE(std::holds_alternative<NumberLiteral>(r.src->expr));
}

TEST(MpiParser, MpiRecvRange) {
    auto prog = parse("10 MPI RECV B(1) THRU B(N) FROM 0 TAG 0\n");
    auto& r = get_stmt<MpiRecvStmt>(prog, 0);
    EXPECT_EQ(r.array_name, "B");
    ASSERT_NE(r.lo_index, nullptr);
    ASSERT_NE(r.hi_index, nullptr);
}

// --- MPI RANK / MPI SIZE expressions ---

TEST(MpiParser, MpiRankExpression) {
    auto prog = parse("10 LET R = MPI RANK\n");
    auto& let = get_stmt<LetStmt>(prog, 0);
    ASSERT_TRUE(std::holds_alternative<MpiIntrinsic>(let.value->expr));
    EXPECT_EQ(std::get<MpiIntrinsic>(let.value->expr).kind, MpiIntrinsicKind::RANK);
}

TEST(MpiParser, MpiSizeExpression) {
    auto prog = parse("10 LET S = MPI SIZE\n");
    auto& let = get_stmt<LetStmt>(prog, 0);
    ASSERT_TRUE(std::holds_alternative<MpiIntrinsic>(let.value->expr));
    EXPECT_EQ(std::get<MpiIntrinsic>(let.value->expr).kind, MpiIntrinsicKind::SIZE);
}

TEST(MpiParser, MpiRankInExpression) {
    auto prog = parse("10 LET X = MPI RANK + 1\n");
    auto& let = get_stmt<LetStmt>(prog, 0);
    ASSERT_TRUE(std::holds_alternative<BinaryExpr>(let.value->expr));
    auto& bin = std::get<BinaryExpr>(let.value->expr);
    EXPECT_EQ(bin.op, BinaryOp::ADD);
    ASSERT_TRUE(std::holds_alternative<MpiIntrinsic>(bin.left->expr));
    EXPECT_EQ(std::get<MpiIntrinsic>(bin.left->expr).kind, MpiIntrinsicKind::RANK);
}

TEST(MpiParser, MpiSizeInComparison) {
    auto prog = parse("10 IF MPI RANK < MPI SIZE - 1 THEN PRINT \"not last\"\n");
    auto& if_stmt = get_stmt<IfStmt>(prog, 0);
    ASSERT_TRUE(std::holds_alternative<BinaryExpr>(if_stmt.condition->expr));
}

// --- Full program ---

TEST(MpiParser, MpiFullProgram) {
    auto prog = parse(
        "10 MPI INIT\n"
        "20 LET RANK = MPI RANK\n"
        "30 LET SIZE = MPI SIZE\n"
        "40 MPI BARRIER\n"
        "50 MPI FINALIZE\n"
    );
    ASSERT_EQ(prog.lines.size(), 5u);
    get_stmt<MpiInitStmt>(prog, 0);
    get_stmt<LetStmt>(prog, 1);
    get_stmt<LetStmt>(prog, 2);
    get_stmt<MpiBarrierStmt>(prog, 3);
    get_stmt<MpiFinalizeStmt>(prog, 4);
}

// --- Error cases ---

TEST(MpiParser, MpiInvalidSubcommand) {
    EXPECT_THROW(parse("10 MPI BLAH\n"), std::runtime_error);
}

TEST(MpiParser, MpiSendMissingTo) {
    EXPECT_THROW(parse("10 MPI SEND A 1 TAG 0\n"), std::runtime_error);
}

TEST(MpiParser, MpiRecvMissingFrom) {
    EXPECT_THROW(parse("10 MPI RECV B 0 TAG 0\n"), std::runtime_error);
}
