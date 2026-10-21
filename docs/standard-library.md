# Standard Library Reference

The built-in `std` library is loaded automatically by the interpreter. It exports a small set of native functions for common operations.

## `abs(number)`

Returns the absolute value of a number.

```gsl
out exec abs -5
```

Output:

```text
5
```

Arguments:

- `number`: numeric value

## `len(value)`

Returns the length of a string or list.

```gsl
out exec len "hello"
out exec len [10, 20, 30]
```

Arguments:

- `value`: a string or list

## `toNumber(value)`

Converts a number or a numeric string into a GSL number.

```gsl
out exec toNumber "42"
out exec toNumber 7
```

Arguments:

- `value`: a number or string containing a valid numeric literal

## Other native libraries

The interpreter also registers the `list` and `random` libraries:

### `list.append(list, value)`

Adds a value to the end of a list.

```gsl
import "list"
decl values = [1, 2]
exec list.append values 3
out values
```

Arguments:

- `list`: the target list
- `value`: the value to append

### `list.conjoin(list1, list2)`

Appends the elements of one list to another and returns the mutated list.

```gsl
import "list"
decl left = [1]
decl right = [2, 3]
out exec list.conjoin left right
```

Arguments:

- `list1`: destination list
- `list2`: source list

### `random.random(min, max)`

Returns a random integer or floating-point value between the given bounds.

```gsl
import "random"
out exec random.random 1 10
```

Arguments:

- `min`: lower bound
- `max`: upper bound

If both values are integers, the result is an integer in the inclusive range. If either is floating-point, the result is a real-number random value.

## Calling native functions

Native functions are called with the same `exec` syntax as tasks:

```gsl
out exec len [1, 2, 3]
out exec abs -9
```

This makes them behave like regular tasks while still being implemented in C++.
