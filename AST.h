#ifndef AST_H
#define AST_H

#include <string>
#include <vector>
#include <memory>
#include <unordered_set>

enum class TokenType {
    INT,
    FLOAT,
    PLUS,
    MINUS,
    MUL,
    DIV,
    MOD,
    POW,
    LPAREN,
    RPAREN,
    UNARYOP,
    IDENT,
    EQ,
    KEYWRD,
    NEWLINE,
    EE,
    LT,
    GT,
    LTE,
    GTE,
    NE,
    INDENT,
    DEDENT,
    STRING,
    SEPARATOR,
    DOT,
    SQRLBRCKT,
    SQRRBRCKT
};


class Token {
    public:
        TokenType type;
        std::string value;

        size_t ln;
        size_t col;

        Token(TokenType type, const std::string& value, size_t ln, size_t col) : type(type), value(value), ln(ln), col(col) {}
};

struct ASTNode {
    virtual ~ASTNode() = default;
};

struct StatementsNode : ASTNode {
    std::vector<std::unique_ptr<ASTNode>> statements;

    StatementsNode() = default;
    explicit StatementsNode(std::vector<std::unique_ptr<ASTNode>> stmts)
        : statements(std::move(stmts)) {}
};

struct IfNode : ASTNode {
    std::unique_ptr<ASTNode> condition;
    std::unique_ptr<StatementsNode> body;
    std::unique_ptr<StatementsNode> otherwise;

    IfNode(std::unique_ptr<ASTNode> cond, std::unique_ptr<StatementsNode> bdy, std::unique_ptr<StatementsNode> othr)
        : condition(std::move(cond)), body(std::move(bdy)), otherwise(std::move(othr)) {}
};

struct LoopNode : ASTNode {
    std::unique_ptr<ASTNode> iterable;
    std::string binding;
    std::unique_ptr<StatementsNode> body;

    LoopNode(
        std::unique_ptr<ASTNode> iterable,
        std::string binding,
        std::unique_ptr<StatementsNode> body
    )
        : iterable(std::move(iterable)),
          binding(std::move(binding)),
          body(std::move(body)) {}
};

struct WhileNode : ASTNode {
    std::unique_ptr<ASTNode> condition;
    std::unique_ptr<StatementsNode> body;

    WhileNode(
        std::unique_ptr<ASTNode> condition,
        std::unique_ptr<StatementsNode> body
    )
        : condition(std::move(condition)),
          body(std::move(body)) {}
};

struct ReturnNode : ASTNode {
    std::unique_ptr<ASTNode> returnVal;

    ReturnNode(std::unique_ptr<ASTNode> node) : returnVal(std::move(node)) {}
};

struct VarAccessNode : ASTNode {
    Token token;

    explicit VarAccessNode(Token t)
        : token(std::move(t)) {}
};

struct OutNode : ASTNode {
    std::unique_ptr<ASTNode> value;

    explicit OutNode(std::unique_ptr<ASTNode> val)
        : value(std::move(val)) {}
};

struct VarAssignNode : ASTNode {
    Token token;
    std::unique_ptr<ASTNode> value;

    VarAssignNode(Token t, std::unique_ptr<ASTNode> val)
        : token(std::move(t)), value(std::move(val)) {}
};

struct NumberNode : ASTNode {
    Token token;

    explicit NumberNode(Token t)
        : token(std::move(t)) {}
};

struct StringNode : ASTNode {
    Token value;

    explicit StringNode(Token t) : value(std::move(t)) {}
};

struct ListNode : ASTNode {
    std::vector<std::unique_ptr<ASTNode>> elements;

    explicit ListNode(std::vector<std::unique_ptr<ASTNode>> values)
        : elements(std::move(values)) {}
};

struct IndexNode : ASTNode {
    std::unique_ptr<ASTNode> collection;
    std::unique_ptr<ASTNode> index;

    IndexNode(std::unique_ptr<ASTNode> collection, std::unique_ptr<ASTNode> index)
        : collection(std::move(collection)), index(std::move(index)) {}
};

struct TaskDeclareNode : ASTNode {
    std::shared_ptr<StatementsNode> statements;
    std::string ident;
    std::vector<std::string> argumentNames;
    std::unordered_set<std::string> usingNames;

    explicit TaskDeclareNode(
        std::unique_ptr<StatementsNode> statements,
        const std::string& ident,
        std::vector<std::string> argumentNames = {},
        std::unordered_set<std::string> usingNames = {}
    )
        : statements(std::move(statements)),
          ident(std::move(ident)),
          argumentNames(std::move(argumentNames)),
          usingNames(std::move(usingNames)) {}
};

struct SublibDeclareNode : ASTNode {
    std::string ident;
    std::vector<std::string> initFields;
    std::shared_ptr<StatementsNode> statements;

    SublibDeclareNode(
        std::string ident,
        std::vector<std::string> initFields,
        std::unique_ptr<StatementsNode> statements
    )
        : ident(std::move(ident)),
          initFields(std::move(initFields)),
          statements(std::move(statements)) {}
};

struct ExecNode : ASTNode {
    std::string ident;
    std::vector<std::unique_ptr<ASTNode>> arguments;

    ExecNode(const std::string& ident, std::vector<std::unique_ptr<ASTNode>> arguments)
        : ident(std::move(ident)), arguments(std::move(arguments)) {}
};

struct CreateNode : ASTNode {
    std::string ident;
    std::vector<std::unique_ptr<ASTNode>> arguments;

    CreateNode(
        std::string ident,
        std::vector<std::unique_ptr<ASTNode>> arguments
    )
        : ident(std::move(ident)), arguments(std::move(arguments)) {}
};

struct ImportNode : ASTNode {
    std::string filename;

    explicit ImportNode(std::string filename)
        : filename(std::move(filename)) {}
};

struct UnpackNode : ASTNode {
    std::string library;

    explicit UnpackNode(std::string library)
        : library(std::move(library)) {}
};

struct ScanNode : ASTNode {};

struct UnaryOpNode : ASTNode {
    Token op;
    std::unique_ptr<ASTNode> operand;

    UnaryOpNode(Token op, std::unique_ptr<ASTNode> expr)
        : op(std::move(op)), operand(std::move(expr)) {}
};

struct BinOpNode : ASTNode {
    std::unique_ptr<ASTNode> left;
    Token op;
    std::unique_ptr<ASTNode> right;

    BinOpNode(
        std::unique_ptr<ASTNode> left,
        Token op,
        std::unique_ptr<ASTNode> right
    )
        : left(std::move(left)),
          op(std::move(op)),
          right(std::move(right)) {}
};

typedef std::vector<Token> TokenList;

#endif
