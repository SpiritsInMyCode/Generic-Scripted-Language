#pragma once

#include "NativeLibrary.h"

#include <cmath>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

// Standard library: std
// ============================================================

struct StdLibrary {
    static constexpr const char* name = "std";

    // abs(number)
    static Value abs(const std::vector<Value>& args) {
        if (args.size() != 1) {
            fail("abs expects one argument");
            return Value(0);
        }

        return Value(
            std::abs(args[0].as_number().as_double())
        );
    }

    // len(string | list)
    static Value len(const std::vector<Value>& args) {
        if (args.size() != 1) {
            fail("len expects one argument");
            return Value(0);
        }

        if (args[0].is_list()) {
            return Value(
                static_cast<double>(args[0].as_list().size())
            );
        }

        if (!args[0].is_string()) {
            fail("len expects a string or list");
            return Value(0);
        }

        return Value(
            static_cast<double>(args[0].as_string().size())
        );
    }

    // toNumber(number | string)
    static Value conv(const std::vector<Value>& args) {
       if (args.size() != 2) {
            fail(ArgumentOverFlowError("Expected 2 arguments"));
       }
       if (args[1].as_string() == "int" || args[1].as_string() == "string" || args[1].as_string() == "double" || args[1].as_string() == "list") {
            if (args[1].as_string() == "int") {
                if (args[0].is_string()) {
                    return std::stod(args[0].as_string());
                }
            }
       } else {
            fail(TypeError("Expected second argument to be any one data type in string form"));
       }
    }

    // Manually maintained export list.
    // Every exported function must appear here.
    static const NativeFunctionList functions;
    static const SymbolTable symbols;
};

inline const NativeFunctionList StdLibrary::functions = {
    {"abs",      &StdLibrary::abs},
    {"len",      &StdLibrary::len},
    {"conv",     &StdLibrary::conv},
};

inline const SymbolTable StdLibrary::symbols = [] {
    SymbolTable symbols;
    symbols.set("PI", Value(3.14159265358979323846));
    return symbols;
}();

// ============================================================
