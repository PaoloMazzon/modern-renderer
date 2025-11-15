## Contributing
Use the `.clang-format` for formatting your code. Some things to keep in mind:

- `goto` and multi-line macros are strictly banned
- Write tests for your code where applicable (obviously not all render work can have automated tests)
- No esoteric names
- Keep C++ crazy OOP nonsense to a minimum
- Use snake_case for variables and functions, prefix methods with `m_` and use PascalCase for class/struct names
- Use `mvr_FunctionName` for user-facing `extern "C"` function names
- Use `MVR_StructName` for anything typedef'd in the user-facing `extern "C"` side
- Constants/enums are `MVR_CONSTANT_NAME` (user-facing)
- Any user-facing function with more than 3 parameters needs to pass a pointer to a struct instead of
  the parameters themselves. Name this struct `MVR_FunctionNameParams`
- Document user-facing functions properly with Doxygen notation
- User-facing structs should be named with a `_s` suffix and typedef'd with no suffix
- Internal C++ things should all be in the `MVRender` namespace

Take a peek at the project tab or issues to see what work needs to be done.