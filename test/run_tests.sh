#!/usr/bin/env bash

# Runs the host-side tests. They need only g++ and Node.js, and no clock.

set -euo pipefail

TEST_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
PROJECT_DIR=$(dirname -- "$TEST_DIR")
BUILD_DIR=$(mktemp -d)
trap 'rm -rf "$BUILD_DIR"' EXIT
failures=0

for test_file in "$TEST_DIR"/test_*.cpp; do
    name=$(basename "$test_file" .cpp)
    if g++ -std=c++17 -Wall -Wextra -Werror -I "$PROJECT_DIR/src" -o "$BUILD_DIR/$name" "$test_file" &&
        "$BUILD_DIR/$name"; then
        echo "PASS: $name"
    else
        echo "FAIL: $name"
        failures=$((failures + 1))
    fi
done

if ! node --test "$TEST_DIR"/test_*.mjs; then
    failures=$((failures + 1))
fi

if ((failures)); then
    echo "$failures test group(s) failed." >&2
    exit 1
fi
echo "All tests passed."
