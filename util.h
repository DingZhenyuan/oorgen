#pragma once

#define ERROR(err_message) \
    do { \
        std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ", function " << __func__ << "():\n    " << err_message << std::endl; \
        abort(); \
    } while (false)
    