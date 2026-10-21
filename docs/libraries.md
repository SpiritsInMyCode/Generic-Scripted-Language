# Libraries and Imports

GSL supports native libraries and imported modules. This project loads built-in native libraries automatically and supports explicit loading of additional modules.

## Built-in libraries

The interpreter registers the following libraries at startup:

- `std`
- `list`
- `random`

The standard library is always available. The others are native libraries that can be loaded by import or by explicit native registration.

## Importing a library

Use `import` with a string filename or native library name:

```gsl
import "list"
```

This loads the named library or the module file. If a file is imported, the file's stem becomes the library name.

For example, the project includes a module file:

```text
import/maths.gsl
```

The library name becomes `maths` when imported with:

```gsl
import "maths.gsl"
```

The interpreter resolves the import by trying the handler's import directory and the script's source directory.

## Accessing library members

Library members are accessed with dot notation:

```gsl
import "list"
out exec list.append [1, 2] 3
```

This calls the `append` task on the `list` library and appends `3` to the list.

## `unpack`

`unpack` copies the tasks and symbols from a loaded library into the current scope:

```gsl
import "list"
unpack list

out exec append [1, 2] 3
```

After unpacking, the imported tasks can be invoked without qualifying them with the library name.

## File libraries

A module file is parsed as a GSL program. It exposes its public tasks and variables as a library entry named after the file stem:

```gsl
!- in maths.gsl

task square using n
    yield n * n
```

Then another script can do:

```gsl
import "maths.gsl"
out exec maths.square 4
```

This is the standard way to split reusable functionality into separate files.
