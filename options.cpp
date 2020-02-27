#include <algorithm>

#include "options.h"

using namespace oorgen;

Options* oorgen::options;

// 对象初始化默认参数设置
Options::Options() : standard_id(CXX11), mode_64bit(true),
                     include_valarray(false), include_vector(false), include_array(false) {
    plane_oorgen_version = oorgen_version;
    plane_oorgen_version.erase(std::remove(plane_oorgen_version.begin(), plane_oorgen_version.end(), '.'),
                                plane_oorgen_version.end());
}