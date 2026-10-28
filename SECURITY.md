# Security Policy

## Supported versions

Fixes go to `master` and ship in the next release. Older releases are not
patched.

## Reporting a vulnerability

Open an issue at https://github.com/kuba--/zip/issues. Even better, send a pull
request with the fix and a test (see [CONTRIBUTING.md](CONTRIBUTING.md)).

A report needs:

1. The release or commit you tested.
2. Your platform, compiler and build flags.
3. A reproducer that goes through the public API in `src/zip.h`. That is
   either an archive plus a small program that calls `zip_*` functions, or an
   input that crashes one of the fuzz targets in `fuzz/` (`fuzz_entry.c`,
   `fuzz_stream.c`).
4. The sanitizer output from running the reproducer against a sanitizer build
   of the library.
5. The impact: what an attacker controls (archive contents, entry names,
   sizes) and what they gain.

Reports without a reproducer will be closed. Reopen the issue, or open a new
one, once you have a reproducer.

## Building a reproducer

Build the library with AddressSanitizer and UndefinedBehaviorSanitizer (GCC or
Clang), then link your program against it:

```sh
CFLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer -g" \
LDFLAGS="-fsanitize=address,undefined" \
cmake -S . -B build-asan
cmake --build build-asan
cc -fsanitize=address,undefined -g -Isrc repro.c build-asan/libzip.a -o repro
./repro
```

A minimal `repro.c`:

```c
#include <zip.h>

int main(void) {
  struct zip_t *zip = zip_open("crash.zip", 0, 'r');
  /* the calls that trigger the bug */
  zip_close(zip);
  return 0;
}
```

## What is not a vulnerability

- Code that is only wrong for values no public API call can produce, for
  example an internal `static` function that would misbehave with a length its
  callers never pass. Hardening for cases like this is welcome as a regular
  pull request.
- Findings from code reading, static analysis or AI tools that come without a
  reproducer.
- Programs that copy code out of `src/` and test the copy instead of the
  library.
- Misuse of the API, such as passing invalid or freed pointers, or using a
  handle after `zip_close`.
- Bugs in code that wraps this library (language bindings, applications).
  Report those to that project.

## AI-assisted reports

You may use AI tools to find or describe an issue. Say so in the report, and
check every claim against the code yourself. The reproducer requirement still
applies.

## Advisories and CVEs

The maintainer publishes an advisory or requests a CVE only for issues
confirmed to be reachable through the public API. Merging a hardening pull
request does not confirm a vulnerability.
