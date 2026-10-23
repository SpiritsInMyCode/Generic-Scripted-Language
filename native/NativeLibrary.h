#pragma once

#include "../Interpreter.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

using NativeFunctionEntry = std::pair<std::string, NativeFunction>;
using NativeFunctionList = std::vector<NativeFunctionEntry>;

class NativeSublib;
using NativeSublibList = std::vector<NativeSublib>;

class NativeSublib {
public:
    NativeSublib(
        std::string name,
        NativeFunctionList functions = {},
        SymbolTable symbols = {},
        NativeSublibList sublibs = {},
        std::vector<std::string> initFields = {}
    )
        : name_(std::move(name)),
          functions_(std::move(functions)),
          symbols_(std::move(symbols)),
          sublibs_(std::move(sublibs)),
          initFields_(std::move(initFields)) {}

    const std::string& getName() const { return name_; }

    std::shared_ptr<SublibDefinition> createDefinition() const {
        if (name_.empty()) {
            fail("Cannot register a native sublib without a name.");
        }

        auto definition = std::make_shared<SublibDefinition>();
        definition->name = name_;
        definition->initFields = initFields_;
        definition->symbols = symbols_;
        definition->symbols.parent = nullptr;

        for (const auto& [functionName, function] : functions_) {
            definition->tasks.registerNative(functionName, function);
        }

        for (const NativeSublib& sublib : sublibs_) {
            auto child = sublib.createDefinition();
            if (!definition->sublibs->definitions.emplace(
                    child->name, std::move(child)).second) {
                fail("Native sublib is already defined: " + sublib.getName());
            }
        }

        return definition;
    }

private:
    std::string name_;
    NativeFunctionList functions_;
    SymbolTable symbols_;
    NativeSublibList sublibs_;
    std::vector<std::string> initFields_;
};

class NativeLibrary {
public:
    NativeLibrary(std::string name, const NativeFunctionList& functions,
                  const SymbolTable& symbols,
                  const NativeSublibList& sublibs = {})
        : name_(std::move(name)), symbols_(symbols), sublibs_(sublibs) {
        for (const auto& [functionName, function] : functions) {
            tasks_.registerNative(functionName, function);
        }
    }

    const std::string& getName() const { return name_; }

    LibraryEntry createEntry() const {
        LibraryEntry entry(tasks_, symbols_);
        for (const NativeSublib& sublib : sublibs_) {
            auto definition = sublib.createDefinition();
            if (!entry.sublibs->definitions.emplace(
                    definition->name, std::move(definition)).second) {
                fail("Native sublib is already defined: " + sublib.getName());
            }
        }
        return entry;
    }

private:
    std::string name_;
    TasksRegistry tasks_;
    SymbolTable symbols_;
    NativeSublibList sublibs_;
};
