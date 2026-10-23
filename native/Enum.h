#pragma once

#include "NativeLibrary.h"

struct Enum {
    static constexpr const char* name = "Enum";

    static const SymbolTable symbols;
};

inline const SymbolTable Enum::symbols = [] {
    SymbolTable symbols;
    symbols.set("int", Value("int"));
    symbols.set("string", Value("string"));
    symbols.set("list", Value("list"));
    symbols.set("double", Value("double"));
    return symbols;
}();
