#!/usr/bin/env bash
# Compile and run all standalone unit tests in the test/ directory.
# No external dependencies: each test is a self-contained C++ program with main().

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FAILURES=0

for test_file in "$SCRIPT_DIR"/test_*.cpp; do
    [ -f "$test_file" ] || continue
    name="$(basename "$test_file" .cpp)"
    binary="$SCRIPT_DIR/$name"

    if g++ -std=c++17 -Wall -Wextra -o "$binary" "$test_file" 2>/tmp/build_err; then
        if "$binary"; then
            echo "PASS: $name"
        else
            echo "FAIL: $name (test returned non-zero)"
            FAILURES=$((FAILURES + 1))
        fi
    else
        echo "FAIL: $name (compilation error)"
        cat /tmp/build_err
        FAILURES=$((FAILURES + 1))
    fi
    rm -f "$binary"
done

echo ""
if [ "$FAILURES" -eq 0 ]; then
    echo "All tests passed."
    exit 0
else
    echo "$FAILURES test(s) failed."
    exit 1
fi
