# Getting Started with GSL

This project includes a small interpreter and parser for the GSL language. The executable entry point is `GSL0.1-Windows/executable_handler.cpp`, and it reads a `.gsl` source file, parses it, and runs it.

## Build the interpreter

From the project root (`d:\Projects`), compile the interpreter with:

```powershell
C:/msys64/ucrt64/bin/g++.exe -std=c++17 .\GSL0.1-Windows\executable_handler.cpp -o .\executable_handler.exe
```

This matches the build task defined in the workspace configuration.

## Create your first program

Create a file named `hello.gsl` in the project folder:

```gsl
out "Hello, world!"
```

## Run it

```powershell
.\executable_handler.exe .\hello.gsl
```

Expected output:

```text
Hello, world!
```

## Notes

- `out` writes a value to standard output.
- The interpreter treats the file path you pass as the script entry point.
- Imports and built-in libraries are resolved relative to the interpreter and the current script directory.
- A script can read input with `scan` and call tasks with `exec`.

For the language details, continue with the syntax and task documentation in this folder.
