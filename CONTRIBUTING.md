# Contributing

Every PR is welcome. To get yours merged quickly:

- Keep one change per pull request.
- If you fix a bug, add a test that fails without the fix.
- Make sure the build and the tests pass locally, including under sanitizers.
- In the PR description, explain what was wrong and how to reproduce it.

## Build and test

```sh
cmake -S . -B build
cmake --build build
cd build && ctest --output-on-failure
```

For changes in `src/`, also run the tests with AddressSanitizer and
UndefinedBehaviorSanitizer (GCC or Clang):

```sh
CFLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer -g" \
LDFLAGS="-fsanitize=address,undefined" \
cmake -S . -B build-asan
cmake --build build-asan
cd build-asan && ASAN_OPTIONS=detect_leaks=0 ctest --output-on-failure
```

CI also builds on Windows (MSVC and MinGW) and with optional features turned
off (`-DZIP_ENABLE_INFLATE=OFF`, `-DZIP_ENABLE_DEFLATE=OFF`,
`-DZIP_HAVE_SYMLINK=OFF`, `-DMINIZ_NO_STDIO=ON`). Keep code portable, and keep
feature-specific code inside the matching `#if` blocks.

## Tests

Tests live in `test/test_*.c` and use `test/minunit.h`. Add your case to the
file closest to the code you change. A new test file also needs
`add_executable`, `add_test` and `add_sanitizers` entries in
`test/CMakeLists.txt`.

Tests can only call the public API in `src/zip.h`; the `static` functions in
`src/zip.c` are not visible to them, so test the behavior through the public
API. A test that copies code out of `src/zip.c` checks the copy, not the
library.

## Formatting

Use the default LLVM `clang-format` style. CI checks it with clang-format 21;
other versions may format slightly differently.

```sh
git ls-files '*.c' '*.h' | xargs clang-format -i --style=LLVM
```

## Commit messages

Write a short lowercase subject that says what changes, for example
`reject entries with a zero compressed size on entry open`. If the reason isn't
obvious, explain it in the body.

## AI-assisted contributions

You may use AI tools, but you are responsible for every line and every claim
in the pull request:

- Say in the PR description that you used them.
- Build and run the tests yourself. If you write that a change was verified,
  include the commands you ran and their output.
- Check each claim against the code before making it: follow the callers and
  the branch conditions to confirm the bug can actually happen.

Pull requests whose description doesn't match what the code does will be
closed.

## Security issues

See [SECURITY.md](SECURITY.md).
