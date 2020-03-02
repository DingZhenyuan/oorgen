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
