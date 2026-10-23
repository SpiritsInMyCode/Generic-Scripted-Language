#pragma once

#include "../Interpreter.h"

#include <string>
#include <utility>
#include <vector>

using NativeFunctionEntry = std::pair<std::string, NativeFunction>;
using NativeFunctionList = std::vector<NativeFunctionEntry>;

class NativeLibrary {
public:
    NativeLibrary(std::string name, const NativeFunctionList& functions,
                  const SymbolTable& symbols)
        : name_(std::move(name)), symbols_(symbols) {
        for (const auto& [functionName, function] : functions) {
            tasks_.registerNative(functionName, function);
        }
    }

    const std::string& getName() const { return name_; }

    LibraryEntry createEntry() const { return LibraryEntry(tasks_, symbols_); }

private:
    std::string name_;
    TasksRegistry tasks_;
    SymbolTable symbols_;
};
