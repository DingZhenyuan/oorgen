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

// 所有基本类型的共同父类.
class BuiltinType : public Type {
    public:
        // 将Type和Value连接在一起（和Type保持一直）
        class ScalarTypedVal {
            public:
                // 值（各种类型）
                union Val {
                    bool bool_val;
                    signed char char_val;
                    unsigned char uchar_val;
                    short shrt_val;
                    unsigned short ushrt_val;
                    int int_val;
                    unsigned int uint_val;
                    int lint32_val; // for 32-bit mode
                    unsigned int ulint32_val; // for 32-bit mode
                    long long int lint64_val; // for 64-bit mode
                    unsigned long long int ulint64_val; // for 64-bit mode
                    long long int llint_val;
                    unsigned long long int ullint_val;
                };

                // ScalarTypedVal构造函数
                ScalarTypedVal (BuiltinType::IntegerTypeID _int_type_id) : int_type_id(_int_type_id), res_of_ub(NoUB) { val.ullint_val = 0; }
                ScalarTypedVal (BuiltinType::IntegerTypeID _int_type_id, UB _res_of_ub) : int_type_id (_int_type_id), res_of_ub(_res_of_ub)  { val.ullint_val = 0; }
                Type::IntegerTypeID get_int_type_id () const { return int_type_id; }

                // Utility functions for UB
                UB get_ub () { return res_of_ub; }
                void set_ub (UB _ub) { res_of_ub = _ub; }
                bool has_ub () { return res_of_ub != NoUB; }

                // Interface to value through uint64_t
                //TODO: it is a stub for shift rebuild. Can we do it better?
                uint64_t get_abs_val ();
                void set_abs_val (uint64_t new_val);

                // Functions which implements UB detection and semantics of all operators
                ScalarTypedVal cast_type (Type::IntegerTypeID to_type_id);
                ScalarTypedVal operator++ (int) { return pre_op(true ); } // Postfix, but used also as prefix
                ScalarTypedVal operator-- (int) { return pre_op(false); }// Postfix, but used also as prefix
                ScalarTypedVal operator- ();
                ScalarTypedVal operator~ ();
                ScalarTypedVal operator! ();

                ScalarTypedVal operator+ (ScalarTypedVal rhs);
                ScalarTypedVal operator- (ScalarTypedVal rhs);
                ScalarTypedVal operator* (ScalarTypedVal rhs);
                ScalarTypedVal operator/ (ScalarTypedVal rhs);
                ScalarTypedVal operator% (ScalarTypedVal rhs);
                ScalarTypedVal operator< (ScalarTypedVal rhs);
                ScalarTypedVal operator> (ScalarTypedVal rhs);
                ScalarTypedVal operator<= (ScalarTypedVal rhs);
                ScalarTypedVal operator>= (ScalarTypedVal rhs);
                ScalarTypedVal operator== (ScalarTypedVal rhs);
                ScalarTypedVal operator!= (ScalarTypedVal rhs);
                ScalarTypedVal operator&& (ScalarTypedVal rhs);
                ScalarTypedVal operator|| (ScalarTypedVal rhs);
                ScalarTypedVal operator& (ScalarTypedVal rhs);
                ScalarTypedVal operator| (ScalarTypedVal rhs);
                ScalarTypedVal operator^ (ScalarTypedVal rhs);
                ScalarTypedVal operator<< (ScalarTypedVal rhs);
                ScalarTypedVal operator>> (ScalarTypedVal rhs);

                // 随机生成 ScalarTypedVal
                static ScalarTypedVal generate (std::shared_ptr<Context> ctx, BuiltinType::IntegerTypeID _int_type_id);
                static ScalarTypedVal generate (std::shared_ptr<Context> ctx, ScalarTypedVal min, ScalarTypedVal max);

                // 自身的值
                Val val;

            private:
                // Common fuction for all pre-increment and post-increment operators
                ScalarTypedVal pre_op (bool inc);

                BuiltinType::IntegerTypeID int_type_id;
                // If we can use the value or it was obtained from operation with UB
                UB res_of_ub;
        };

        BuiltinType (BuiltinTypeID _builtin_id) : Type (Type::BUILTIN_TYPE), bit_size (0), suffix(""), builtin_id (_builtin_id) {}
        BuiltinType (BuiltinTypeID _builtin_id, CV_Qual _cv_qual, bool _is_static, uint32_t _align) :
                    Type (Type::BUILTIN_TYPE, _cv_qual, _is_static, _align), bit_size (0), suffix(""), builtin_id (_builtin_id) {}
        bool is_builtin_type() { return true; }

        // Getters for BuiltinType properties
        BuiltinTypeID get_builtin_type_id() { return builtin_id; }
        uint32_t get_bit_size () { return bit_size; }
        std::string get_int_literal_suffix() { return suffix; }

    protected:
        unsigned int bit_size;
        // Suffix for integer literals
        std::string suffix;

    private:
        BuiltinTypeID builtin_id;
};

std::ostream& operator<< (std::ostream &out, const BuiltinType::ScalarTypedVal &scalar_typed_val);

// Class which serves as common ancestor for all standard integer types, bool and bit-fields
class IntegerType : public BuiltinType {
    public:
        IntegerType (IntegerTypeID it_id) : BuiltinType (BuiltinTypeID::Integer), is_signed (false), min(it_id), max(it_id), int_type_id (it_id) {}
        IntegerType (IntegerTypeID it_id, CV_Qual _cv_qual, bool _is_static, uint32_t _align) :
                     BuiltinType (BuiltinTypeID::Integer, _cv_qual, _is_static, _align),
                     is_signed (false), min(it_id), max(it_id), int_type_id (it_id) {}
        bool is_int_type() { return true; }

        // Getters for IntegerType properties
        IntegerTypeID get_int_type_id () { return int_type_id; }
        bool get_is_signed () { return is_signed; }
        BuiltinType::ScalarTypedVal get_min () { return min; }
        BuiltinType::ScalarTypedVal get_max () { return max; }

        // This utility functions take IntegerTypeID and return shared pointer to corresponding type
        static std::shared_ptr<IntegerType> init (BuiltinType::IntegerTypeID _type_id);
        static std::shared_ptr<IntegerType> init (BuiltinType::IntegerTypeID _type_id, CV_Qual _cv_qual, bool _is_static, uint32_t _align);

        // If type A can represent all the values of type B
        static bool can_repr_value (BuiltinType::IntegerTypeID A, BuiltinType::IntegerTypeID B); // if type B can represent all of the values of the type A
        // Returns corresponding unsigned type
        static BuiltinType::IntegerTypeID get_corr_unsig (BuiltinType::IntegerTypeID _type_id);

        // Randomly generate IntegerType (except bit-fields)
        static std::shared_ptr<IntegerType> generate (std::shared_ptr<Context> ctx);

    protected:
        bool is_signed;
        // Minimum and maximum value, which can fit in type
        BuiltinType::ScalarTypedVal min;
        BuiltinType::ScalarTypedVal max;

    private:
        IntegerTypeID int_type_id;
};

// Class which represents bit-field
class BitField : public IntegerType {
    public:
        BitField (IntegerTypeID it_id, uint32_t _bit_size) : IntegerType(it_id) { init_type(it_id, _bit_size); }
        BitField (IntegerTypeID it_id, uint32_t _bit_size, CV_Qual _cv_qual) : IntegerType(it_id, _cv_qual, false, 0) { init_type(it_id, _bit_size); }

        // Getters of BitField properties
        bool get_is_bit_field() { return true; }
        uint32_t get_bit_field_width() { return bit_field_width; }

        // If all values of the bit-field can fit in signed/unsigned int
        static bool can_fit_in_int (BuiltinType::ScalarTypedVal val, bool is_unsigned);

        // Randomly generate BitField
        static std::shared_ptr<BitField> generate (std::shared_ptr<Context> ctx, bool is_unnamed = false);

        void dbg_dump ();

    private:
        // Common initializer functions, used in constructors
        void init_type(IntegerTypeID it_id, uint32_t _bit_size);
        uint32_t bit_field_width;
};

}
