#ifndef ERROR_H
#define ERROR_H

#include <string>
#include <stdexcept>    

struct Error {
    std::string error_name;
    std::string details;
};

[[noreturn]] inline void fail(const std::string& message) {
    throw std::runtime_error(message);
}

[[noreturn]] inline void fail(const Error& error) {
    fail(error.error_name + ": " + error.details);
}


struct IllegalCharError : public Error {
    IllegalCharError(const std::string& details) {
        error_name = "Illegal Character";
        this->details = details;
    }
};

struct DecimalOverflowError : public Error {
    DecimalOverflowError(const std::string& details) {
        error_name = "Decimal Overflow";
        this->details = details;
    }
};

struct InvalidSyntaxError : public Error {
    InvalidSyntaxError(const std::string& details) {
        error_name = "Invalid Syntax";
        this->details = details;
    }
};

struct MissingOperandError : public Error {
    MissingOperandError(const std::string& details) {
        error_name = "Missing Operand";
        this->details = details;
    }
};

struct BrokenPairError : public Error {
    BrokenPairError(const std::string& details) {
        error_name = "Broken Pair";
        this->details = details;
    }
};
struct UndefinedValueError : public Error {
    UndefinedValueError(const std::string& details) {
        error_name = "Undefined Value";
        this->details = details;
    }
};

struct ArgumentOverFlowError : public Error {
    ArgumentOverFlowError(const std::string& details) {
        error_name = "Argument Overflow";
        this->details = details;
    }
};

struct TypeError : public Error {
    TypeError(const std::string& details) {
        error_name = "Type error";
        this->details = details;
    }
};

#endif