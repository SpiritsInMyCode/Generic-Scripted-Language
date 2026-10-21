# GSL — Generic Scripted Language

GSL is an interpreted scripting language implemented in C++. It uses a custom lexer, parser, abstract syntax tree, and interpreter.

## Features

- Variables and arithmetic expressions
- Conditional execution
- Loops
- User-defined tasks and return values
- Input and output
- Native libraries
- Nested scopes
- File importing

*Feature list should reflect the current implementation.*

## Quick Start

### Requirements

- A C++17-compatible compiler
- GCC/MinGW or another supported compiler

### Build

```bash
g++ -std=c++17 -O2 executable_handler.cpp -o gsl
```

On Windows with MinGW, the output can be named `gsl.exe`.

### Run a program

```bash
./gsl examples/hello_world.gsl
```

On Windows:

```powershell
.\gsl.exe examples\hello_world.gsl
```

## Documentation

See [`docs/`](docs/) for language syntax, tutorials, and library references.

## Examples

See [`examples/`](examples/) for sample GSL programs.

## Project Status

GSL is under active development. Some features and APIs may change.

