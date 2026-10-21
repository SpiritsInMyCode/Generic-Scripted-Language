# Tasks in GSL

Tasks are named functions or procedures in GSL. They are declared with the `task` keyword and can accept parameters declared with `using`.

## Basic task declaration

```gsl
task greet using name
    out "Hello, " + name
```

To invoke a task, use `exec`:

```gsl
exec greet "Ada"
```

## Parameters and `using`

Parameters are listed after `using`:

```gsl
task square using n
    yield n * n
```

The `using` names are the parameter names; when a task is executed, the arguments are bound to those names in the task's local scope.

## Returning a value with `yield`

`yield` returns a value from a task. The interpreter expects a task body to end with `yield` when a value should be produced.

```gsl
task identity using value
    yield value

out exec identity 42
```

This example prints:

```text
42
```

## Executing a task

The task call syntax is:

```gsl
exec taskName argument1 argument2
```

For example:

```gsl
task add using a, b
    yield a + b

out exec add 10 20
```

A comma can separate arguments, and an argument can itself be a nested expression:

```gsl
out exec identity (1 + 2)
```

## Tasks and scope

Tasks create their own scope. Variables declared inside a task are local to that task unless the task explicitly yields a value or stores state in a surrounding scope.

Example:

```gsl
task scale using value
    decl doubled = value * 2
    yield doubled

out exec scale 7
```

## Reading input into tasks

Tasks often consume input from `scan`:

```gsl
task echo using text
    yield text

out exec echo scan
```

This reads one line from standard input and returns it to the caller.
