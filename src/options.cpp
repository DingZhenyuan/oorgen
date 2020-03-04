#include <algorithm>

#include "options.h"

using namespace oorgen;

// options指针定义
Options* oorgen::options;

// 对象初始化默认参数设置
Options::Options() : standard_id(CXX11), mode_64bit(true),
                     include_valarray(false), include_vector(false), include_array(false) {
    plane_oorgen_version = oorgen_version;
    plane_oorgen_version.erase(std::remove(plane_oorgen_version.begin(), plane_oorgen_version.end(), '.'),
                                plane_oorgen_version.end());
}

// 将StandardID和其字符串形式匹配
const std::map<std::string, Options::StandardID> Options::str_to_standard = {
    {"c99", C99},
    {"c11", C11},

    {"c++98", CXX98},
    {"c++03", CXX03},
    {"c++11", CXX11},
    {"c++14", CXX14},
    {"c++17", CXX17},

    {"opencl_1_0", OpenCL_1_0},
    {"opencl_1_1", OpenCL_1_1},
    {"opencl_1_2", OpenCL_1_2},
    {"opencl_2_0", OpenCL_2_0},
    {"opencl_2_1", OpenCL_2_1},
    {"opencl_2_2", OpenCL_2_2},
};

// 判断
bool Options::is_c() {
    return C99 <= standard_id && standard_id < MAX_CStandardID;
}

bool Options::is_cxx() {
    return CXX98 <= standard_id && standard_id < MAX_CXXStandardID;
}

bool Options::is_opencl() {
    return OpenCL_1_0 <= standard_id && standard_id < MAX_OpenCLStandardID;
}
