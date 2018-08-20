#!/bin/bash
set -e
function codecov() {
    local dir=$(git rev-parse --show-toplevel)
    cd "$dir" || exit 1

    local gcov
    case "$CC" in
        gcc*) gcov=${CC/gcc/gcov}
            ;;
        *) gcov="llvm-cov gcov"
            ;;
    esac
    find build -name '*.gcno' -exec "$gcov" {} \;
    bash <(curl -s https://codecov.io/bash) || \
        echo "Codecov did not collect coverage reports"
}
codecov
