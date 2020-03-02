#include <cassert>
#include <sstream>

#include "options.h"
#include "type.h"

using namespace oorgen;

// 获取Type全名
std::string Type::get_name () {
    std::string ret = "";
    ret += is_static ? "static " : "";
    switch (cv_qual) {
        case CV_Qual::VOLAT:
            ret += "volatile ";
            break;
        case CV_Qual::CONST:
            ret += "const ";
            break;
        case CV_Qual::CONST_VOLAT:
            ret += "const volatile ";
            break;
        case CV_Qual::NTHG:
            break;
        case CV_Qual::MAX_CV_QUAL:
            ERROR("bad cv_qual (Type)");
            break;
    }
    ret += name;
    if (align != 0)
        ret += " __attribute__(aligned(" + std::to_string(align) + "))";
    return ret;
}

// StructMember构造函数
StructType::StructMember::StructMember (std::shared_ptr<Type> _type, std::string _name) : type(_type), name(_name), data(nullptr) {
    if (!type->get_is_static())
        return;
    if (type->is_int_type())
        data = std::make_shared<ScalarVariable>(name, std::static_pointer_cast<IntegerType>(type));
    else if (type->is_struct_type())
        data = std::make_shared<Struct>(name, std::static_pointer_cast<StructType>(type));
    else {
        ERROR("unsupported data type (StructType)");
    }
}

// 添加成员
void StructType::add_member (std::shared_ptr<Type> _type, std::string _name) {
    StructType::StructMember new_mem (_type, _name);
    if (_type->is_struct_type()) {
        // 更改结构体嵌套深度
        nest_depth = std::static_pointer_cast<StructType>(_type)->get_nest_depth() >= nest_depth ?
                     std::static_pointer_cast<StructType>(_type)->get_nest_depth() + 1 : nest_depth;
    }
    members.push_back(std::make_shared<StructMember>(new_mem));
    shadow_members.push_back(std::make_shared<StructMember>(new_mem));
}

// 获取member
std::shared_ptr<StructType::StructMember> StructType::get_member (unsigned int num) {
    if (num >= members.size())
        return nullptr;
    else
        return members.at(num);
}

// 获得定义程序代码
std::string StructType::StructMember::get_definition (std::string offset) {
    std::string ret = offset + type->get_name() + " " + name;
    if (type->get_is_bit_field())
        ret += " : " + std::to_string(std::static_pointer_cast<BitField>(type)->get_bit_field_width());
    return ret;
}

// 获得定义程序代码
std::string StructType::get_definition (std::string offset) {
    std::string ret = "";
    if (options->is_c())
        ret += "typedef ";
    ret += "struct ";
    if (options->is_cxx())
        ret += name;
    ret += " {\n";
    for (auto i : shadow_members) {
        ret += i->get_definition(offset + "    ") + ";\n";
    }
    ret += "}";
    if (options->is_c())
        ret += " " + name;
    ret += ";\n";
    return ret;
}

// This function implements single iteration of loop of static members' initialization emission
// 完成静态变量初始化的单次循环
static std::string static_memb_init_iter(std::shared_ptr<Data> member) {
    std::string ret;
    if (member->get_class_id() == Data::VAR) {
        ConstExpr init_expr(std::static_pointer_cast<ScalarVariable>(member)->get_init_value());
        std::stringstream sstream;
        init_expr.emit(sstream);
        ret += sstream.str();
    } else if (member->get_class_id() == Data::STRUCT) {
        std::shared_ptr<Struct> member_struct = std::static_pointer_cast<Struct>(member);
        // Recursively walk over all members
        ret += "{";
        uint64_t member_count = member_struct->get_member_count();
        for (unsigned int i = 0; i < member_count; ++i) {
            std::shared_ptr<Data> cur_member = member_struct->get_member(i);
            if (cur_member->get_type()->get_is_static())
                continue;
            ret += static_memb_init_iter(cur_member) + (i != member_count - 1 ? ", " : "");
        }
        ret += "}";
    } else
        ERROR("bad Data::ClassID");

    return ret;
}

// 获取静态成员的定义程序
std::string StructType::get_static_memb_def (std::string offset) {
    std::string ret;
    for (const auto& i : members)
        if (i->get_type()->get_is_static())
            ret += offset + i->get_type()->get_simple_name() + " " + name + "::" + i->get_name() + " = " +
                   static_memb_init_iter(i->get_data()) + ";\n";
    return ret;
}

// This function implements single iteration of loop of static members' check emission
// 完成静态成员的检查放出的循环的单次迭代
static std::string static_memb_check_iter(std::string offset, std::string parent_str, std::shared_ptr<Data> member) {
    std::string ret;
    parent_str = parent_str + member->get_name();
    if (member->get_class_id() == Data::VAR)
        ret += offset + "hash(&seed, " + parent_str + ");\n";
    else if (member->get_class_id() == Data::STRUCT) {
        std::shared_ptr<Struct> member_struct = std::static_pointer_cast<Struct>(member);
        // Recursively walk over all members
        for (unsigned int i = 0; i < member_struct->get_member_count(); ++i) {
            std::shared_ptr<Data> cur_member = member_struct->get_member(i);
            if (cur_member->get_type()->get_is_static())
                continue;
            ret += static_memb_check_iter(offset, parent_str + ".", cur_member);
        }
    } else
        ERROR("bad Data::ClassID");

    return ret;
}

// 
std::string StructType::get_static_memb_check (std::string offset) {
    std::string ret;
    for (const auto& i : members)
        if (i->get_type()->get_is_static())
        ret += static_memb_check_iter(offset, name + "::", i->get_data());
    return ret;
}

void StructType::dbg_dump() {
    std::cout << get_definition () << std::endl;
    std::cout << "depth: " << nest_depth << std::endl;
}

std::shared_ptr<StructType> StructType::generate (std::shared_ptr<Context> ctx) {
    std::vector<std::shared_ptr<StructType>> empty_vec;
    return generate(ctx, empty_vec);
}

std::shared_ptr<StructType> StructType::generate (std::shared_ptr<Context> ctx,
                                                  std::vector<std::shared_ptr<StructType>> nested_struct_types) {
    auto p = ctx->get_gen_policy();
    Type::CV_Qual primary_cv_qual = rand_val_gen->get_rand_elem(p->get_allowed_cv_qual());

    bool primary_static_spec = false;
    //TODO: add distr to gen_policy
    if (p->get_allow_static_var())
        primary_static_spec = rand_val_gen->get_rand_value(false, true);

    IntegerType::IntegerTypeID int_type_id = rand_val_gen->get_rand_id(p->get_allowed_int_types());
    //TODO: what about align?
    std::shared_ptr<Type> primary_type = IntegerType::init(int_type_id, primary_cv_qual, primary_static_spec, 0);

    NameHandler& name_handler = NameHandler::get_instance();
    std::shared_ptr<StructType> struct_type = std::make_shared<StructType>(name_handler.get_struct_type_name());
    int struct_member_count = rand_val_gen->get_rand_value(p->get_min_struct_member_count(),
                                                           p->get_max_struct_member_count());
    int member_count = 0;
    for (int i = 0; i < struct_member_count; ++i) {
        if (p->get_allow_mix_cv_qual_in_struct())
            primary_cv_qual = rand_val_gen->get_rand_elem(p->get_allowed_cv_qual());

        if (p->get_allow_mix_static_in_struct())
            primary_static_spec = p->get_allow_static_members() ? rand_val_gen->get_rand_value(false, true) : false;

        if (p->get_allow_mix_types_in_struct()) {
            Data::VarClassID member_class = rand_val_gen->get_rand_id(p->get_member_class_prob());
            bool add_substruct = false;
            std::shared_ptr<StructType> substruct_type = nullptr;
            if (member_class == Data::VarClassID::STRUCT && p->get_max_struct_depth() > 0 && nested_struct_types.size() > 0) {
                substruct_type = rand_val_gen->get_rand_elem(nested_struct_types);
                add_substruct = substruct_type->get_nest_depth() + 1 != p->get_max_struct_depth();
            }
            if (add_substruct) {
                primary_type = std::make_shared<StructType>(*substruct_type);
            }
            else {
                GenPolicy::BitFieldID bit_field_dis = rand_val_gen->get_rand_id(p->get_bit_field_prob());
                // In C, bit-field may be declared with a type other than unsigned int or signed int
                // only with "J.5.8 Extended bit-field types"
                if (options->is_c()) {
                    auto search_allowed_bit_filed_type = [] (Probability<IntegerType::IntegerTypeID> prob) {
                        return prob.get_id() == IntegerType::IntegerTypeID::INT ||
                               prob.get_id() == IntegerType::IntegerTypeID::UINT;
                    };
                    auto search_res = std::find_if(p->get_allowed_int_types().begin(),
                                                   p->get_allowed_int_types().end(),
                                                   search_allowed_bit_filed_type);
                    if (search_res == p->get_allowed_int_types().end())
                        bit_field_dis = GenPolicy::BitFieldID::MAX_BIT_FIELD_ID;
                }
                if (bit_field_dis == GenPolicy::BitFieldID::UNNAMED) {
                    struct_type->add_shadow_member(BitField::generate(ctx, true));
                    continue;
                }
                else if (bit_field_dis == GenPolicy::BitFieldID::NAMED) {
                    primary_type = BitField::generate(ctx);
                    primary_static_spec = false; // BitField can't be static member of struct
                }
                else
                    primary_type = IntegerType::generate(ctx);
            }
        }
        primary_type->set_cv_qual(primary_cv_qual);
        primary_type->set_is_static(primary_static_spec);
        struct_type->add_member(primary_type, "member_" + std::to_string(name_handler.get_struct_type_count()) + "_" +
                                              std::to_string(member_count++));
    }
    return struct_type;
}