
#ifndef PARSER_H
#define PARSER_H

#include "AST.h"

#include <string>
#include <utility>
#include "iostream"
#include "lexer.h"
#include "Error.h"  
#include <unordered_set>
#include <vector>

class Parser {
public:
    explicit Parser(TokenList tokens)
        : tokens_(std::move(tokens)) {}

    // Parse the complete token list into an AST.
    std::unique_ptr<ASTNode> parse() {
        auto root = std::make_unique<StatementsNode>();

        while (peek() != nullptr) {
            skipNewlines();
            if (peek() == nullptr) {
                break;
            }

            auto statement = parseExpression();
            const bool isBlockStatement =
                dynamic_cast<IfNode*>(statement.get()) != nullptr ||
                dynamic_cast<LoopNode*>(statement.get()) != nullptr ||
                dynamic_cast<WhileNode*>(statement.get()) != nullptr ||
                dynamic_cast<TaskDeclareNode*>(statement.get()) != nullptr;
            root->statements.push_back(std::move(statement));

            if (peek() != nullptr &&
                peek()->type != TokenType::NEWLINE &&
                !isBlockStatement) {
                failAtCurrentToken(InvalidSyntaxError("Unexpected token '" + peek()->value + "'"));
            }
        }

        return root;
    }

private:
    TokenList tokens_;
    std::size_t pos_ = 0;
    // Return the current token without consuming it.
    const Token* peek() const {
        if (pos_ < tokens_.size()) {
            return &tokens_[pos_];
        }

        return nullptr;
    }

    void skipNewlines() {
        while (peek() != nullptr && peek()->type == TokenType::NEWLINE) {
            advance();
        }
    }

    const Token* lookBack() const {
        if (pos_ > 0 && pos_ <= tokens_.size()) {
            return &tokens_[pos_ - 1];
        }

        return nullptr;
    }

    const Token* lookAhead() const {
        if (pos_ + 1 < tokens_.size()) {
            return &tokens_[pos_ + 1];
        }

        return nullptr;
    }

    // Consume and return the current token.
    Token advance() {
        if (peek() == nullptr) {
            failAtCurrentToken(InvalidSyntaxError("Unexpected end of input"));
        }

        return tokens_[pos_++];
    }

    // Consume the current token only if its type matches.
    bool match(TokenType type) {
        const Token* token = peek();

        if (token != nullptr && token->type == type) {
            ++pos_;
            return true;
        }

        return false;
    }

    [[noreturn]] void failAtCurrentToken(const Error& error) const {
        const Token* token = peek();

        if (token != nullptr) {
            ::fail(Error{
                error.error_name,
                error.details + " at line " + std::to_string(token->ln) +
                    ", column " + std::to_string(token->col)
            });
        }

        ::fail(Error{error.error_name, error.details + " at end of input"});
    }

    bool canStartExecArgument() const {
        const Token* token = peek();
        if (token == nullptr) {
            return false;
        }
        if (token->type == TokenType::INT ||
            token->type == TokenType::FLOAT ||
            token->type == TokenType::IDENT ||
            token->type == TokenType::STRING ||
            token->type == TokenType::LPAREN ||
            token->type == TokenType::SQRLBRCKT ||
            token->type == TokenType::MINUS ||
            token->type == TokenType::UNARYOP) {
            return true;
        }
        return token->type == TokenType::KEYWRD &&
            (token->value == "exec" ||
             token->value == "scan" ||
             token->value == "@");
    }

    std::unique_ptr<ASTNode> parseExecNode() {
        advance();
        if (peek() == nullptr || peek()->type != TokenType::IDENT) {
            failAtCurrentToken(InvalidSyntaxError(
                "Expected an identifier after 'exec' keyword"));
        }

        std::string taskIdent = advance().value;
        while (match(TokenType::DOT)) {
            if (peek() == nullptr || peek()->type != TokenType::IDENT) {
                failAtCurrentToken(InvalidSyntaxError(
                    "Expected an identifier after '.'"));
            }
            taskIdent += "." + advance().value;
        }
        std::vector<std::unique_ptr<ASTNode>> arguments;
        while (canStartExecArgument()) {
            arguments.push_back(parseCompExpression());

            if (!match(TokenType::SEPARATOR)) {
                break;
            }
            if (!canStartExecArgument()) {
                failAtCurrentToken(InvalidSyntaxError(
                    "Expected argument after separator"));
            }
        }

        return std::make_unique<ExecNode>(taskIdent, std::move(arguments));
    }

    std::unique_ptr<ASTNode> parseCompExpression() {
        auto node = parseCompAndExpression();

        while (peek() != nullptr &&
               peek()->type == TokenType::KEYWRD &&
               peek()->value == "|") {
            Token op = advance();
            auto right = parseCompAndExpression();
            node = std::make_unique<BinOpNode>(
                std::move(node),
                std::move(op),
                std::move(right)
            );
        }

        return node;
    }

    std::unique_ptr<ASTNode> parseCompAndExpression() {
        auto node = parseCompNotExpression();

        while (peek() != nullptr &&
               peek()->type == TokenType::KEYWRD &&
               peek()->value == "&") {
            Token op = advance();
            auto right = parseCompNotExpression();
            node = std::make_unique<BinOpNode>(
                std::move(node),
                std::move(op),
                std::move(right)
            );
        }

        return node;
    }

    std::unique_ptr<ASTNode> parseCompNotExpression() {
        if (peek() != nullptr &&
            peek()->type == TokenType::KEYWRD &&
            peek()->value == "@") {
            Token op = advance();
            return std::make_unique<UnaryOpNode>(
                std::move(op),
                parseCompNotExpression()
            );
        }

        return parseComparisonExpression();
    }

    std::unique_ptr<ASTNode> parseComparisonExpression() {
        auto node = parseArithmeticExpression();

        while (peek() != nullptr &&
               (peek()->type == TokenType::EE ||
                peek()->type == TokenType::GT ||
                peek()->type == TokenType::GTE ||
                peek()->type == TokenType::LT ||
                peek()->type == TokenType::LTE ||
                peek()->type == TokenType::NE)) {
            Token op = advance();
            auto right = parseArithmeticExpression();
            node = std::make_unique<BinOpNode>(
                std::move(node),
                std::move(op),
                std::move(right)
            );
        }

        return node;
    }

    std::unique_ptr<StatementsNode> parseIndentedStatements(
        const std::string& keyword) {
        if (peek() == nullptr || peek()->type != TokenType::NEWLINE) {
            failAtCurrentToken(InvalidSyntaxError(
                "Expected a new line after '" + keyword + "' keyword"));
        }
        advance();
        skipNewlines();

        if (peek() == nullptr || peek()->type != TokenType::INDENT) {
            failAtCurrentToken(InvalidSyntaxError(
                "Expected indentation after '" + keyword + "' keyword"));
        }
        advance();

        std::vector<std::unique_ptr<ASTNode>> statements;
        while (peek() != nullptr && peek()->type != TokenType::DEDENT) {
            if (match(TokenType::NEWLINE)) {
                continue;
            }

            statements.push_back(parseExpression());
            match(TokenType::NEWLINE);
        }
        match(TokenType::DEDENT);

        return std::make_unique<StatementsNode>(std::move(statements));
    }

    std::unique_ptr<ASTNode> parseExpression() {
        if (peek() != nullptr && peek()->type == TokenType::KEYWRD) {
            if (peek()->value == "import") {
                advance();
                if (peek() == nullptr || peek()->type != TokenType::STRING) {
                    failAtCurrentToken(InvalidSyntaxError(
                        "Expected a quoted filename after 'import'"));
                }
                return std::make_unique<ImportNode>(advance().value);
            } else if (peek()->value == "unpack") {
                advance();
                if (peek() == nullptr || peek()->type != TokenType::IDENT) {
                    failAtCurrentToken(InvalidSyntaxError(
                        "Expected a library name after 'unpack'"));
                }
                return std::make_unique<UnpackNode>(advance().value);
            } else if (peek()->value == "decl") {

                if (lookBack() != nullptr &&
                    lookBack()->type != TokenType::NEWLINE &&
                    lookBack()->type != TokenType::DEDENT &&
                    lookBack()->type != TokenType::INDENT) {
                    failAtCurrentToken(InvalidSyntaxError("Unexpected variable declaration after " + tokenTypeToString(lookBack()->type) + " '" + lookBack()->value + "'"));
                }

                advance(); // Consume 'decl'

                if (!match(TokenType::IDENT)) {
                    if (peek() == nullptr || peek()->type == TokenType::NEWLINE) {
                        failAtCurrentToken(InvalidSyntaxError("Expected identifier after 'decl'"));
                    } else {
                        failAtCurrentToken(InvalidSyntaxError("Expected identifier after 'decl', got '" +
                            peek()->value + "'"));
                    }
                }

                // The identifier has already been consumed by match().
                // Retrieve it from the previous token.
                std::string name = tokens_[pos_ - 1].value;

                if (!match(TokenType::EQ)) {
                    if (peek() == nullptr || peek()->type == TokenType::NEWLINE) {
                        failAtCurrentToken(InvalidSyntaxError("Expected '=' after identifier '" + name + "'"));
                    } else  {
                        failAtCurrentToken(InvalidSyntaxError("Expected '=' after identifier '" + name +
                            "', got '" + peek()->value + "'"));
                    }
                }

                auto value = parseExpression();

                return std::make_unique<VarAssignNode>(
                    Token(TokenType::IDENT, name,
                        tokens_[pos_ - 1].ln, tokens_[pos_ - 1].col),
                    std::move(value)
                );
            } else if (peek()->value == "out") {
                advance();
                return std::make_unique<OutNode>(parseExpression());
            } else if (peek()->value == "@") {
                return parseCompExpression();
            } else if (peek()->value == "if") {
                if (lookBack() != nullptr &&
                    lookBack()->type != TokenType::NEWLINE &&
                    lookBack()->type != TokenType::DEDENT &&
                    lookBack()->type != TokenType::INDENT) {
                    failAtCurrentToken(InvalidSyntaxError("Unexpected 'if' after " + tokenTypeToString(lookBack()->type) + " '" + lookBack()->value + "'"));
                }

                advance();
                auto condition = parseExpression();

                if (peek() == nullptr ||
                    peek()->type != TokenType::KEYWRD ||
                    peek()->value != "do") {
                    failAtCurrentToken(InvalidSyntaxError("Expected 'do' keyword after 'if' condition"));
                }
                advance();

                auto body = parseIndentedStatements("do");

                std::unique_ptr<StatementsNode> otherwise;
                if (peek() != nullptr &&
                    peek()->type == TokenType::KEYWRD &&
                    peek()->value == "otherwise") {
                    advance();
                    otherwise = parseIndentedStatements("otherwise");
                }

                return std::make_unique<IfNode>(
                    std::move(condition),
                    std::move(body),
                    std::move(otherwise)
                );

            } else if (peek()->value == "loop") {
                if (lookBack() != nullptr &&
                    lookBack()->type != TokenType::NEWLINE &&
                    lookBack()->type != TokenType::DEDENT &&
                    lookBack()->type != TokenType::INDENT) {
                    failAtCurrentToken(InvalidSyntaxError(
                        "Unexpected 'loop' after " +
                        tokenTypeToString(lookBack()->type) + " '" +
                        lookBack()->value + "'"));
                }

                advance();
                auto iterable = parseExpression();

                if (peek() == nullptr ||
                    peek()->type != TokenType::KEYWRD ||
                    peek()->value != "bind") {
                    failAtCurrentToken(InvalidSyntaxError(
                        "Expected 'bind' keyword after loop expression"));
                }
                advance();

                if (peek() == nullptr || peek()->type != TokenType::IDENT) {
                    failAtCurrentToken(InvalidSyntaxError(
                        "Expected an identifier after 'bind'"));
                }
                const std::string binding = advance().value;
                auto body = parseIndentedStatements("loop");

                return std::make_unique<LoopNode>(
                    std::move(iterable),
                    binding,
                    std::move(body)
                );

            } else if (peek()->value == "while") {
                if (lookBack() != nullptr &&
                    lookBack()->type != TokenType::NEWLINE &&
                    lookBack()->type != TokenType::DEDENT &&
                    lookBack()->type != TokenType::INDENT) {
                    failAtCurrentToken(InvalidSyntaxError(
                        "Unexpected 'while' after " +
                        tokenTypeToString(lookBack()->type) + " '" +
                        lookBack()->value + "'"));
                }

                advance();
                auto condition = parseExpression();
                if (peek() == nullptr ||
                    peek()->type != TokenType::KEYWRD ||
                    peek()->value != "do") {
                    failAtCurrentToken(InvalidSyntaxError(
                        "Expected 'do' keyword after 'while' condition"));
                }
                advance();

                auto body = parseIndentedStatements("while");
                return std::make_unique<WhileNode>(
                    std::move(condition),
                    std::move(body)
                );

            } else if (peek()->value == "task") {
                if (lookAhead() == nullptr || lookAhead()->type != TokenType::IDENT) {
                    failAtCurrentToken(InvalidSyntaxError("Expected an identifier after 'task' keyword"));
                }

                advance();

                const std::string taskIdent = peek()->value;
                advance();

                std::vector<std::string> argumentNames;
                std::unordered_set<std::string> usingNames;
                if (peek() != nullptr && peek()->type == TokenType::KEYWRD && peek()->value == "using") {
                    advance();

                    if (peek() == nullptr || peek()->type != TokenType::IDENT) {
                        failAtCurrentToken(InvalidSyntaxError(
                            "Expected identifier after 'using' keyword"
                        ));
                    }

                    while (true) {
                        const std::string name = advance().value;
                        if (!usingNames.insert(name).second) {
                            fail(InvalidSyntaxError(
                                "Duplicate parameter name: '" + name + "'"
                            ));
                        }
                        argumentNames.push_back(name);

                        if (!match(TokenType::SEPARATOR)) {
                            break;
                        }
                        if (peek() == nullptr || peek()->type != TokenType::IDENT) {
                            failAtCurrentToken(InvalidSyntaxError(
                                "Expected identifier after separator"
                            ));
                        }
                    }
                }

                auto statements = std::make_unique<StatementsNode>();

                if (peek() == nullptr || peek()->type != TokenType::NEWLINE) {
                    failAtCurrentToken(InvalidSyntaxError("Expected newline, got: '" + (peek() == nullptr ? "end of input" : peek()->value) + "'"));
                }
                advance();
                skipNewlines();

                if (peek() == nullptr || peek()->type != TokenType::INDENT) {
                    failAtCurrentToken(InvalidSyntaxError("No expression inside function declaration"));
                }
                advance();

                while (peek() != nullptr && peek()->type != TokenType::DEDENT) {
                    if (match(TokenType::NEWLINE)) {
                        continue;
                    }
                    statements->statements.push_back(parseExpression());
                    match(TokenType::NEWLINE);
                }
                match(TokenType::DEDENT);

                return std::make_unique<TaskDeclareNode>(
                    std::move(statements),
                    taskIdent,
                    std::move(argumentNames),
                    std::move(usingNames)
                );
            } else if (peek()->value == "exec" || peek()->value == "scan") {
                return parseCompExpression();
            } else if (peek()->value == "yield") {
                advance();

                return std::make_unique<ReturnNode>(parseExpression());
            }
            else {
                failAtCurrentToken(InvalidSyntaxError("Unknown keyword '" + peek()->value + "'"));
            }
        }

        return parseCompExpression();
    }

    std::unique_ptr<ASTNode> parseArithmeticExpression() {
        auto node = parseTerm();

        while (peek() != nullptr &&
               (peek()->type == TokenType::PLUS ||
                peek()->type == TokenType::MINUS)) {
            Token op = advance();
            auto right = parseTerm();

            node = std::make_unique<BinOpNode>(
                std::move(node),
                std::move(op),
                std::move(right)
            );
        }

        return node;
    }

    // term := unary (('*' | '/' | '%' | '**') unary)*
    //
    // Multiplication, division, modulus, and exponentiation have higher
    // precedence than addition and subtraction.
    std::unique_ptr<ASTNode> parseTerm() {
        auto node = parseUnary();

        while (peek() != nullptr &&
               (peek()->type == TokenType::MUL ||
                peek()->type == TokenType::DIV ||
                peek()->type == TokenType::MOD ||
                peek()->type == TokenType::POW)) {
            Token op = advance();
            auto right = parseUnary();

            node = std::make_unique<BinOpNode>(
                std::move(node),
                std::move(op),
                std::move(right)
            );
        }

        return node;
    }

        std::unique_ptr<ASTNode> parseUnary() {
            // Unary plus does not change the value.
            if (match(TokenType::PLUS)) {
                return parseUnary();
            }

            const Token* token = peek();

            if (token != nullptr &&
                (token->type == TokenType::UNARYOP ||
                token->type == TokenType::MINUS)) {

                Token op = advance();

                // Parse the operand recursively.
                auto operand = parseUnary();

                return std::make_unique<UnaryOpNode>(
                    std::move(op),
                    std::move(operand)
                );
            }

            return parsePrimary();
        }

    // primary := literal | identifier | '(' expression ')' | list
    // postfix := primary ('[' expression ']')*
    std::unique_ptr<ASTNode> parsePrimary() {
        const Token* token = peek();
        std::unique_ptr<ASTNode> node;

        if (token != nullptr &&
            (token->type == TokenType::INT ||
             token->type == TokenType::FLOAT)) {

            node = std::make_unique<NumberNode>(advance());
        } else if (token != nullptr && token->type == TokenType::IDENT) {
            Token identifier = advance();
            while (match(TokenType::DOT)) {
                if (peek() == nullptr || peek()->type != TokenType::IDENT) {
                    failAtCurrentToken(InvalidSyntaxError(
                        "Expected an identifier after '.'"));
                }
                identifier.value += "." + advance().value;
            }
            node = std::make_unique<VarAccessNode>(std::move(identifier));
        } else if (token != nullptr && token->type == TokenType::STRING) {
            node = std::make_unique<StringNode>(advance());
        } else if (token != nullptr &&
                   token->type == TokenType::KEYWRD &&
                   token->value == "exec") {
            node = parseExecNode();
        } else if (token != nullptr &&
                   token->type == TokenType::KEYWRD &&
                   token->value == "scan") {
            advance();
            node = std::make_unique<ScanNode>();
        } else if (match(TokenType::LPAREN)) {
            const Token* opening = lookBack();
            const size_t lparen_ln = opening->ln;
            const size_t lparen_col = opening->col;
            node = parseExpression();

            if (!match(TokenType::RPAREN)) {
                failAtCurrentToken(BrokenPairError(
                    std::string("Expected ')' after left parenthesis at \nline: ") +
                    std::to_string(lparen_ln) +
                    "\ncolumn: " +
                    std::to_string(lparen_col) +
                    "\n"
                ));
            }
        } else if (match(TokenType::SQRLBRCKT)) {
            std::vector<std::unique_ptr<ASTNode>> elements;
            if (!match(TokenType::SQRRBRCKT)) {
                while (true) {
                    if (peek() == nullptr ||
                        peek()->type == TokenType::NEWLINE ||
                        peek()->type == TokenType::SQRRBRCKT) {
                        failAtCurrentToken(InvalidSyntaxError(
                            "Expected a list element"));
                    }
                    elements.push_back(parseExpression());
                    if (match(TokenType::SQRRBRCKT)) {
                        break;
                    }
                    if (!match(TokenType::SEPARATOR)) {
                        failAtCurrentToken(InvalidSyntaxError(
                            "Expected ',' or ']' after list element"));
                    }
                    if (match(TokenType::SQRRBRCKT)) {
                        break;
                    }
                }
            }
            node = std::make_unique<ListNode>(std::move(elements));
        }

        if (!node) {
            failAtCurrentToken(InvalidSyntaxError(
                "Expected a value, list, or '('"));
        }

        while (match(TokenType::SQRLBRCKT)) {
            if (peek() == nullptr ||
                peek()->type == TokenType::SQRRBRCKT ||
                peek()->type == TokenType::SEPARATOR) {
                failAtCurrentToken(InvalidSyntaxError(
                    "Expected a list index"));
            }
            auto index = parseExpression();
            if (!match(TokenType::SQRRBRCKT)) {
                failAtCurrentToken(BrokenPairError(
                    "Expected ']' after list index"));
            }
            node = std::make_unique<IndexNode>(
                std::move(node), std::move(index));
        }

        return node;
    }
};

inline void printAST(const ASTNode* node, int depth = 2) {
    if (node == nullptr) {
        return;
    }

    std::string indent(depth * 2, ' ');

    if (const auto* statementsNode = dynamic_cast<const StatementsNode*>(node)) {
        for (const auto& statement : statementsNode->statements) {
            printAST(statement.get(), depth);
        }
    } else if (const auto* numberNode = dynamic_cast<const NumberNode*>(node)) {
        std::cout << indent << "Number: " << numberNode->token.value << std::endl;
    } else if (const auto* binOpNode = dynamic_cast<const BinOpNode*>(node)) {
        std::cout << indent << "Operator: " << binOpNode->op.value << std::endl;
        printAST(binOpNode->left.get(), depth + 1);
        printAST(binOpNode->right.get(), depth + 1);
    }
}

// int main() {
//     std::string input;
//     std::cout << "Enter input: ";
//     std::getline(std::cin, input);

//     Lexer lexer(input);
//     lexer.advance(); // Initialize the first character

//     try {
//         TokenList tokens = lexer.tokenize();
//         Parser parser(tokens);
//         auto ast = parser.parse();

//         printAST(ast.get());

//         std::cout << "Parsing successful!" << std::endl;
//     } catch (const std::runtime_error& e) {
//         std::cerr << e.what() << std::endl;
//     }

//     return 0;
// }

#endif
