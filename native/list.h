#pragma once

#include "NativeLibrary.h"

#include <cmath>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

// List library: list
// ============================================================

struct ListLibrary {
    static constexpr const char* name = "list";

    // append(list, value)
    static Value append(const std::vector<Value>& args) {
        if (args.size() != 2) {
            fail(ArgumentOverFlowError(
                "append expects exactly 2 arguments"
            ));
            return Value(0);
        }

        if (!args[0].is_list()) {
            fail(TypeError("Expected 1st argument to be a list"));
            return Value(0);
        }

        auto list = std::get<std::shared_ptr<ListValue>>(
            args[0].value
        );

        list->elements.push_back(args[1]);

        return Value(0);
    }

    // conjoin(list1, list2)
    static Value conjoin(const std::vector<Value>& args) {
        if (args.size() != 2) {
            fail(ArgumentOverFlowError(
                "conjoin expects exactly 2 arguments"
            ));
            return Value(0);
        }

        if (!args[0].is_list() || !args[1].is_list()) {
            fail(TypeError("Expected both arguments to be lists"));
            return Value(0);
        }

        auto list1 = std::get<std::shared_ptr<ListValue>>(
            args[0].value
        );

        auto list2 = std::get<std::shared_ptr<ListValue>>(
            args[1].value
        );

        // Copy first so conjoin(list, list) is also safe.
        const auto elementsToAppend = list2->elements;

        list1->elements.insert(
            list1->elements.end(),
            elementsToAppend.begin(),
            elementsToAppend.end()
        );

        return args[0];
    }

    // Manually maintained export list.
    static const NativeFunctionList functions;
    static const SymbolTable symbols;
};

inline const NativeFunctionList ListLibrary::functions = {
    {"append",  &ListLibrary::append},
    {"conjoin", &ListLibrary::conjoin},
};

inline const SymbolTable ListLibrary::symbols = {};

// ============================================================
