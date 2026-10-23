#pragma once

#include "NativeLibrary.h"

#include <cmath>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

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
    static const SymbolTable symbols;
};

inline const NativeFunctionList RandomLibrary::functions = {
    {"random", &RandomLibrary::random},
};

inline const SymbolTable RandomLibrary::symbols = {};

// ============================================================
