#pragma once

#include "Interpreter.h"

#include <cmath>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

// ============================================================
// Shared native function definitions
// ============================================================

using NativeFunctionEntry = std::pair<std::string, NativeFunction>;
using NativeFunctionList = std::vector<NativeFunctionEntry>;

// ============================================================
// Generic native library wrapper
// ============================================================

class NativeLibrary {
public:
    NativeLibrary(
        std::string name,
        const NativeFunctionList& functions
    )
        : name_(std::move(name)) {

        for (const auto& [functionName, function] : functions) {
            tasks_.registerNative(functionName, function);
        }
    }

    const std::string& getName() const {
        return name_;
    }

    LibraryEntry createEntry() const {
        return LibraryEntry(tasks_, SymbolTable());
    }

private:
    std::string name_;
    TasksRegistry tasks_;
};

// ============================================================
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
    static Value toNumber(const std::vector<Value>& args) {
        if (args.size() != 1) {
            fail("toNumber expects one argument");
            return Value(0);
        }

        if (args[0].is_number()) {
            return args[0];
        }

        if (!args[0].is_string()) {
            fail("toNumber expects a number or string");
            return Value(0);
        }

        const std::string& input = args[0].as_string();

        std::size_t parsed = 0;
        double number = 0.0;

        try {
            number = std::stod(input, &parsed);
        }
        catch (const std::invalid_argument&) {
            fail("toNumber could not convert '" + input + "' to a number");
            return Value(0);
        }
        catch (const std::out_of_range&) {
            fail("toNumber value is out of range: '" + input + "'");
            return Value(0);
        }

        if (parsed != input.size()) {
            fail("toNumber could not convert '" + input + "' to a number");
            return Value(0);
        }

        return Value(number);
    }

    // Manually maintained export list.
    // Every exported function must appear here.
    static const NativeFunctionList functions;
};

inline const NativeFunctionList StdLibrary::functions = {
    {"abs",      &StdLibrary::abs},
    {"len",      &StdLibrary::len},
    {"toNumber", &StdLibrary::toNumber},
};

// ============================================================
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
};

inline const NativeFunctionList ListLibrary::functions = {
    {"append",  &ListLibrary::append},
    {"conjoin", &ListLibrary::conjoin},
};

// ============================================================
// Random library: random
// ============================================================

struct RandomLibrary {
    static constexpr const char* name = "random";

    // randomInt(min, max)
    // Both integer bounds are inclusive.
    // If either argument is floating-point, returns a real value.
    static Value random(const std::vector<Value>& args) {
        if (args.size() != 2) {
            fail("randomInt expects exactly 2 arguments");
            return Value(0);
        }

        if (!args[0].is_number() || !args[1].is_number()) {
            fail("randomInt expects two numeric arguments");
            return Value(0);
        }

        static thread_local std::mt19937 generator{
            std::random_device{}()
        };

        const auto& first = args[0].as_number();
        const auto& second = args[1].as_number();

        if (first.is_int() && second.is_int()) {
            const int minimum = std::get<int>(first.value);
            const int maximum = std::get<int>(second.value);

            if (minimum > maximum) {
                fail("randomInt minimum cannot exceed maximum");
                return Value(0);
            }

            std::uniform_int_distribution<int> distribution(
                minimum,
                maximum
            );

            return Value(distribution(generator));
        }

        const double minimum = first.as_double();
        const double maximum = second.as_double();

        if (minimum > maximum) {
            fail("randomInt minimum cannot exceed maximum");
            return Value(0);
        }

        std::uniform_real_distribution<double> distribution(
            minimum,
            maximum
        );

        return Value(distribution(generator));
    }

    // Manually maintained export list.
    static const NativeFunctionList functions;
};

inline const NativeFunctionList RandomLibrary::functions = {
    {"random", &RandomLibrary::random},
};

// ============================================================
// Generic registration
// ============================================================

// Registers any library struct that provides:
//   Library::name
//   Library::functions
template <typename Library>
inline void registerNativeLibrary(Interpreter& interpreter) {
    NativeLibrary library(
        Library::name,
        Library::functions
    );

    interpreter.registerNativeLibrary(
        library.getName(),
        [library]() {
            return library.createEntry();
        }
    );
}

// ============================================================
// Global built-in registration
// ============================================================

inline void registerBuiltins(Interpreter& interpreter) {
    registerNativeLibrary<StdLibrary>(interpreter);
    registerNativeLibrary<ListLibrary>(interpreter);
    registerNativeLibrary<RandomLibrary>(interpreter);

    if (!interpreter.loadNativeLibrary(StdLibrary::name)) {
        fail("Could not load the standard native library.");
        return;
    }

    for (const auto& task :
         interpreter.libraryRegistry.at("std").first.tasks) {
        interpreter.globalRegistry.define(task.first, task.second);
    }
}