#pragma once

#include <vector>

#include "ir_node.h"
#include "type.h"
#include "variable.h"

namespace oorgen {

class Context;
class GenPolicy;

//抽象类，用来作为所有表达式的父类
class Expr : public Node {
    public:
        // Expr构造函数
        Expr (Node::NodeID _id, std::shared_ptr<Data> _value, uint32_t _compl) :
              Node(_id), value(_value), complexity(_compl) {}
        // Getters and Setters
        Type::TypeID get_type_id () { return value->get_type()->get_type_id (); }
        virtual std::shared_ptr<Data> get_value ();
        uint32_t get_complexity() { return complexity; }
        static void increase_expr_count(uint32_t val) { total_expr_count += val; func_expr_count += val; }
        static uint32_t get_total_expr_count () { return total_expr_count; }
        static void zero_out_func_expr_count () { func_expr_count = 0; }

    protected:
        // 此函数会将语言标准要求的类型转换（隐式强制转换，Integral提升或常规算术转换）执行到现有子节点。
        // 结果，它在现存子节点和当前节点之间插入所需的TypeCastExpr。
        virtual bool propagate_type () = 0;

        // 此函数基于当前节点的子节点们，计算当前节点的值
        // 也探测UB并且消除它，需要首先调用propagate_type()
        virtual UB propagate_value () = 0;
        std::shared_ptr<Data> value;
        uint32_t complexity;

        // 通过所有的测试程序计算表达式
        static uint32_t total_expr_count;
        // 给每个单独的测试函数计算表达式
        static uint32_t func_expr_count;
};

// Variable Use Expression 提供对变量的访问。
// 此类在生成的测试中与变量进行任何交互（访问其值）均由此类表示。
// 例如，分配给变量可以将VarUseExpr用作lhs
class VarUseExpr : public Expr {
    public:
        VarUseExpr (std::shared_ptr<Data> _var);
        std::shared_ptr<Expr> set_value (std::shared_ptr<Expr> _expr);
        // 此方法可以直接访问基本变量
        std::shared_ptr<Data> get_raw_value () { return value; }
        void emit (std::ostream& stream, std::string offset = "") { stream << value->get_name (); }

    private:
        bool propagate_type () { return true; }
        UB propagate_value () { return NoUB; }
};

// Assignment Expresseion 表示将一个表达式分配给另一个表达式。
// 它的构造函数用TypeCastExpr节点替换隐式强制转换（将rhs转换为lhs的类型）并更新左侧的值（仅当在测试中评估了此分配，即“ taken”为true时）。
// 例如：左侧的表达式 = 右侧的表达式
class AssignExpr : public Expr {
    public:
        AssignExpr (std::shared_ptr<Expr> _to, std::shared_ptr<Expr> _from, bool _taken = true);
        void emit (std::ostream& stream, std::string offset = "");

    private:
        bool propagate_type ();
        UB propagate_value ();

        // 目的 (只能是VarUseExpr or MemberExpr).
        std::shared_ptr<Expr> to;
        // 赋值表达式的右侧
        std::shared_ptr<Expr> from;
        // Taken 表示是否评估表达式以及是否应该更改表达式左侧的值
        bool taken;
};

// Type Cast expression 表示隐式和显式类型转换。
// TypeCastExpr的创建者应做出有关其类型（隐式或显式）的决定，并将其传递给构造函数。
// 所有隐式强制类型转换都应由此类表示。
// 例如：（to_type）expr;
class TypeCastExpr : public Expr {
    public:
        TypeCastExpr (std::shared_ptr<Expr> _expr, std::shared_ptr<Type> _type, bool _is_implicit = false);
        void emit (std::ostream& stream, std::string offset = "");
        static std::shared_ptr<TypeCastExpr> generate (std::shared_ptr<Context> ctx, std::shared_ptr<Expr> from);

    private:
        bool propagate_type ();
        UB propagate_value ();

        std::shared_ptr<Expr> expr;
        std::shared_ptr<Type> to_type;
        // 这个flag表示转换是否时隐式
        // 即如果它时隐式的和省略的，则程序行为不会改变
        bool is_implicit;
};

// Constant expression 代表常数值
// E.g.: 123ULL
// TODO: should we play around with different representation of constants? I.e. decimal,
// hex, octal, with and without C++14 style apostrophes, etc.
class ConstExpr : public Expr {
    public:
        ConstExpr (BuiltinType::ScalarTypedVal _val);
        void emit (std::ostream& stream, std::string offset = "");
        static std::shared_ptr<ConstExpr> generate (std::shared_ptr<Context> ctx);

        // 此函数将填充所用常量的内部缓冲区（对于每种context类型都是唯一的），
        // 并且应在每个Stmt新生成之前调用。
        // 这些缓冲区稍后在ConstExpr :: generate中使用。
        static void fill_const_buf(std::shared_ptr<Context> ctx);

    private:
        // Buffer for constants, used in arithmetic context
        static std::vector<BuiltinType::ScalarTypedVal> arith_const_buffer;
        // Buffer for constants, used in bit-logical context
        static std::vector<BuiltinType::ScalarTypedVal> bit_log_const_buffer;

        template <typename T>
        std::string to_string(T T_val, T min, std::string suffix);
        bool propagate_type () { return true; }
        UB propagate_value () { return NoUB; }
};

// Arithmetic expression 抽象类, 用作一元/二元表达式的公共父类。
// 我们使用自上而下的方法构造表达式树。 
// 之后，我们将自下而上地传播类型和值（如果检测到UB，则将其消除）。
class ArithExpr : public Expr {
    public:
        // ArithExpr的复杂度应在所有转换后手动设置，而不是传递给Expr构造函数
        ArithExpr(Node::NodeID _node_id, std::shared_ptr<Data> _val) : Expr(_node_id, _val, 0) {}
        static std::shared_ptr<Expr> generate (std::shared_ptr<Context> ctx, std::vector<std::shared_ptr<Expr>> inp);

    protected:
        // This function chooses one of ArithSSP::ConstUse patterns and combines old_gen_policy with it
        static GenPolicy choose_and_apply_ssp_const_use (GenPolicy old_gen_policy);
        // This function chooses one of ArithSSP::SimilarOp patterns and combines old_gen_policy with it
        static GenPolicy choose_and_apply_ssp_similar_op (GenPolicy old_gen_policy);
        // Bridge to choose_and_apply_ssp_const_use and choose_and_apply_ssp_similar_op. This function combines both of them.
        static GenPolicy choose_and_apply_ssp (GenPolicy old_gen_policy);
        // Top-level recursive function for expression tree generation
        static std::shared_ptr<Expr> gen_level (std::shared_ptr<Context> ctx, std::vector<std::shared_ptr<Expr>> inp, uint32_t par_depth);

        std::shared_ptr<Expr> integral_prom (std::shared_ptr<Expr> arg);
        std::shared_ptr<Expr> conv_to_bool (std::shared_ptr<Expr> arg);

        // Total number of all generated arithmetic expressions
};

// Unary expression 表示所有的一元操作
// E.g.: +lhs_expr;
class UnaryExpr : public ArithExpr {
    public:
        // 一元操作
        enum Op {
            PreInc,  ///< Pre-increment //no
            PreDec,  ///< Pre-decrement //no
            PostInc, ///< Post-increment //no
            PostDec, ///< Post-decrement //no
            Plus,    ///< Plus //ip
            Negate,  ///< Negation //ip
            LogNot,  ///< Logical not //bool
            BitNot,  ///< Bit not //ip
            MaxOp
        };
        UnaryExpr (Op _op, std::shared_ptr<Expr> _arg);
        Op get_op () { return op; }
        static std::shared_ptr<UnaryExpr> generate (std::shared_ptr<Context> ctx, std::vector<std::shared_ptr<Expr>> inp, uint32_t par_depth);
        void emit (std::ostream& stream, std::string offset = "");

    private:
        bool propagate_type ();
        UB propagate_value ();
        // This function eliminates UB. It changes operator to complimentary.
        void rebuild (UB ub);

        Op op;
        std::shared_ptr<Expr> arg;
};

// Binary expression - 表示所有的二元操作
// E.g.: lhs_expr + rhs_expr;
class BinaryExpr : public ArithExpr {
    public:
        // 二元操作
        enum Op {
            Add   , ///< Addition
            Sub   , ///< Subtraction
            Mul   , ///< Multiplication
            Div   , ///< Division
            Mod   , ///< Modulus
            Shl   , ///< Shift left
            Shr   , ///< Shift right

            Lt    ,  ///< Less than
            Gt    ,  ///< Greater than
            Le    ,  ///< Less than or equal
            Ge    , ///< Greater than or equal
            Eq    , ///< Equal
            Ne    , ///< Not equal

            BitAnd, ///< Bitwise AND
            BitXor, ///< Bitwise XOR
            BitOr , ///< Bitwise OR

            LogAnd, ///< Logical AND
            LogOr , ///< Logical OR

            MaxOp ,
            Ter   , ///< Ternary (BinaryExpr is the easiest way to implement it)
        };

        BinaryExpr (Op _op, std::shared_ptr<Expr> lhs, std::shared_ptr<Expr> rhs);
        Op get_op () { return op; }
        static std::shared_ptr<BinaryExpr> generate (std::shared_ptr<Context> ctx, std::vector<std::shared_ptr<Expr>> inp, uint32_t par_depth);
        void emit (std::ostream& stream, std::string offset = "");

    protected:
        bool propagate_type ();
        UB propagate_value ();
        void perform_arith_conv ();
        // This function eliminates UB. It changes operator to complimentary or inserts new nodes.
        void rebuild (UB ub);

        Op op;
        std::shared_ptr<Expr> arg0;
        std::shared_ptr<Expr> arg1;
};

// ConditionalExpr - 三元条件运算
class ConditionalExpr : public BinaryExpr {
    public:
        ConditionalExpr (std::shared_ptr<Expr> _cond, std::shared_ptr<Expr> lhs, std::shared_ptr<Expr> rhs);
        void emit (std::ostream& stream, std::string offset = "");
        static std::shared_ptr<ConditionalExpr> generate (std::shared_ptr<Context> ctx, std::vector<std::shared_ptr<Expr>> inp, int par_depth);

    private:
        UB propagate_value ();
        std::shared_ptr<Expr> condition;
};

// Member expression - 提供对struct变量成员的访问
// E.g.: struct_obj.member_1
class MemberExpr : public Expr {
    public:
        MemberExpr (std::shared_ptr<Struct> _struct, uint64_t _identifier);
        MemberExpr (std::shared_ptr<MemberExpr> _member_expr, uint64_t _identifier);
        std::shared_ptr<Expr> set_value (std::shared_ptr<Expr> _expr);
        // 此方法提供对基础成员的直接访问
        std::shared_ptr<Data> get_raw_value () { return value; }
        void emit (std::ostream& stream, std::string offset = "");

    private:
        bool propagate_type ();
        UB propagate_value ();
        std::shared_ptr<Expr> check_and_set_bit_field (std::shared_ptr<Expr> _expr);

        std::shared_ptr<MemberExpr> member_expr;
        std::shared_ptr<Struct> struct_var;
        uint64_t identifier;
};

// AddressOf expression - 表示获取地址
class AddressOfExpr : public Expr {
    public:
        AddressOfExpr(std::shared_ptr<Expr> expr);
        void emit (std::ostream& stream, std::string offset = "");

    private:
        bool propagate_type () { return true; }
        UB propagate_value () { return NoUB; }

        std::shared_ptr<Expr> addr_of_expr;
};

// ExprStar - 表示对指针的取消引用
class ExprStar : public Expr {
    public:
        ExprStar(std::shared_ptr<Expr> expr);
        std::shared_ptr<Expr> set_value (std::shared_ptr<Expr> _expr);
        std::shared_ptr<Data> get_value ();
        void emit (std::ostream& stream, std::string offset = "");

    private:
        bool propagate_type () { return true; }
        UB propagate_value () { return NoUB; }

        std::shared_ptr<Expr> expr_star;
};

// Stub expression - 用作未实现功能的辅助功能
class StubExpr : public Expr {
    public:
        StubExpr(std::string _str) : Expr(Node::NodeID::STUB, nullptr, 1), string(_str) {}
        void emit (std::ostream& stream, std::string offset = "") { stream << offset << string; }

    private:
        bool propagate_type () { return true; }
        UB propagate_value () { return NoUB; }

        std::string string;
};
}
