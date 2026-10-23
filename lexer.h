#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <optional>
#include <vector>
#include <iostream>
#include <cctype>
#include <unordered_set>

#include "AST.h"
#include "Error.h"

std::unordered_set<std::string> keywords = {
    "decl",
    "out",
    "&",
    "|",
    "@",
    "if",
    "loop",
    "while",
    "bind",
    "do",
    "otherwise",
    "scan",
    "task",
    "sublib",
    "init",
    "exec",
    "create",
    "yield",
    "using",
    "import",
    "unpack",
};


class Lexer {
public:
    std::string text;
    size_t pos = static_cast<size_t>(-1);
    std::optional<char> current_char;

    size_t col = 0;
    size_t ln = 1;

    std::vector<size_t> indentStack{0};
    bool atLineStart = true;

    static constexpr size_t TAB_WIDTH = 4;

    Lexer(const std::string& text) : text(text) {
        advance();
    }

    void advance() {
        pos++;

        if (pos < text.size()) {
            if (text[pos] == '\n') {
                ln++;
                col = 0;
            } else {
                col++;
            }

            current_char = text[pos];
        } else {
            current_char = std::nullopt;
        }
    }

    char peekNext() {
        if (pos + 1 < text.size()) {
            return text[pos + 1];
        }

        return '\0';
    }

    Token makeNumber() {
        std::string num_str;
        int dot_count = 0;

        size_t start_col = col;
        size_t start_ln = ln;

        while (current_char.has_value() &&
               (std::isdigit(static_cast<unsigned char>(current_char.value())) ||
                current_char.value() == '.')) {

            if (current_char.value() == '.') {
                dot_count++;

                if (dot_count > 1) {
                    DecimalOverflowError error(
                        "Too many decimal points in number at column " +
                        std::to_string(col) +
                        ", line " + std::to_string(ln)
                    );

                    fail(error);
                }
            }

            num_str += current_char.value();
            advance();
        }

        if (dot_count == 0) {
            return Token(TokenType::INT, num_str, start_ln, start_col);
        }

        return Token(TokenType::FLOAT, num_str, start_ln, start_col);
    }

    Token makeIdent() {
        std::string id_str;

        size_t start_col = col;
        size_t start_ln = ln;

        while (current_char.has_value() &&
               (std::isalnum(static_cast<unsigned char>(current_char.value())) ||
                current_char.value() == '_')) {

            id_str += current_char.value();
            advance();
        }

        TokenType type =
            keywords.find(id_str) != keywords.end()
                ? TokenType::KEYWRD
                : TokenType::IDENT;

        return Token(type, id_str, start_ln, start_col);
    }

    void processIndentation(TokenList& tokens) {
        size_t indentation = 0;

        // Count leading spaces and tabs.
        while (current_char.has_value() &&
            (current_char.value() == ' ' ||
                current_char.value() == '\t')) {

            if (current_char.value() == ' ') {
                indentation++;
            } else {
                // Expand tabs to the next multiple of TAB_WIDTH.
                indentation += TAB_WIDTH - (indentation % TAB_WIDTH);
            }

            advance();
        }

        // Blank and comment-only lines do not change indentation.
        if (!current_char.has_value() ||
            current_char.value() == '\n' ||
            (current_char.value() == '!' && peekNext() == '-')) {
            return;
        }

        size_t currentIndent = indentStack.back();

        // Enter a deeper indentation level.
        if (indentation > currentIndent) {
            indentStack.push_back(indentation);

            tokens.emplace_back(
                TokenType::INDENT, "INDENT", ln, col
            );
        }

        // Return to a previous indentation level.
        else if (indentation < currentIndent) {
            while (indentStack.size() > 1 &&
                indentation < indentStack.back()) {

                indentStack.pop_back();

                tokens.emplace_back(
                    TokenType::DEDENT, "DEDENT", ln, col
                );
            }

            // The indentation must match a previous level exactly.
            if (indentation != indentStack.back()) {
                fail(
                    "Inconsistent indentation at line " +
                    std::to_string(ln)
                );
            }
        }
    }

    TokenList tokenize() {
        TokenList tokens;

        while (current_char.has_value()) {
            char ch = current_char.value();

            if (atLineStart) {
                processIndentation(tokens);

                if (!current_char.has_value()) {
                    break;
                }

                atLineStart = false;
                ch = current_char.value();
            }

            if (std::isspace(static_cast<unsigned char>(ch)) &&
                ch != '\n') {
                advance();
            }

            else if (std::isdigit(static_cast<unsigned char>(ch))) {
                tokens.emplace_back(makeNumber());
            }

            else if (std::isalpha(static_cast<unsigned char>(ch)) ||
                     ch == '_') {
                tokens.emplace_back(makeIdent());
            }

            else if (ch == ',') {
                tokens.emplace_back(TokenType::SEPARATOR, ",", ln, col);
                advance();
            }

            else if (ch == '.') {
                tokens.emplace_back(TokenType::DOT, ".", ln, col);
                advance();
            }

            // Addition
            else if (ch == '+') {
                tokens.emplace_back(TokenType::PLUS, "+", ln, col);
                advance();
            }

            // Subtraction or unary minus
            else if (ch == '-') {
                if (tokens.empty() ||
                    tokens.back().type == TokenType::LPAREN ||
                    tokens.back().type == TokenType::PLUS ||
                    tokens.back().type == TokenType::MINUS ||
                    tokens.back().type == TokenType::MUL ||
                    tokens.back().type == TokenType::DIV ||
                    tokens.back().type == TokenType::MOD ||
                    tokens.back().type == TokenType::POW ||
                    tokens.back().type == TokenType::EQ ||
                    tokens.back().type == TokenType::EE ||
                    tokens.back().type == TokenType::GTE ||
                    tokens.back().type == TokenType::GT ||
                    tokens.back().type == TokenType::LTE ||
                    tokens.back().type == TokenType::LT ||
                    tokens.back().type == TokenType::NE) {

                    tokens.emplace_back(TokenType::UNARYOP, "-", ln, col);
                } else {
                    tokens.emplace_back(TokenType::MINUS, "-", ln, col);
                }

                advance();
            }

            // Multiplication or exponentiation
            else if (ch == '*') {
                if (peekNext() == '*') {
                    tokens.emplace_back(TokenType::POW, "**", ln, col);
                    advance();
                    advance();
                } else {
                    tokens.emplace_back(TokenType::MUL, "*", ln, col);
                    advance();
                }
            }

            // Division
            else if (ch == '/') {
                tokens.emplace_back(TokenType::DIV, "/", ln, col);
                advance();
            }

            else if (ch == '%') {
                tokens.emplace_back(TokenType::MOD, "%", ln, col);
                advance();
            }

            else if (ch == '\\') {
                tokens.emplace_back(TokenType::INDENT, "\\", ln, col);
                advance();
            }

            else if (ch == ';') {
                tokens.emplace_back(TokenType::NEWLINE, ";", ln, col);
                advance();
            }

            // A comment runs to the end of the current line.
            else if (ch == '!' && peekNext() == '-') {
                while (current_char.has_value() &&
                       current_char.value() != '\n') {
                    advance();
                }
            }

            // Parentheses
            else if (ch == '(') {
                tokens.emplace_back(TokenType::LPAREN, "(", ln, col);
                advance();
            }

            else if (ch == '[') {
                tokens.emplace_back(TokenType::SQRLBRCKT, "[", ln, col);
                advance();
            }

            else if (ch == ']') {
                tokens.emplace_back(TokenType::SQRRBRCKT, "]", ln, col);
                advance();
            }

            else if (ch == ')') {
                tokens.emplace_back(TokenType::RPAREN, ")", ln, col);
                advance();
            } else if (ch == '"' || ch == '\'') {
                // Save the location of the opening quotation mark.
                size_t start_ln = ln;
                size_t start_col = col;
                const char quote = ch;

                advance(); // Consume the opening quotation mark

                std::string string_val;

                while (current_char.has_value() &&
                    current_char.value() != quote) {

                    char string_ch = current_char.value();

                    if (string_ch == '\n' || string_ch == ';') {
                        BrokenPairError error("Unterminated string");
                        fail(error);
                    }

                    if (string_ch == '\\' && peekNext() == quote) {
                        advance();
                        string_ch = current_char.value();
                    }
                    string_val += string_ch;
                    advance();
                }

                // If we reached EOF before finding a closing quotation mark,
                // report an unterminated string.
                if (!current_char.has_value()) {
                    BrokenPairError error("Unterminated string");
                    fail(error);
                }

                advance(); // Consume the closing quotation mark

                tokens.emplace_back(
                    TokenType::STRING,
                    string_val,
                    start_ln,
                    start_col
                );
            }

            // Greater than or greater than or equal
            else if (ch == '>') {
                if (peekNext() == '=') {
                    tokens.emplace_back(TokenType::GTE, ">=", ln, col);
                    advance();
                    advance();
                } else {
                    tokens.emplace_back(TokenType::GT, ">", ln, col);
                    advance();
                }
            }

            // Less than or less than or equal
            else if (ch == '<') {
                if (peekNext() == '=') {
                    tokens.emplace_back(TokenType::LTE, "<=", ln, col);
                    advance();
                    advance();
                } else {
                    tokens.emplace_back(TokenType::LT, "<", ln, col);
                    advance();
                }
            }

            // Assignment or equality comparison
            else if (ch == '=') {
                if (peekNext() == '=') {
                    tokens.emplace_back(TokenType::EE, "==", ln, col);
                    advance();
                    advance();
                } else {
                    tokens.emplace_back(TokenType::EQ, "=", ln, col);
                    advance();
                }
            }

            // Not equal
            else if (ch == '!') {
                if (peekNext() == '=') {
                    tokens.emplace_back(TokenType::NE, "!=", ln, col);
                    advance();
                    advance();
                } else {
                    IllegalCharError error(
                        std::string("Invalid character: '! at column ") +
                        std::to_string(col) +
                        ", line " + std::to_string(ln)
                    );

                    fail(error);
                }
            }

            // Newline
            else if (ch == '\n') {
                tokens.emplace_back(TokenType::NEWLINE, "NEWLINE", ln, col);
                advance();
                atLineStart = true;
            }

            // Keyword operators
            else if (ch == '&' || ch == '|' || ch == '@') {
                std::string op(1, ch);
                tokens.emplace_back(TokenType::KEYWRD, op, ln, col);
                advance();
            }

            // Invalid character
            else {
                IllegalCharError error(
                    std::string("Invalid character: '") +
                    ch +
                    "' at column " + std::to_string(col) +
                    ", line " + std::to_string(ln)
                );

                fail(error);
            }
        }

        return tokens;
    }
};

inline std::string tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::INT:     return "INT";
        case TokenType::FLOAT:   return "FLOAT";
        case TokenType::PLUS:    return "PLUS";
        case TokenType::MINUS:   return "MINUS";
        case TokenType::MUL:     return "MUL";
        case TokenType::DIV:     return "DIV";
        case TokenType::MOD:     return "MOD";
        case TokenType::LPAREN:  return "LPAREN";
        case TokenType::RPAREN:  return "RPAREN";
        case TokenType::SQRLBRCKT: return "SQRLBRCKT";
        case TokenType::SQRRBRCKT: return "SQRRBRCKT";
        case TokenType::KEYWRD:  return "KEYWRD";
        case TokenType::IDENT:   return "IDENT";
        case TokenType::EQ:      return "EQ";
        case TokenType::EE:      return "EE";
        case TokenType::GT:      return "GT";
        case TokenType::GTE:     return "GTE";
        case TokenType::LT:      return "LT";
        case TokenType::LTE:     return "LTE";
        case TokenType::NE:      return "NE";
        case TokenType::POW:     return "POW";
        case TokenType::UNARYOP: return "UNARYOP";
        case TokenType::NEWLINE: return "NEWLINE";
        case TokenType::INDENT:  return "INDENT";
        case TokenType::DEDENT:  return "DEDENT";
        case TokenType::STRING:  return "STRING";
        case TokenType::SEPARATOR: return "SEPARATOR";
        case TokenType::DOT:     return "DOT";
        default:                return "UNKNOWN";
    }
}

#endif // LEXER_H
