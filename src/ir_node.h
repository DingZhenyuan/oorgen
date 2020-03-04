#pragma once

#include <ostream>
#include <vector>

#include "type.h"
#include "variable.h"

namespace oorgen {

class Node {
    public:
        enum NodeID {
            // Expr types
            MIN_EXPR_ID,
            ASSIGN,
            BINARY,
            CONST,
//            EXPR_LIST,
//            FUNC_CALL,
            TYPE_CAST,
            UNARY,
            VAR_USE,
            MEMBER,
            REFERENCE,
            DEREFERENCE,
            STUB,
            MAX_EXPR_ID,
            // Stmt type
            MIN_STMT_ID,
            DECL,
            EXPR,
            SCOPE,
//            CNT_LOOP,
            IF,
//            BREAK,
//            CONTINUE,
            MAX_STMT_ID
        };
        Node (NodeID _id) : id(_id) {}
        NodeID get_id () { return id; }
        virtual void emit (std::ostream& stream, std::string offset = "") = 0;

        virtual ~Node () {}

    private:
        NodeID id;
};

}
