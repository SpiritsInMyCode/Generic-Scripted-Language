#pragma once

// Add one include and one registerNativeLibrary call for each native/<name>.h.
// The header name is the GSL import name. Library::functions,
// Library::symbols, and Library::sublibs are optional export tables and
// default to empty when omitted.
#include "native/std.h"
#include "native/list.h"
#include "native/random.h"
#include "native/Enum.h"
#include "native/math.h"

#include <type_traits>

namespace native_library_detail {

template <typename Library, typename = void>
struct HasFunctions : std::false_type {};

template <typename Library>
struct HasFunctions<Library, std::void_t<decltype(Library::functions)>>
    : std::true_type {};

template <typename Library, typename = void>
struct HasSymbols : std::false_type {};

template <typename Library>
struct HasSymbols<Library, std::void_t<decltype(Library::symbols)>>
    : std::true_type {};

template <typename Library, typename = void>
struct HasSublibs : std::false_type {};

template <typename Library>
struct HasSublibs<Library, std::void_t<decltype(Library::sublibs)>>
    : std::true_type {};

inline const NativeFunctionList& emptyFunctions() {
    static const NativeFunctionList functions;
    return functions;
}

inline const SymbolTable& emptySymbols() {
    static const SymbolTable symbols;
    return symbols;
}

inline const NativeSublibList& emptySublibs() {
    static const NativeSublibList sublibs;
    return sublibs;
}

template <typename Library>
const NativeFunctionList& functions() {
    if constexpr (HasFunctions<Library>::value) {
        return Library::functions;
    } else {
        return emptyFunctions();
    }
}

template <typename Library>
const SymbolTable& symbols() {
    if constexpr (HasSymbols<Library>::value) {
        return Library::symbols;
    } else {
        return emptySymbols();
    }
}

template <typename Library>
const NativeSublibList& sublibs() {
    if constexpr (HasSublibs<Library>::value) {
        return Library::sublibs;
    } else {
        return emptySublibs();
    }
}

} // namespace native_library_detail

template <typename Library>
inline void registerNativeLibrary(Interpreter& interpreter) {
    NativeLibrary library(Library::name,
                          native_library_detail::functions<Library>(),
                          native_library_detail::symbols<Library>(),
                          native_library_detail::sublibs<Library>());
    interpreter.registerNativeLibrary(library.getName(), [library]() {
        return library.createEntry();
    });
}

inline void registerBuiltins(Interpreter& interpreter) {
    registerNativeLibrary<StdLibrary>(interpreter);
    registerNativeLibrary<ListLibrary>(interpreter);
    registerNativeLibrary<RandomLibrary>(interpreter);
    registerNativeLibrary<Enum>(interpreter);
    registerNativeLibrary<math>(interpreter);

    if (!interpreter.loadNativeLibrary(StdLibrary::name)) {
        fail("Could not load the standard native library.");
        return;
    }

    for (const auto& task : interpreter.libraryRegistry.at("std").first.tasks) {
        interpreter.globalRegistry.define(task.first, task.second);
    }
}
