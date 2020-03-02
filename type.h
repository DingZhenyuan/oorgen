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

        // Type构造函数
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

        // 我们假定静态存储周期、CV限定符和字节对齐共同构成了Type的全名
        // 获取Type全名
        std::string get_name ();
        std::string get_simple_name () { return name; }
        virtual std::string get_type_suffix() { return ""; }

        // 帮助快速确定Type种类的函数
        virtual bool is_builtin_type() { return false; }
        virtual bool is_int_type() { return false; }
        virtual bool is_struct_type() { return false; }
        virtual bool is_array_type() { return false; }
        virtual bool is_ptr_type() { return false; }

        // 纯虚函数，debug用
        virtual void dbg_dump() = 0;

        // Type析构函数
        virtual ~Type () {}

    protected:
        std::string name;
        CV_Qual cv_qual;
        bool is_static;
        uint32_t align;

    private:
        TypeID id;
};

// 结构体类
class StructType : public Type {
    public:
        // 代表结构体中成员的类，包括bit-fields
        //TODO: add generator?
        struct StructMember {
            public:
                // StructMember构造函数
                StructMember (std::shared_ptr<Type> _type, std::string _name);
                // 获得名字
                std::string get_name () { return name; }
                // 获得类型指针
                std::shared_ptr<Type> get_type() { return type; }
                // 获得数据指针
                std::shared_ptr<Data> get_data() { return data; }
                // 获得定义程序代码
                std::string get_definition (std::string offset = "");

            private:
                std::shared_ptr<Type> type;
                std::string name;

                std::shared_ptr<Data> data; 
                //TODO: 静态成员
        };

        // StructType构造函数
        StructType (std::string _name) : Type (Type::STRUCT_TYPE), nest_depth(0) { name = _name; }
        StructType (std::string _name, CV_Qual _cv_qual, bool _is_static, uint32_t _align) :
                    Type (Type::STRUCT_TYPE, _cv_qual, _is_static, _align), nest_depth(0) { name = _name; }
        
        // 判断是否时struct类型
        bool is_struct_type() { return true; }

        // Getters and setters
        //TODO: 应该加入对嵌套深度的更改
        // 添加成员变量
        void add_member (std::shared_ptr<StructMember> new_mem) { members.push_back(new_mem); shadow_members.push_back(new_mem); }
        void add_member (std::shared_ptr<Type> _type, std::string _name);
        // 添加shadow member
        void add_shadow_member (std::shared_ptr<Type> _type) { shadow_members.push_back(std::make_shared<StructMember>(_type, "")); }
        // 获取member的个数
        uint32_t get_member_count () { return members.size(); }
        // 获取shadow member的个数
        uint32_t get_shadow_member_count () { return shadow_members.size(); }
        // 获取嵌套深度
        uint32_t get_nest_depth () { return nest_depth; }
        // 获取成员
        std::shared_ptr<StructMember> get_member (unsigned int num);


        // 获取定义程序代码
        std::string get_definition (std::string offset = "");
        // It returns an out-of-line definition for all static members of the structure
        // 为结构体的所有静态成员返回一个out-of-line的定义
        std::string get_static_memb_def (std::string offset = "");
        std::string get_static_memb_check (std::string offset = "");

        //
        void dbg_dump();

        // 随机生成结构体类型
        static std::shared_ptr<StructType> generate (std::shared_ptr<Context> ctx);
        static std::shared_ptr<StructType> generate (std::shared_ptr<Context> ctx, std::vector<std::shared_ptr<StructType>> nested_struct_types);

    private:
        //TODO: it is a stub for unnamed bit fields. Nobody should know about them
        
        // shadow member的容器
        std::vector<std::shared_ptr<StructMember>> shadow_members;
        // 成员的容器
        std::vector<std::shared_ptr<StructMember>> members;
        // 嵌套深度
        uint32_t nest_depth;
};

// 所有已处理未定义行为的ID
enum UB {
    NoUB,
    NullPtr, // nullptr ptr dereferencing
    SignOvf, // Signed overflow
    SignOvfMin, // Special case of signed overflow: INT_MIN * (-1)
    ZeroDiv, // FPE
    ShiftRhsNeg, // Shift by negative value
    ShiftRhsLarge, // // Shift by large value
    NegShift, // Shift of negative value
    NoMemeber, // Can't find member of structure
    MaxUB
};

}