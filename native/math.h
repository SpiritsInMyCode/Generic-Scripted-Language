#pragma once

#include "NativeLibrary.h"
#include <cmath>

struct math {

    static constexpr const char* name = "math";

    static Value sin(const std::vector<Value>& args) {
        if (args.size() != 1) {
            fail(ArgumentOverFlowError("Expected only one argument"));
        }
        if (!args[0].is_number()) {
            fail(TypeError("Expected 1st argument to be a number"));
        }

        if (args[0].as_number().is_int()) {
            return std::sinf(std::get<int>(args[0].as_number().value));
        } else {
            return std::sinf(args[0].as_number().as_double());
        }
    }
    static Value cos(const std::vector<Value>& args) {
        if (args.size() != 1) {
            fail(ArgumentOverFlowError("Expected only one argument"));
        }
        if (!args[0].is_number()) {
            fail(TypeError("Expected 1st argument to be a number"));
        }

        if (args[0].as_number().is_int()) {
            return std::cosf(std::get<int>(args[0].as_number().value));
        } else {
            return std::cosf(args[0].as_number().as_double());
        }
    }
    static Value tan(const std::vector<Value>& args) {
        if (args.size() != 1) {
            fail(ArgumentOverFlowError("Expected only one argument"));
        }
        if (!args[0].is_number()) {
            fail(TypeError("Expected 1st argument to be a number"));
        }

        if (args[0].as_number().is_int()) {
            return std::tanf(std::get<int>(args[0].as_number().value));
        } else {
            return std::tanf(args[0].as_number().as_double());
        }
    }

    static const NativeFunctionList functions;
    static const SymbolTable symbols;
    static const NativeSublibList sublibs;
};

inline const NativeFunctionList math::functions = {};

inline const SymbolTable math::symbols = {};

inline const NativeSublibList math::sublibs = [] {
    SymbolTable constants;
    constants.set("PI", Value(3.141592653589793));

    return NativeSublibList{
        NativeSublib("trig", {{"sin", &math::sin}, {"cos", &math::cos}, {"tan", &math::tan}}),
        NativeSublib("constants", {}, std::move(constants))
    };
}();
