# GSL Syntax

This page covers the core syntax used by the language implementation in this project.

## Variables

Use `decl` to create or update a variable:

```gsl
decl answer = 42
out answer
```

Variables are stored in the current scope. Later assignments overwrite the current value in the active scope.

## Literals

GSL supports:

- integers: `42`
- floating-point numbers: `3.14`
- strings: `"hello"` or `'hello'`
- lists: `[1, 2, 3]`

Example:

```gsl
decl values = [10, "hello", [20, 30]]
out values[0]
out values[2][1]
```

## Operators

### Arithmetic

```gsl
+   -   *   /   %   **
```

Examples:

```gsl
decl total = 2 + 3 * 4
out total
```

The project treats `%` as modulus and `**` as exponentiation. Integer division preserves the interpreter's integer-division behavior when both operands are integers.

### Comparison

```gsl
==  !=  <  >  <=  >=
```

Examples:

```gsl
if 10 > 5 do
    out "yes"
```

### Logical

```gsl
@   &   |
```

- `@` is logical NOT
- `&` is logical AND
- `|` is logical OR

Logical results are treated as numeric `0` or `1` in the interpreter.

## Conditionals

The syntax is:

```gsl
if condition do
    ...
otherwise
    ...
```

Example:

```gsl
if 5 > 2 do
    out "greater"
otherwise
    out "not greater"
```

The `otherwise` block is optional.

## Loops

### `loop ... bind ...`

```gsl
loop values bind value
    out value
```

This iterates over a list, a string, or a numeric range.

Example:

```gsl
decl items = [1, 2, 3]
loop items bind item
    out item
```

### `while ... do`

```gsl
decl count = 0
while count < 3 do
    out count
    decl count = count + 1
```

The loop body runs while the condition is truthy.

## Comments

GSL uses `!-` for single-line comments:

```gsl
!- This is a comment
out "still runs"
```

Comments are ignored by the lexer and parser.

## Newlines and blocks

GSL uses indentation to define blocks. A block begins after `do` and continues while the indentation stays deeper than the surrounding scope.

Example:

```gsl
if true do
    out "inside"
```

The parser also accepts semicolons as statement separators, but indentation is the normal block form used in the project.
