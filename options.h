#pragma once

#include <map>
#include <string>

namespace oorgen {
    struct Options {
        std::string oorgen_version = "0.0";
        std::string plane_oorgen_version;
        
        // 支持的C++语言的标准
        enum StandardID {
            C99, C11
        };
    }
}