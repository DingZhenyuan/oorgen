#include "type.h"
#include "variable.h"
#include "util.h"

using namespace oorgen;

void Struct::allocate_members() {
    std::shared_ptr<StructType> struct_type = std::static_pointer_cast<StructType>(type);
    // TODO: struct member can be not only integer
    for (uint32_t i = 0; i < struct_type->get_member_count(); ++i) {
        std::shared_ptr<StructType::StructMember> cur_member = struct_type->get_member(i);
        if (cur_member->get_type()->is_int_type()) {
            if (struct_type->get_member(i)->get_type()->get_is_static()) {
//                std::cout << struct_type->get_member(i)->get_definition() << std::endl;
//                std::cout << struct_type->get_member(i)->get_data().get() << std::endl;
                members.push_back(struct_type->get_member(i)->get_data());
            }
            else {
                std::shared_ptr<IntegerType> int_mem_type = std::static_pointer_cast<IntegerType> (struct_type->get_member(i)->get_type());
                ScalarVariable new_mem (struct_type->get_member(i)->get_name(), int_mem_type);
                members.push_back(std::make_shared<ScalarVariable>(new_mem));
            }
        }
        else if (cur_member->get_type()->is_struct_type()) {
            if (struct_type->get_member(i)->get_type()->get_is_static()) {
//                std::cout << struct_type->get_member(i)->get_definition() << std::endl;
//                std::cout << struct_type->get_member(i)->get_data().get() << std::endl;
                members.push_back(struct_type->get_member(i)->get_data());
            }
            else {
                Struct new_struct (cur_member->get_name(), std::static_pointer_cast<StructType>(cur_member->get_type()));
                members.push_back(std::make_shared<Struct>(new_struct));
            }
        }
        else {
            ERROR("unsupported type of struct member (Struct)");
        }
    }
}

std::shared_ptr<Data> Struct::get_member (unsigned int num) {
    if (num >= members.size())
        return nullptr;
    else
        return members.at(num);
}

void Struct::dbg_dump () {
    std::cout << "type ";
    type->dbg_dump();
    std::cout << "name: " << name << std::endl;
    std::cout << "cv_qual: " << type->get_cv_qual() << std::endl;
    std::cout << "members ";
    for (auto i : members) {
        i->dbg_dump();
    }
}

std::shared_ptr<Struct> Struct::generate (std::shared_ptr<Context> ctx) {
    //TODO: what about nested structs? StructType::generate need it. Should it take it itself from context?
    NameHandler& name_handler = NameHandler::get_instance();
    std::shared_ptr<Struct> ret = std::make_shared<Struct>(name_handler.get_struct_var_name(), StructType::generate(ctx));
    ret->generate_members_init(ctx);
    return ret;
}

std::shared_ptr<Struct> Struct::generate (std::shared_ptr<Context> ctx, std::shared_ptr<StructType> struct_type) {
    NameHandler& name_handler = NameHandler::get_instance();
    std::shared_ptr<Struct> ret = std::make_shared<Struct>(name_handler.get_struct_var_name(), struct_type);
    ret->generate_members_init(ctx);
    return ret;
}

void Struct::generate_members_init(std::shared_ptr<Context> ctx) {
    for (uint32_t i = 0; i < get_member_count(); ++i) {
        if (get_member(i)->get_type()->is_struct_type()) {
            std::static_pointer_cast<Struct>(get_member(i))->generate_members_init(ctx);
        }
        else if (get_member(i)->get_type()->is_int_type()) {
            std::shared_ptr<IntegerType> member_type = std::static_pointer_cast<IntegerType>(get_member(i)->get_type());
            BuiltinType::ScalarTypedVal init_val = BuiltinType::ScalarTypedVal::generate(ctx, member_type->get_min(), member_type->get_max());
            std::static_pointer_cast<ScalarVariable>(get_member(i))->set_init_value(init_val);
        }
        else {
            ERROR("unsupported type of struct member (Struct)");
        }
    }
}

ScalarVariable::ScalarVariable (std::string _name, std::shared_ptr<IntegerType> _type) : Data (_name, _type, Data::VarClassID::VAR),
                    min(_type->get_int_type_id()), max(_type->get_int_type_id()),
                    init_val(_type->get_int_type_id()), cur_val(_type->get_int_type_id()) {
    min = _type->get_min();
    max = _type->get_max();
    init_val = _type->get_min();
    cur_val = _type->get_min();
    was_changed = false;
}

void ScalarVariable::dbg_dump () {
    std::cout << "type ";
    type->dbg_dump();
    std::cout << "name: " << name << std::endl;
    std::cout << "cv_qual: " << type->get_cv_qual() << std::endl;
    std::cout << "init_value: " << init_val << std::endl;
    std::cout << "was_changed " << was_changed << std::endl;
    std::cout << "cur_value: " << cur_val << std::endl;
    std::cout << "min: " << min << std::endl;
    std::cout << "max: " << max << std::endl;
}

std::shared_ptr<ScalarVariable> ScalarVariable::generate(std::shared_ptr<Context> ctx) {
    NameHandler& name_handler = NameHandler::get_instance();
    std::shared_ptr<ScalarVariable> ret = std::make_shared<ScalarVariable> (name_handler.get_scalar_var_name(),
                                                                            IntegerType::generate(ctx));
    std::shared_ptr<IntegerType> int_type = IntegerType::generate(ctx);
    return ScalarVariable::generate(ctx, int_type);
}

std::shared_ptr<ScalarVariable> ScalarVariable::generate(std::shared_ptr<Context> ctx,
                                                         std::shared_ptr<IntegerType> int_type) {
    NameHandler& name_handler = NameHandler::get_instance();
    std::shared_ptr<ScalarVariable> ret = std::make_shared<ScalarVariable> (name_handler.get_scalar_var_name(),
                                                                            int_type);
    ret->set_init_value(BuiltinType::ScalarTypedVal::generate(ctx, ret->get_type()->get_int_type_id()));
    return ret;
}

Array::Array (std::string _name, std::shared_ptr<ArrayType> _type, std::shared_ptr<Context> ctx) :
              Data(_name, _type, Data::ARRAY) {
    if (!type->is_array_type())
        ERROR("can't create array without ArrayType");
    init_elements(ctx);
}

void Array::init_elements (std::shared_ptr<Context> ctx) {
    std::shared_ptr<ArrayType> array_type = std::static_pointer_cast<ArrayType>(type);
    std::shared_ptr<Type> base_type = array_type->get_base_type();
    ArrayType::Kind kind = array_type->get_kind();
    std::shared_ptr<Data> new_element;

    auto pick_name = [this, &ctx, &kind] (int32_t idx) -> std::string {
        if (ctx != nullptr && ctx.use_count() != 0) {
            ArrayType::ElementSubscript subs_type = rand_val_gen->get_rand_id(ctx->get_gen_policy()->
                                                                                   get_array_elem_subs_prob());
            if ((kind == ArrayType::STD_VEC || kind == ArrayType::STD_ARR) && subs_type == ArrayType::At)
                return name + ".at(" + std::to_string(idx) + ")";
        }
        return name + " [" + std::to_string(idx) + "]";
    };

    auto var_init = [&new_element, &base_type, &ctx, &pick_name] (int32_t idx) {
        std::shared_ptr<IntegerType> base_int_type = std::static_pointer_cast<IntegerType>(base_type);
        if (ctx == nullptr || ctx.use_count() == 0)
            new_element = std::make_shared<ScalarVariable>(pick_name(idx), base_int_type);
        else {
            new_element = ScalarVariable::generate(ctx, base_int_type);
            new_element->set_name(pick_name(idx));
        }
    };

    auto struct_init = [&new_element, &base_type, &ctx, &pick_name] (int32_t idx) {
        std::shared_ptr<StructType> base_struct_type = std::static_pointer_cast<StructType>(base_type);
        if (ctx == nullptr || ctx.use_count() == 0)
            new_element = std::make_shared<Struct>(pick_name(idx), base_struct_type);
        else {
            new_element = Struct::generate(ctx, base_struct_type);
            new_element->set_name(pick_name(idx));
        }
    };

    for (unsigned int i = 0; i < array_type->get_size(); ++i) {
        if (base_type->is_int_type())
            var_init(i);
        else if (base_type->is_struct_type())
            struct_init(i);
        else
            ERROR("bad base TypeID");

        elements.push_back(new_element);
    }
}

std::shared_ptr<Data> Array::get_element (uint64_t idx) {
    return (idx >= elements.size()) ? nullptr : elements.at(idx);
}

void Array::dbg_dump () {
    std::cout << "name: " << name << std::endl;
    std::cout << "array type: " << std::endl;
    type->dbg_dump();
    std::cout << "elements: " << std::endl;
    for (auto const& i : elements)
        i->dbg_dump();
}

std::shared_ptr<Array> Array::generate(std::shared_ptr<Context> ctx) {
    std::shared_ptr<ArrayType> array_type = ArrayType::generate(ctx);
    return generate(ctx, array_type);
}

std::shared_ptr<Array> Array::generate(std::shared_ptr<Context> ctx, std::shared_ptr<ArrayType> array_type) {
    NameHandler& name_handler = NameHandler::get_instance();
    std::shared_ptr<Array> ret = std::make_shared<Array>(name_handler.get_array_var_name(), array_type, ctx);
    return ret;
}

Pointer::Pointer(std::string _name, std::shared_ptr<Data> _pointee) :
                 Data (_name, nullptr, Data::VarClassID::POINTER), pointee(_pointee) {
    type = std::make_shared<PointerType>(pointee->get_type());
}

Pointer::Pointer(std::string _name, std::shared_ptr<PointerType> _type) :
                 Data (_name, _type, Data::VarClassID::POINTER), pointee(nullptr) {
}

void Pointer::set_pointee (std::shared_ptr<Data> _pointee) {
    std::shared_ptr<PointerType> ptr_type = std::static_pointer_cast<PointerType>(type);
    if (ptr_type->get_pointee_type()->get_int_type_id() != _pointee->get_type()->get_int_type_id())
        ERROR("pointer type and pointee type do not match");
    pointee = _pointee;
}

void Pointer::dbg_dump () {
    std::cout << "name: " << name << std::endl;
    std::cout << "pointer type: " << std::endl;
    type->dbg_dump();
    std::cout << "base type: " << std::endl;
    pointee->dbg_dump();
}
