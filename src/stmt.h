#pragma once

#include <ostream>
#include <vector>

#include "type.h"
#include "variable.h"
#include "ir_node.h"
#include "expr.h"

namespace oorgen {

class Context;

// 抽象类，作为所有statements的公共父类
class Stmt : public Node {
    public:
        Stmt (Node::NodeID _id) : Node(_id) {};

        static void increase_stmt_count() { total_stmt_count++; func_stmt_count++; }
        static void zero_out_func_stmt_count () { func_stmt_count = 0; }

    protected:
        // Count of statements over all test program
        static uint32_t total_stmt_count;
        // Counter of statements per single test function
        static uint32_t func_stmt_count;
};

// Declaration statement 会创建新变量（在当前context中声明变量）并将其添加到本地符号表中：例如：variable_declaration = init_statement;
// 还提供了外部声明的发出（此功能仅在打印过程中使用）：例如：extern variable_declaration;
class DeclStmt : public Stmt {
    public:
        DeclStmt (std::shared_ptr<Data> _data, std::shared_ptr<Expr> _init, bool _is_extern = false);
        void set_is_extern (bool _is_extern) { is_extern = _is_extern; }
        std::shared_ptr<Data> get_data () { return data; }
        void emit (std::ostream& stream, std::string offset = "");
        // count_up_total determines whether to increase Expr::total_expr_count or not (used for CSE)
        static std::shared_ptr<DeclStmt> generate (std::shared_ptr<Context> ctx,
                                                   std::vector<std::shared_ptr<Expr>> inp,
                                                   bool count_up_total);

    private:
        std::shared_ptr<Data> data;
        std::shared_ptr<Expr> init;
        bool is_extern;
};

// Expression statement 将任何表达式“转换”为语句。
// 例如，它允许使用AssignExpr作为语句：var_16 = 123ULL * 10;
class ExprStmt : public Stmt {
    public:
        ExprStmt (std::shared_ptr<Expr> _expr) : Stmt(Node::NodeID::EXPR), expr(_expr) {}
        void emit (std::ostream& stream, std::string offset = "");
        // For info about count_up_total see note above
        static std::shared_ptr<ExprStmt> generate (std::shared_ptr<Context> ctx,
                                                   std::vector<std::shared_ptr<Expr>> inp,
                                                   std::shared_ptr<Expr> out,
                                                   bool count_up_total);

    private:
        std::shared_ptr<Expr> expr;
};

// Scope statement 表示作用于和内容:
// E.g.:
// {
//     ...
// }
//TODO: it also fills global SymTable at startup. Program class should do it.
class ScopeStmt : public Stmt {
    public:
        ScopeStmt () : Stmt(Node::NodeID::SCOPE) {}
        void add_stmt (std::shared_ptr<Stmt> stmt) { scope.push_back(stmt); }
        void emit (std::ostream& stream, std::string offset = "");
        static std::shared_ptr<ScopeStmt> generate (std::shared_ptr<Context> ctx);

    private:
        using ExprVector = std::vector<std::shared_ptr<Expr>>;
        static ExprVector extract_inp_from_ctx(std::shared_ptr<Context> ctx);
        static ExprVector extract_locals_from_ctx(std::shared_ptr<Context> ctx);
        static ExprVector extract_inp_and_mix_from_ctx(std::shared_ptr<Context> ctx);
        static std::map<std::string, ExprVector> extract_all_local_ptr_exprs(std::shared_ptr<Context> ctx);

        std::vector<std::shared_ptr<Stmt>> scope;
};

// If statement - 表示if-else语句，else可选
// E.g.:
// if (cond) {
// ...
// }
// <else {
// ...
// }>
class IfStmt : public Stmt {
    public:
        IfStmt (std::shared_ptr<Expr> cond, std::shared_ptr<ScopeStmt> if_branch,
                std::shared_ptr<ScopeStmt> else_branch);
        static bool count_if_taken (std::shared_ptr<Expr> cond);
        void emit (std::ostream& stream, std::string offset = "");
        // For info about count_up_total see note above
        static std::shared_ptr<IfStmt> generate (std::shared_ptr<Context> ctx,
                                                 std::vector<std::shared_ptr<Expr>> inp,
                                                 bool count_up_total);

    private:
        // TODO: do we need it? It should indicate whether the scope is evaluated.
        bool taken;
        std::shared_ptr<Expr> cond;
        std::shared_ptr<ScopeStmt> if_branch;
        std::shared_ptr<ScopeStmt> else_branch;
};
}
