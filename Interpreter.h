#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "AST.h"
#include "Parser.h"
#include "lexer.h"
#include "Error.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <optional>
#include <functional>
#include <filesystem>
#include <fstream>

class Number {
public:
    std::variant<int, double> value;

    Number() : value(0) {}
    Number(int v) : value(v) {}
    Number(double v) : value(v) {}

    bool is_int() const noexcept {
        return std::holds_alternative<int>(value);
    }

    double as_double() const noexcept {
        return is_int() ? static_cast<double>(std::get<int>(value))
                        : std::get<double>(value);
    }

    static Number from_wide_integer(long long result) {
        if (result >= static_cast<long long>(std::numeric_limits<int>::min()) &&
            result <= static_cast<long long>(std::numeric_limits<int>::max())) {
            return Number(static_cast<int>(result));
        }
        return Number(static_cast<double>(result));
    }

    Number added_to(const Number& other) const {
        if (is_int() && other.is_int()) {
            const long long result = static_cast<long long>(std::get<int>(value)) +
                                     static_cast<long long>(std::get<int>(other.value));
            return from_wide_integer(result);
        }
        return Number(as_double() + other.as_double());
    }

    Number subbed_by(const Number& other) const {
        if (is_int() && other.is_int()) {
            const long long result = static_cast<long long>(std::get<int>(value)) -
                                     static_cast<long long>(std::get<int>(other.value));
            return from_wide_integer(result);
        }
        return Number(as_double() - other.as_double());
    }

    Number multed_by(const Number& other) const {
        if (is_int() && other.is_int()) {
            const long long result = static_cast<long long>(std::get<int>(value)) *
                                     static_cast<long long>(std::get<int>(other.value));
            return from_wide_integer(result);
        }
        return Number(as_double() * other.as_double());
    }

    Number moduled_by(const Number& other) const {
        if (other.as_double() == 0.0) {
            fail("Operator '%': division by zero.");
        }

        if (is_int() && other.is_int()) {
            const long long numerator = static_cast<long long>(std::get<int>(value));
            const long long denominator = static_cast<long long>(std::get<int>(other.value));
            return from_wide_integer(numerator % denominator);
        }
        return Number(std::fmod(as_double(), other.as_double()));
    }

    Number dived_by(const Number& other) const {
        if (other.as_double() == 0.0) {
            fail("Operator '/': division by zero.");
        }

        // Preserve the original interpreter's integer-division behavior when
        // both operands are integers. Use floating-point division otherwise.
        if (is_int() && other.is_int()) {
            const long long numerator = static_cast<long long>(std::get<int>(value));
            const long long denominator = static_cast<long long>(std::get<int>(other.value));
            // Widen before dividing to avoid the INT_MIN / -1 overflow case.
            return from_wide_integer(numerator / denominator);
        }
        return Number(as_double() / other.as_double());
    }

    Number powed_by(const Number& other) const {
        const double result = std::pow(as_double(), other.as_double());
        if (!std::isfinite(result)) {
            // Infinity and NaN are valid floating-point results for power; they
            // should not be cast to int.
            return Number(result);
        }

        if (is_int() && other.is_int() && std::get<int>(other.value) >= 0 &&
            result >= static_cast<double>(std::numeric_limits<int>::min()) &&
            result <= static_cast<double>(std::numeric_limits<int>::max()) &&
            std::trunc(result) == result) {
            return Number(static_cast<int>(result));
        }
        return Number(result);
    }

    Number negated() const {
        if (is_int()) {
            const int n = std::get<int>(value);
            if (n != std::numeric_limits<int>::min()) {
                return Number(-n);
            }
            return Number(-static_cast<double>(n));
        }
        return Number(-std::get<double>(value));
    }

    friend Number operator+(const Number& lhs, const Number& rhs) {
        return lhs.added_to(rhs);
    }
    friend Number operator-(const Number& lhs, const Number& rhs) {
        return lhs.subbed_by(rhs);
    }
    friend Number operator*(const Number& lhs, const Number& rhs) {
        return lhs.multed_by(rhs);
    }
    friend Number operator/(const Number& lhs, const Number& rhs) {
        return lhs.dived_by(rhs);
    }
    friend Number operator&(const Number& lhs, const Number& rhs) {
        return Number(lhs.as_double() != 0.0 && rhs.as_double() != 0.0);
    }

    friend bool operator==(const Number& lhs, const Number& rhs) {
        return lhs.as_double() == rhs.as_double();
    }
    friend bool operator!=(const Number& lhs, const Number& rhs) {
        return !(lhs == rhs);
    }
    friend bool operator<(const Number& lhs, const Number& rhs) {
        return lhs.as_double() < rhs.as_double();
    }
    friend bool operator<=(const Number& lhs, const Number& rhs) {
        return lhs.as_double() <= rhs.as_double();
    }
    friend bool operator>(const Number& lhs, const Number& rhs) {
        return lhs.as_double() > rhs.as_double();
    }
    friend bool operator>=(const Number& lhs, const Number& rhs) {
        return lhs.as_double() >= rhs.as_double();
    }
};

inline std::ostream& operator<<(std::ostream& os, const Number& number) {
    if (number.is_int()) {
        os << std::get<int>(number.value);
    } else {
        os << std::get<double>(number.value);
    }
    return os;
}

struct ListValue;

class Value {
public:
    std::variant<Number, std::string, std::shared_ptr<ListValue>> value;

    Value() : value(Number(0)) {}
    Value(int v) : value(Number(v)) {}
    Value(double v) : value(Number(v)) {}
    Value(const Number& v) : value(v) {}
    Value(const std::string& v) : value(v) {}
    Value(const char* v) : value(std::string(v == nullptr ? "" : v)) {}
    explicit Value(std::vector<Value> values);

    bool is_string() const noexcept {
        return std::holds_alternative<std::string>(value);
    }

    bool is_number() const noexcept {
        return std::holds_alternative<Number>(value);
    }

    bool is_list() const noexcept {
        return std::holds_alternative<std::shared_ptr<ListValue>>(value);
    }

    bool is_truthy() const {
        if (is_number()) {
            return std::get<Number>(value).as_double() != 0.0;
        }
        if (is_string()) {
            return !std::get<std::string>(value).empty();
        }
        return !as_list().empty();
    }

    const Number& as_number() const {
        if (!is_number()) {
            fail("Expected a number.");
        }
        return std::get<Number>(value);
    }

    const std::string& as_string() const {
        if (!is_string()) {
            fail("Expected a string.");
        }
        return std::get<std::string>(value);
    }

    const std::vector<Value>& as_list() const;

    Value operator+(const Value& other) const {
        if (is_number() && other.is_number()) {
            return Value(as_number() + other.as_number());
        }
        if (is_string() && other.is_string()) {
            return Value(as_string() + other.as_string());
        }
        fail(
            "Operator '+': operands must both be numbers or both be strings.");
    }

    Value operator-(const Value& other) const {
        if (is_number() && other.is_number()) {
            return Value(as_number() - other.as_number());
        } 
        fail("Operator '-': operands must both be numbers.");
    }

    Value operator%(const Value& other) const {
        if (is_number() && other.is_number()) {
            return Value(as_number().moduled_by(other.as_number()));
        }
        fail("Operator '%': operands must both be numbers.");
    }

    Value operator*(const Value& other) const {
        if (is_number() && other.is_number()) {
            return Value(as_number() * other.as_number());
        }
        fail("Operator '*': operands must both be numbers.");
    }

    Value operator/(const Value& other) const {
        if (is_number() && other.is_number()) {
            return Value(as_number() / other.as_number());
        }
        fail("Operator '/': operands must both be numbers.");
    }

    Value powed_by(const Value& other) const {
        if (is_number() && other.is_number()) {
            return Value(as_number().powed_by(other.as_number()));
        }
        fail("Operator '^': operands must both be numbers.");
    }

    Value negated() const {
        if (is_number()) {
            return Value(as_number().negated());
        }
        fail("Unary '-': operand must be a number.");
    }

    bool operator==(const Value& other) const {
        if (is_number() && other.is_number()) {
            return as_number() == other.as_number();
        }
        if (is_string() && other.is_string()) {
            return as_string() == other.as_string();
        }
        if (is_list() && other.is_list()) {
            return as_list() == other.as_list();
        }
        fail("Operator '==': operands must have the same type.");
    }

    bool operator!=(const Value& other) const {
        return !(*this == other);
    }

    bool operator<(const Value& other) const {
        if (is_number() && other.is_number()) {
            return as_number() < other.as_number();
        }
        if (is_string() && other.is_string()) {
            return as_string() < other.as_string();
        }
        fail("Operator '<': operands must have the same type.");
    }

    bool operator<=(const Value& other) const {
        if (is_number() && other.is_number()) {
            return as_number() <= other.as_number();
        }
        if (is_string() && other.is_string()) {
            return as_string() <= other.as_string();
        }
        fail("Operator '<=': operands must have the same type.");
    }

    bool operator>(const Value& other) const {
        if (is_number() && other.is_number()) {
            return as_number() > other.as_number();
        }
        if (is_string() && other.is_string()) {
            return as_string() > other.as_string();
        }
        fail("Operator '>': operands must have the same type.");
    }

    bool operator>=(const Value& other) const {
        if (is_number() && other.is_number()) {
            return as_number() >= other.as_number();
        }
        if (is_string() && other.is_string()) {
            return as_string() >= other.as_string();
        }
        fail("Operator '>=': operands must have the same type.");
    }
};

struct ListValue {
    std::vector<Value> elements;
};

inline Value::Value(std::vector<Value> values)
    : value(std::make_shared<ListValue>(ListValue{std::move(values)})) {}

inline const std::vector<Value>& Value::as_list() const {
    if (!is_list()) {
        fail("Expected a list.");
    }
    return std::get<std::shared_ptr<ListValue>>(value)->elements;
}

using NativeFunction = std::function<Value(const std::vector<Value>&)>;

inline std::ostream& operator<<(std::ostream& os, const Value& value) {
    if (value.is_number()) {
        os << std::get<Number>(value.value);
    } else if (value.is_string()) {
        os << std::get<std::string>(value.value);
    } else {
        os << '[';
        const auto& elements = value.as_list();
        for (std::size_t i = 0; i < elements.size(); ++i) {
            if (i != 0) {
                os << ", ";
            }
            os << elements[i];
        }
        os << ']';
    }
    return os;
}

class Task {
    public:
        std::shared_ptr<TaskDeclareNode> declaration;

        NativeFunction native;

        bool is_native() const noexcept {
            return static_cast<bool>(native);
        }

        explicit Task(TaskDeclareNode node)
            : declaration(std::make_shared<TaskDeclareNode>(std::move(node))) {}
        explicit Task(NativeFunction func) : native(std::move(func)) {}
        Task(Task&&) noexcept = default;
        Task& operator=(Task&&) noexcept = default;
        Task(const Task&) = default;
        Task& operator=(const Task&) = default;
};

class TasksRegistry {
    public:
        TasksRegistry* parent = nullptr;

        std::unordered_map<std::string, Task> tasks;

        void define(const std::string& name, Task task) {
            tasks.insert_or_assign(name, std::move(task));
        }
        Task* get(const std::string& name) {
            auto it = tasks.find(name);

            if (it != tasks.end()) {
                return &it->second;
            }
            if (parent != nullptr) {
                return parent->get(name);
            }
            return nullptr;
        }
        void registerNative(std::string name, NativeFunction func) {
            if (!func) {
                fail("Cannot register an empty native function: " + name);
            }
            tasks.insert_or_assign(std::move(name), Task(std::move(func)));
        }
};

class SymbolTable {
public:
    SymbolTable* parent = nullptr;
    std::unordered_map<std::string, Value> symbols;

    void set(const std::string& name, const Value& value) {
        symbols.insert_or_assign(name, value);
    }

    bool contains_local(const std::string& name) const {
        return symbols.find(name) != symbols.end();
    }

    Value get(const std::string& name) const {
        const auto it = symbols.find(name);
        if (it != symbols.end()) {
            return it->second;
        }
        if (parent != nullptr) {
            return parent->get(name);
        }
        fail("Undefined variable: " + name);
    }
};

using LibraryEntry = std::pair<TasksRegistry, SymbolTable>;
using NativeLibraryFactory = std::function<LibraryEntry()>;

class Interpreter {
    struct ReturnSignal {
        Value value;
    };

public:

    SymbolTable globalSymbolTable;
    SymbolTable *currentSymbolTable = &globalSymbolTable;

    TasksRegistry globalRegistry;
    TasksRegistry *currentFunctionRegistry = &globalRegistry;
    std::unordered_map<std::string, LibraryEntry> libraryRegistry;
    std::unordered_map<std::string, NativeLibraryFactory> nativeLibraryFactories;
    std::filesystem::path handlerSourceDirectory;
    std::filesystem::path mainSourceDirectory;
    std::filesystem::path currentSourceDirectory;



    double numericValue(const Value& value) const {
        return value.as_number().as_double();
    }

    void registerNativeLibrary(
        std::string name,
        NativeLibraryFactory factory
    ) {
        if (name.empty()) {
            fail("Cannot register a native library without a name.");
        }
        if (!factory) {
            fail("Cannot register an empty native library factory: " + name);
        }
        nativeLibraryFactories.insert_or_assign(std::move(name), std::move(factory));
    }

    bool loadNativeLibrary(const std::string& name) {
        const auto factory = nativeLibraryFactories.find(name);
        if (factory == nativeLibraryFactories.end()) {
            return false;
        }
        if (libraryRegistry.find(name) != libraryRegistry.end()) {
            return true;
        }

        LibraryEntry library = factory->second();
        library.first.parent = &globalRegistry;
        library.second.parent = &globalSymbolTable;
        libraryRegistry.emplace(name, std::move(library));
        return true;
    }

    void enter_scope(std::unordered_map<std::string, Value> init_variables = {}) {
        auto* table = new SymbolTable();
        auto* registry = new TasksRegistry();

        table->symbols.insert(init_variables.begin(), init_variables.end());

        table->parent = currentSymbolTable;
        table->symbols.insert_or_assign("void", Value(0));

        registry->parent = currentFunctionRegistry;

        currentSymbolTable = table;
        currentFunctionRegistry = registry;
    }

    void exit_scope() {
        if (currentSymbolTable == &globalSymbolTable) {
            fail("Cannot exit the global scope.");
        }
        if (currentFunctionRegistry == &globalRegistry) {
            fail("Cannot exit global scope.");
        }

        SymbolTable* oldScope = currentSymbolTable;
        currentSymbolTable = currentSymbolTable->parent;
        TasksRegistry* oldtScope = currentFunctionRegistry;
        currentFunctionRegistry = currentFunctionRegistry->parent;

        delete oldScope;
        delete oldtScope;
    }

    Number toNumber(const Value& value) const {
        return value.as_number();
    }

    Value visit_NumberNode(const NumberNode& node) const {
        try {
            if (node.token.type == TokenType::INT) {
                return Value(Number(std::stoi(node.token.value)));
            }
            if (node.token.type == TokenType::FLOAT) {
                return Value(Number(std::stod(node.token.value)));
            }
        } catch (const std::exception& e) {
            fail("Invalid numeric literal '" + node.token.value +
                 "': " + e.what());
        }
        fail("Unknown number token type: " + node.token.value);
    }

    Value visit_StringNode(const StringNode& node) const {
        return Value(node.value.value);
    }

    Value visit_ListNode(const ListNode& node) {
        std::vector<Value> elements;
        elements.reserve(node.elements.size());
        for (const auto& element : node.elements) {
            elements.push_back(visit(element));
        }
        return Value(std::move(elements));
    }

    Value visit_IndexNode(const IndexNode& node) {
        const Value collection = visit(node.collection);
        if (!collection.is_list()) {
            fail("List indexing requires a list.");
        }
        const Value indexValue = visit(node.index);
        if (!indexValue.is_number()) {
            fail("List index must be an integer.");
        }

        const double numericIndex = indexValue.as_number().as_double();
        if (!std::isfinite(numericIndex) ||
            std::trunc(numericIndex) != numericIndex ||
            numericIndex < 0.0) {
            fail("List index must be a non-negative integer.");
        }

        const auto& elements = collection.as_list();
        if (numericIndex >= static_cast<double>(elements.size())) {
            fail("List index out of range.");
        }
        return elements[static_cast<std::size_t>(numericIndex)];
    }

    void visit_OutNode(const OutNode& node) {
        const Value result = visit(node.value);
        std::cout << result << '\n';
    }

    Value visit_VarAssignNode(const VarAssignNode& node) {
        const Value assigned = visit(node.value);
        currentSymbolTable->set(node.token.value, assigned);
        return assigned;
    }

    Value visit_VarAccessNode(const VarAccessNode& node) const {
        const std::size_t separator = node.token.value.find('.');
        if (separator != std::string::npos) {
            if (node.token.value.find('.', separator + 1) != std::string::npos) {
                fail("Library access must use the form 'library.member'.");
            }
            const std::string libraryName = node.token.value.substr(0, separator);
            const std::string memberName = node.token.value.substr(separator + 1);
            const auto library = libraryRegistry.find(libraryName);
            if (library == libraryRegistry.end()) {
                fail("Unknown library: " + libraryName);
            }
            if (library->second.first.tasks.find(memberName) !=
                library->second.first.tasks.end()) {
                fail("Library member '" + node.token.value +
                     "' is a task; execute it with 'exec'.");
            }
            return library->second.second.get(memberName);
        }
        return currentSymbolTable->get(node.token.value);
    }

    Value visit_ScanNode() const {
        std::string input;
        if (!std::getline(std::cin, input)) {
            fail("Input failed while reading a line.");
        }
        return Value(input);
    }
    
    void visit_TaskDeclareNode(const TaskDeclareNode& node) {
        currentFunctionRegistry->define(node.ident, Task(node));
    }

    std::filesystem::path resolveImportPath(const std::string& filename) const {
        const std::filesystem::path requested(filename);
        if (requested.is_absolute()) {
            if (std::filesystem::is_regular_file(requested)) {
                return requested;
            }
            fail("Could not find imported file: " + requested.string());
        }

        const std::filesystem::path importFilePath =
            handlerSourceDirectory / "import" / requested;
        if (!handlerSourceDirectory.empty() &&
            std::filesystem::is_regular_file(importFilePath)) {
            return importFilePath;
        }

        const std::filesystem::path mainSourceFile =
            mainSourceDirectory / requested;
        if (!mainSourceDirectory.empty() &&
            std::filesystem::is_regular_file(mainSourceFile)) {
            return mainSourceFile;
        }

        // Keep imports relative to an imported module working after checking
        // the two stable search locations above.
        const std::filesystem::path moduleSourceFile =
            currentSourceDirectory / requested;
        if (!currentSourceDirectory.empty() &&
            currentSourceDirectory != mainSourceDirectory &&
            std::filesystem::is_regular_file(moduleSourceFile)) {
            return moduleSourceFile;
        }

        fail("Could not find imported file '" + filename +
             "' in '" + (handlerSourceDirectory / "import").string() +
             "', '" + mainSourceDirectory.string() +
             "' or '" + currentSourceDirectory.string() + "'.");
    }

    void visit_ImportNode(const ImportNode& node) {
        if (libraryRegistry.find(node.filename) != libraryRegistry.end() ||
            loadNativeLibrary(node.filename)) {
            return;
        }

        const std::filesystem::path importPath = resolveImportPath(node.filename);
        const std::string libraryName = importPath.stem().string();
        if (libraryName.empty()) {
            fail("Imported file must have a library name: " + node.filename);
        }
        if (libraryRegistry.find(libraryName) != libraryRegistry.end()) {
            return;
        }

        std::ifstream file(importPath);
        if (!file.is_open()) {
            fail("Could not open imported file: " + importPath.string());
        }
        const std::string source(
            (std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>());

        Lexer lexer(source);
        Parser parser(lexer.tokenize());
        std::unique_ptr<ASTNode> module = parser.parse();
        auto* statements = dynamic_cast<StatementsNode*>(module.get());
        if (statements == nullptr) {
            fail("Imported file did not produce a valid module.");
        }

        auto inserted = libraryRegistry.try_emplace(libraryName);
        LibraryEntry& library = inserted.first->second;
        library.first.parent = &globalRegistry;
        library.second.parent = &globalSymbolTable;

        SymbolTable* previousSymbols = currentSymbolTable;
        TasksRegistry* previousTasks = currentFunctionRegistry;
        const std::filesystem::path previousSourceDirectory =
            currentSourceDirectory;
        currentSymbolTable = &library.second;
        currentFunctionRegistry = &library.first;
        currentSourceDirectory = importPath.parent_path();
        try {
            for (const auto& statement : statements->statements) {
                visit(statement);
            }
        } catch (...) {
            currentSymbolTable = previousSymbols;
            currentFunctionRegistry = previousTasks;
            currentSourceDirectory = previousSourceDirectory;
            throw;
        }
        currentSymbolTable = previousSymbols;
        currentFunctionRegistry = previousTasks;
        currentSourceDirectory = previousSourceDirectory;
    }

    void visit_UnpackNode(const UnpackNode& node) {
        const auto library = libraryRegistry.find(node.library);
        if (library == libraryRegistry.end()) {
            fail("Cannot unpack unknown library: " + node.library);
        }
        for (const auto& task : library->second.first.tasks) {
            currentFunctionRegistry->define(task.first, task.second);
        }
        for (const auto& symbol : library->second.second.symbols) {
            currentSymbolTable->set(symbol.first, symbol.second);
        }
    }

    std::optional<Value> visit_Task(const Task& task, const std::vector<Value>& arguments = {}) {
        if (task.is_native()) {
            return task.native(arguments);
        }
        if (!task.declaration) {
            fail("Cannot execute an uninitialized task.");
        }

        const auto& parameterOrder = task.declaration->argumentNames;
        if (arguments.size() != parameterOrder.size()) {
            fail("Task '" + task.declaration->ident + "' expected " +
                 std::to_string(parameterOrder.size()) + " argument(s), got " +
                 std::to_string(arguments.size()) + ".");
        }

        std::unordered_map<std::string, Value> boundArguments;
        for (std::size_t i = 0; i < parameterOrder.size(); ++i) {
            boundArguments.insert_or_assign(parameterOrder[i], arguments[i]);
        }

        enter_scope(boundArguments);
        try {
            for (const auto& statement : task.declaration->statements->statements) {
                visit(statement);
            }
        } catch (const ReturnSignal& signal) {
            exit_scope();
            return signal.value;
        } catch (...) {
            exit_scope();
            throw;
        }
        exit_scope();
        return std::nullopt;
    }
    std::optional<Value> visit_ExecNode(const ExecNode& node) {
        const std::size_t separator = node.ident.find('.');
        const bool qualified = separator != std::string::npos;
        Task* task = nullptr;
        TasksRegistry* taskRegistry = currentFunctionRegistry;
        SymbolTable* symbolTable = currentSymbolTable;
        if (qualified) {
            if (node.ident.find('.', separator + 1) != std::string::npos) {
                fail("Task access must use the form 'library.task'.");
            }
            const std::string libraryName = node.ident.substr(0, separator);
            const std::string taskName = node.ident.substr(separator + 1);
            const auto library = libraryRegistry.find(libraryName);
            if (library == libraryRegistry.end()) {
                fail("Unknown library: " + libraryName);
            }
            const auto taskEntry = library->second.first.tasks.find(taskName);
            if (taskEntry != library->second.first.tasks.end()) {
                task = &taskEntry->second;
            }
            taskRegistry = &library->second.first;
            symbolTable = &library->second.second;
        } else {
            task = currentFunctionRegistry->get(node.ident);
        }
        if (task == nullptr) {
            fail(UndefinedValueError("Cannot find task '" + node.ident + "'"));
        }

        std::vector<Value> argumentValues;
        argumentValues.reserve(node.arguments.size());
        for (const auto& argument : node.arguments) {
            argumentValues.push_back(visit(argument));
        }

        TasksRegistry* previousTasks = currentFunctionRegistry;
        SymbolTable* previousSymbols = currentSymbolTable;
        currentFunctionRegistry = taskRegistry;
        currentSymbolTable = symbolTable;
        try {
            std::optional<Value> result = visit_Task(*task, argumentValues);
            currentFunctionRegistry = previousTasks;
            currentSymbolTable = previousSymbols;
            return result;
        } catch (...) {
            currentFunctionRegistry = previousTasks;
            currentSymbolTable = previousSymbols;
            throw;
        }
    }

    Value visit_StatementsNode(const StatementsNode& node) {
        Value result(0);
        enter_scope();
        try {
            for (const auto& statement : node.statements) {
                result = visit(statement);
            }
        } catch (...) {
            exit_scope();
            throw;
        }
        exit_scope();
        return result;
    }

    Value visit_IfNode(const IfNode& node) {
        const Value condition = visit(node.condition);
        if (condition.is_truthy()) {
            if (node.body) {
                return visit_StatementsNode(*node.body);
            }
        } else if (node.otherwise) {
            return visit_StatementsNode(*node.otherwise);
        }
        return Value(0);
    }

    Value visit_LoopNode(const LoopNode& node) {
        const Value iterable = visit(node.iterable);

        auto executeIteration = [this, &node](const Value& bindingValue) {
            enter_scope({{node.binding, bindingValue}});
            try {
                for (const auto& statement : node.body->statements) {
                    visit(statement);
                }
            } catch (...) {
                exit_scope();
                throw;
            }
            exit_scope();
        };

        if (iterable.is_list()) {
            for (const auto& element : iterable.as_list()) {
                executeIteration(element);
            }
            return Value(0);
        }

        if (iterable.is_string()) {
            for (const char character : iterable.as_string()) {
                executeIteration(Value(std::string(1, character)));
            }
            return Value(0);
        }

        if (iterable.is_number()) {
            const double count = iterable.as_number().as_double();
            if (!std::isfinite(count) ||
                std::trunc(count) != count ||
                count < 0.0 ||
                count > static_cast<double>(std::numeric_limits<int>::max())) {
                fail("A numeric loop expression must be a non-negative integer within the supported range.");
            }

            for (int index = 0; index < static_cast<int>(count); ++index) {
                executeIteration(Value(index));
            }
            return Value(0);
        }

        fail("A loop expression must be a list, string, or non-negative integer.");
    }

    Value visit_WhileNode(const WhileNode& node) {
        enter_scope();
        try {
            while (visit(node.condition).is_truthy()) {
                for (const auto& statement : node.body->statements) {
                    visit(statement);
                }
            }
        } catch (...) {
            exit_scope();
            throw;
        }
        exit_scope();
        return Value(0);
    }

    Value visit_UnaryOpNode(const UnaryOpNode& node) {
        const Value operand = visit(node.operand);

        // The language uses @ for logical NOT. It works with numbers and strings:
        // zero and the empty string are false; other values are true.
        if (node.op.type == TokenType::KEYWRD && node.op.value == "@") {
            return Value(operand.is_truthy() ? 0 : 1);
        }

        if (node.op.type == TokenType::MINUS || node.op.value == "-") {
            return operand.negated();
        }
        if (node.op.type == TokenType::UNARYOP && node.op.value == "+") {
            (void)operand.as_number();
            return operand;
        }

        fail("Unknown unary operator: " + node.op.value);
    }

    [[noreturn]] void visit_returnNode(const ReturnNode& node) {
        throw ReturnSignal{visit(node.returnVal)};
    }

    Value visit_BinOpNode(const BinOpNode& node) {
        const Value left = visit(node.left);

        // Evaluate logical operators with short-circuiting, so the right side is
        // not evaluated when its value cannot change the result.
        if (node.op.type == TokenType::KEYWRD && node.op.value == "&") {
            if (!left.is_truthy()) {
                return Value(0);
            }
            return Value(visit(node.right).is_truthy() ? 1 : 0);
        }
        if (node.op.type == TokenType::KEYWRD && node.op.value == "|") {
            if (left.is_truthy()) {
                return Value(1);
            }
            return Value(visit(node.right).is_truthy() ? 1 : 0);
        }

        const Value right = visit(node.right);
        if (node.op.type == TokenType::PLUS) {
            return left + right;
        }
        if (node.op.type == TokenType::MINUS) {
            return left - right;
        }
        if (node.op.type == TokenType::MUL) {
            return left * right;
        }
        if (node.op.type == TokenType::DIV) {
            return left / right;
        }
        if (node.op.type == TokenType::MOD) {
            return left % right;
        }
        if (node.op.type == TokenType::POW) {
            return left.powed_by(right);
        }
        if (node.op.type == TokenType::EE) {
            return Value(left == right ? 1 : 0);
        }
        if (node.op.type == TokenType::NE) {
            return Value(left != right ? 1 : 0);
        }
        if (node.op.type == TokenType::GT) {
            return Value(left > right ? 1 : 0);
        }
        if (node.op.type == TokenType::GTE) {
            return Value(left >= right ? 1 : 0);
        }
        if (node.op.type == TokenType::LT) {
            return Value(left < right ? 1 : 0);
        }
        if (node.op.type == TokenType::LTE) {
            return Value(left <= right ? 1 : 0);
        }

        fail("Unknown binary operator: " + node.op.value);
    }



    Value visit(const std::unique_ptr<ASTNode>& node) {
        if (!node) {
            fail("Cannot interpret a null AST node.");
        }

        if (const auto* statementsNode = dynamic_cast<const StatementsNode*>(node.get())) {
            return visit_StatementsNode(*statementsNode);
        }
        if (const auto* ifNode = dynamic_cast<const IfNode*>(node.get())) {
            return visit_IfNode(*ifNode);
        }
        if (const auto* loopNode = dynamic_cast<const LoopNode*>(node.get())) {
            return visit_LoopNode(*loopNode);
        }
        if (const auto* whileNode = dynamic_cast<const WhileNode*>(node.get())) {
            return visit_WhileNode(*whileNode);
        }
        if (const auto* outNode = dynamic_cast<const OutNode*>(node.get())) {
            visit_OutNode(*outNode);
            return Value(0);
        }
        if (const auto* numberNode = dynamic_cast<const NumberNode*>(node.get())) {
            return visit_NumberNode(*numberNode);
        }
        if (const auto* binOpNode = dynamic_cast<const BinOpNode*>(node.get())) {
            return visit_BinOpNode(*binOpNode);
        }
        if (const auto* unaryOpNode = dynamic_cast<const UnaryOpNode*>(node.get())) {
            return visit_UnaryOpNode(*unaryOpNode);
        }
        if (const auto* varAssignNode = dynamic_cast<const VarAssignNode*>(node.get())) {
            return visit_VarAssignNode(*varAssignNode);
        }
        if (const auto* varAccessNode = dynamic_cast<const VarAccessNode*>(node.get())) {
            return visit_VarAccessNode(*varAccessNode);
        }
        if (const auto* stringNode = dynamic_cast<const StringNode*>(node.get())) {
            return visit_StringNode(*stringNode);
        }
        if (const auto* listNode = dynamic_cast<const ListNode*>(node.get())) {
            return visit_ListNode(*listNode);
        }
        if (const auto* indexNode = dynamic_cast<const IndexNode*>(node.get())) {
            return visit_IndexNode(*indexNode);
        }
        if (dynamic_cast<const ScanNode*>(node.get()) != nullptr) {
            return visit_ScanNode();
        }
        if (const auto* taskDeclareNode = dynamic_cast<const TaskDeclareNode*>(node.get())) {
            visit_TaskDeclareNode(*taskDeclareNode);
            return Value(0);
        }
        if (const auto* execNode = dynamic_cast<const ExecNode*>(node.get())) {
            std::optional<Value> value = visit_ExecNode(*execNode);

            if (value.has_value()) {
                return value.value();
            } else {
                return Value(0);
            }
        }
        if (const auto* importNode = dynamic_cast<const ImportNode*>(node.get())) {
            visit_ImportNode(*importNode);
            return Value(0);
        }
        if (const auto* unpackNode = dynamic_cast<const UnpackNode*>(node.get())) {
            visit_UnpackNode(*unpackNode);
            return Value(0);
        }
        if (const auto* returnNode = dynamic_cast<const ReturnNode*>(node.get())) {
            visit_returnNode(*returnNode);
        }

        fail("Unknown AST node type.");
    }
};

#endif // INTERPRETER_H
