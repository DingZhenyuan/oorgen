#pragma once

#include <iostream>
#include <climits>
#include <memory>
#include <vector>
#include "options.h"

namespace oorgen {

class Context;
class Data;
class ScalarVariable;
class Struct;

// 抽象类，作为所有types的ancestor
class Type {
    public:
        // 顶层Type种类的ID
        enum TypeID {
            BUILTIN_TYPE,
            STRUCT_TYPE,
            ARRAY_TYPE,
            POINTER_TYPE,
            MAX_TYPE_ID
        };

        // CV限定符
        enum CV_Qual {
            NTHG,
            VOLAT,
            CONST,
            CONST_VOLAT,
            MAX_CV_QUAL
        };

        // 基本类型的ID
        enum BuiltinTypeID {
            Integer, Max_BuiltinTypeID
        };

        // 整数类型的ID
        enum IntegerTypeID {
            BOOL,
            // 注意，char和signed char类型不进行区分，虽然在C++标准中不同，在一切实现中char可能对应unsigned char。这里char就假定为signed char
            CHAR,
            UCHAR,
            SHRT,
            USHRT,
            INT,
            UINT,
            LINT,
            ULINT,
            LLINT,
            ULLINT,
            MAX_INT_ID,
        };

        // 构造函数
        Type (TypeID _id) : cv_qual(CV_Qual::NTHG), is_static(false), align(0), id (_id) {}
        Type (TypeID _id, CV_Qual _cv_qual, bool _is_static, uint32_t _align) :
              cv_qual (_cv_qual), is_static (_is_static), align (_align), id (_id) {}
        
        // Getters和Setters
        Type::TypeID get_type_id () { return id; }
        virtual BuiltinTypeID get_builtin_type_id() { return Max_BuiltinTypeID; }
        virtual IntegerTypeID get_int_type_id () { return MAX_INT_ID; }
        virtual bool get_is_signed() { return false; }
        virtual bool get_is_bit_field() { return false; }
        // cv限定符
        void set_cv_qual (CV_Qual _cv_qual) { cv_qual = _cv_qual; }
        CV_Qual get_cv_qual () { return cv_qual; }
        // 静态与否
        void set_is_static (bool _is_static) { is_static = _is_static; }
        bool get_is_static () { return is_static; }
        // align字节对齐
        void set_align (uint32_t _align) { align = _align; }
        uint32_t get_align () { return align; }

    protected:
        std::string name;
        CV_Qual cv_qual;
        bool is_static;
        uint32_t align;

    private:
        TypeID id;
};

}