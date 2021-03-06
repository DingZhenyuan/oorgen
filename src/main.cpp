#include <cassert>
#include <cstring>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <sstream>

#include "gen_policy.h"
#include "options.h"
#include "program.h"
#include "sym_table.h"
#include "type.h"
#include "variable.h"
#include "util.h"

#ifndef BUILD_DATE
#define BUILD_DATE __DATE__
#endif

#ifndef BUILD_VERSION
#define BUILD_VERSION ""
#endif

using namespace oorgen;

void printVersion () {
    std::cout << "oorgen version " + options->oorgen_version +
                 " (build " << BUILD_VERSION << " on " << BUILD_DATE << ")" << std::endl;
}

extern void self_test();

bool option_starts_with (char* option, const char* test) {
    return !strncmp(option, test, strlen(test));
}

// This function prints out optional error_message, help and exits
// 此函数用来打印输出可选的错误信息、帮助信息和退出信息
void print_usage_and_exit (std::string error_msg = "") {
    int exit_code = 0;
    if (error_msg != "") {
        std::cerr << error_msg << std::endl;
        exit_code = -1;
    }
    std::cout << std::endl;
    printVersion();
    std::cout << "usage: oorgen\n";
    std::cout << "\t-q                        Quiet mode\n";
    std::cout << "\t-v, --version             Print oorgen version\n";
    std::cout << "\t-d, --out-dir=<out-dir>   Output directory\n";
    std::cout << "\t-s, --seed=<seed>         Predefined seed (it is accepted in form of SSS or VV_SSS)\n";
    std::cout << "\t-m, --bit-mode=<32/64>    Generated test's bit mode\n";
    std::cout << "\t--std=<standard>          Generated test's language standard\n";
    auto search_for_default_std = [] (const std::pair<std::string, Options::StandardID> &pair) {
        return pair.second == options->standard_id;
    };
    auto search_res = std::find_if(Options::str_to_standard.begin(), Options::str_to_standard.end(), search_for_default_std);
    assert(search_res != Options::str_to_standard.end() && "Can't match default standard_id and string");
    std::cout << "\t\t\t\t  Default: " << search_res->first << "\n";
    std::string all_standatds = "\t\t\t\t  Possible variants are:";
    for (const auto &iter : Options::str_to_standard)
        all_standatds += " " + iter.first + ",";
    all_standatds.pop_back();
    std::cout << all_standatds << std::endl;
    exit (exit_code);
}

// This function handles command-line options in form of "-short_arg <value>" and performs action(<value>)
bool parse_short_args (int argc, int &argv_iter, char** &argv, std::string short_arg,
                       std::function<void(char*)> action, std::string error_msg) {
    if (!strcmp(argv[argv_iter], short_arg.c_str())) {
        if (++argv_iter == argc)
            print_usage_and_exit(error_msg);
        else {
            action(argv[argv_iter]);
            return true;
        }
    }
    return false;
}

// This function handles command-line options in form of "--long_arg=<value>" and performs action(<value>)
bool parse_long_args (int &argv_iter, char** &argv, std::string long_arg,
                      std::function<void(char*)> action, std::string error_msg) {
    if (option_starts_with(argv[argv_iter], (long_arg + "=").c_str())) {
        size_t option_end = strlen((long_arg + "=").c_str());
        if (strlen(argv[argv_iter]) == option_end)
            print_usage_and_exit(error_msg);
        else {
            action(argv[argv_iter] + strlen((long_arg + "=").c_str()));
            return true;
        }
    }
    return false;
}

bool parse_long_and_short_args (int argc, int &argv_iter, char** &argv, std::string short_arg,
                                std::string long_arg, std::function<void(char*)> action, std::string error_msg) {
    return parse_long_args (      argv_iter, argv, long_arg , action, error_msg) ||
           parse_short_args(argc, argv_iter, argv, short_arg, action, error_msg);
}

// 程序入口
int main (int argc, char* argv[128]) {
    options = new Options;
    uint64_t seed = 0;
    std::string out_dir = "./";
    bool quiet = false;

    // Utility functions. They are necessary for copy-paste reduction. They perform main actions during option parsing.
    // Detects output directory
    // 实用程序功能。 它们对于减少复制粘贴是必需的。 它们在选项解析期间执行主要操作。
    // 检测输出目录
    auto out_dir_action = [&out_dir] (std::string arg) {
        out_dir = arg;
    };

    // 检测预定义的seed
    auto seed_action = [&seed] (std::string arg) {
        size_t *pEnd = nullptr;
        std::stringstream arg_ss(arg);
        std::string segment;
        std::vector<std::string> seed_list;
        while(std::getline(arg_ss, segment, '_'))
            seed_list.push_back(segment);

        if ((seed_list.size() > 1 && seed_list.at(0) != options->plane_oorgen_version) ||
            seed_list.size() > 2)
            ERROR("Incompatible oorgen version in seed: " + arg);

        try {
            seed = std::stoull(seed_list.back(), pEnd, 10);
        }
        catch (std::invalid_argument& e) {
            print_usage_and_exit("Can't recognize seed: " + arg);
        }
    };

    // 检测 oorgen 的 bit_mode
    auto bit_mode_action = [] (std::string arg) {
        size_t *pEnd = nullptr;
        try {
            uint64_t bit_mode_arg = std::stoull(arg, pEnd, 10);
            if (bit_mode_arg == 32)
                options->mode_64bit = false;
            else if (bit_mode_arg == 64)
                options->mode_64bit = true;
            else
                print_usage_and_exit("Can't recognize bit mode: " + std::string(arg));
        }
        catch (std::invalid_argument& e) {
            print_usage_and_exit("Can't recognize bit mode: " + arg);
        }
    };

    // 检测想要的语言标准
    auto standard_action = [] (std::string arg) {
        std::string search_str = arg;
        auto search_res = Options::str_to_standard.find(search_str);
        if (search_res != Options::str_to_standard.end())
            options->standard_id = search_res->second;
        else {
            print_usage_and_exit("Can't recognize language standard: --std=" + search_str + "\n");
        }
    };

    // 解析命令行选项的主循环
    for (int i = 0; i < argc; ++i) {
        if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")) {
            print_usage_and_exit();
        }
        else if (!strcmp(argv[i], "--version") || !strcmp(argv[i], "-v")) {
            printVersion();
            exit(0);
        }
        else if (!strcmp(argv[i], "-q")) {
            quiet = true;
        }
        else if (parse_long_args(i, argv, "--std", standard_action,
                                 "Can't recognize language standard:")) {}
        else if (parse_long_and_short_args(argc, i, argv, "-d", "--out-dir", out_dir_action,
                                           "Output directory wasn't specified.")) {}
        else if (parse_long_and_short_args(argc, i, argv, "-s", "--seed", seed_action,
                                           "Seed wasn't specified.")) {}
        else if (parse_long_and_short_args(argc, i, argv, "-m", "--bit-mode", bit_mode_action,
                                           "Can't recognize bit mode:")) {}
        else if (argv[i][0] == '-') {
            print_usage_and_exit("Unknown option: " + std::string(argv[i]));
        }
    }

    if (argc == 1 && !quiet) {
        std::cerr << "Using default options" << std::endl;
        std::cerr << "For help type " << argv [0] << " -h" << std::endl;
    }

    rand_val_gen = std::make_shared<RandValGen>(RandValGen (seed));
    default_gen_policy.init_from_config();

//    self_test();

    Program mas (out_dir);
    mas.generate ();
    mas.emit_func ();
    mas.emit_decl ();
    mas.emit_main ();

    delete(options);

    return 0;
}
