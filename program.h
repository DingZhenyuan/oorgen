#pragma once

#include <fstream>

#include "gen_policy.h"
#include "sym_table.h"
#include "stmt.h"

///////////////////////////////////////////////////////////////////////////////

namespace oorgen {

// This class serves as a driver to generation process.
// First of all, you should initialize global variable rand_val_gen with seed (see RandValGen).
// After that you can call generate method (it will do all the work).
// To print-out result, just call consecutively all emit methods.
//
// Generation process starts with initialization of global Context and extern SymTable.
// After it, recursive Scope generation method starts
class Program {
    public:
        Program (std::string _out_folder);

        // It initializes global Context and launches generation process.
        void generate ();

        // Print-out methods
        // To get valid test, all of them should be called (the order doesn't matter)
        void emit_func ();
        void emit_decl ();
        void emit_main ();

    private:

        void form_extern_sym_table(std::shared_ptr<Context> ctx);

        GenPolicy gen_policy;
        std::vector<std::shared_ptr<ScopeStmt>> functions;
        // There are three kind of global variables which exist in test.
        // All of them are declared as external variables for core test function
        // to prevent constant propagation optimization.
        // Also all they are initialized at startup of the test to prevent UB.
        // Main difference between them is what happens after:
        // 1) Input variables - they can't change their value (it is necessary for CSE)
        // 2) Mixed variables - they can change their value multiple times.
        //    They are used in checksum calculation.
        // 3) Output variables - they can change their value only once.
        //    They are also used in checksum calculation.
        std::vector<std::shared_ptr<SymbolTable>> extern_inp_sym_table;
        std::vector<std::shared_ptr<SymbolTable>> extern_mix_sym_table;
        std::vector<std::shared_ptr<SymbolTable>> extern_out_sym_table;
        std::string out_folder;
};
}

