#pragma once

#include <map>
#include <string>

namespace oorgen {
    
struct Options {
        // 版本
        std::string oorgen_version = "0.0";
        std::string plane_oorgen_version;
        
        // 支持的C++语言的标准的ID
        enum StandardID {
            C99, C11, MAX_CStandardID,
            CXX98, CXX03, CXX11, CXX14, CXX17, MAX_CXXStandardID,
            OpenCL_1_0, OpenCL_1_1, OpenCL_1_2, OpenCL_2_0, OpenCL_2_1, OpenCL_2_2, MAX_OpenCLStandardID
        };

        // map 讲标准的ID对应到字符串文本
        static const std::map<std::string, StandardID> str_to_standard;

        Options();
        
        // 版本判断
        bool is_c();
        bool is_cxx();
        bool is_opencl();

        StandardID standard_id;
        // 是否时64bit形式
        bool mode_64bit;

        // 是否包括以下结构
        bool include_valarray;
        bool include_vector;
        bool include_array;
    };
    
extern Options *options;
}
