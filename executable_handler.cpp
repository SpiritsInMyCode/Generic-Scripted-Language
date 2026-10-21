#include "Parser.h"
#include "lexer.h"
#include "Interpreter.h"
#include "NativeLibraries.h"

#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>
#include <iterator>

int main(int argc, char* argv[])
{
    if (argc == 2 && std::string(argv[1]) == "--check-stdin") {
        std::string input(
            (std::istreambuf_iterator<char>(std::cin)),
            std::istreambuf_iterator<char>());

        try {
            Lexer lexer(input);
            Parser parser(lexer.tokenize());
            parser.parse();
        }
        catch (const std::runtime_error& e) {
            std::cerr << e.what() << std::endl;
            return 1;
        }

        return 0;
    }

    if (argc < 2) {
        std::cerr << "Usage: executable_handler <source.gsl>\n";
        return 1;
    }

    const std::filesystem::path filePath =
        std::filesystem::absolute(argv[1]);
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filePath.string() << '\n';
        return 1;
    }

    std::string input((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    try {
        Lexer lexer(input);
        Parser parser(lexer.tokenize());
        auto ast = parser.parse();

        Interpreter interpreter;
        registerBuiltins(interpreter);
        interpreter.globalSymbolTable.set("void", Value(0));
        interpreter.handlerSourceDirectory =
            std::filesystem::absolute(std::filesystem::path(__FILE__)).parent_path();
        interpreter.mainSourceDirectory = filePath.parent_path();
        interpreter.currentSourceDirectory = filePath.parent_path();
        interpreter.visit(ast);
    }
    catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    system("pause");

    return 0;
}