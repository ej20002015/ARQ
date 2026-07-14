# Code style

Formatting rules are defined primarily in [`.editorconfig`](.editorconfig). This
document records conventions that are not readily expressed there and can be
extended as additional conventions are agreed.

## Line endings

- Follow the line-ending rules defined by `.editorconfig` and `.gitattributes`.
- Text files use LF by default.
- Windows command scripts (`.bat` and `.cmd`) use CRLF.
- Files must end with a newline.
- Do not mix line-ending styles within a file.

## C++

### Prefer fixed-width integer types

Prefer the fixed-width integer types from `<cstdint>` (for example,
`int32_t` and `uint64_t`) when the intended size is known. This makes
range and representation expectations explicit.

### Align related declarations and assignments and other code

Where it improves readability, horizontally align related declarations and
assignments. Align types, identifiers and equals signs into consistent columns.
(horizontal alignment may also be used in class method declarations, using statements,
etc. - anything where it makes the code easier to read basically)

```cpp
const int32_t  retry_count = 3;
const uint64_t timeout_ns  = 1'000'000;

minimum_price = 100.0;
maximum_price = 125.0;
```

Do not force alignment where the declarations are unrelated or where it would
make the code harder to maintain.

### Prefer `const`

Use `const` wherever it expresses a meaningful invariant. In particular,
prefer it for local values, references, pointers and member functions that are
not intended to modify state.

Do not add `const` where it has no useful effect, such as a top-level `const` on
a non-reference return type. Avoid it where it would prevent an intended move
or introduce unnecessary copying or another material performance cost.
