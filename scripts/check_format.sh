#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
PROJECT_DIR=$(dirname -- "$SCRIPT_DIR")
CLANG_FORMAT_FILE="$PROJECT_DIR/.clang-format"

APPLY=false
TARGETS=()

usage() {
    echo "Usage: $0 [--fix] [--help] [path ...]"
    echo "  default    Check C++ source formatting against .clang-format"
    echo "  --fix      Apply formatting in place (requires clang-format)"
    echo "  path       Files or directories to check (default: src/)"
}

while (($#)); do
    case $1 in
        --fix | -f)
            APPLY=true
            ;;
        -h | --help)
            usage
            exit
            ;;
        *)
            TARGETS+=("$1")
            ;;
    esac
    shift
done

# Locate a clang-format binary.
CLANG_FORMAT=""
for candidate in clang-format clang-format-18 clang-format-17 clang-format-16 \
                 clang-format-15 clang-format-14; do
    if command -v "$candidate" >/dev/null 2>&1; then
        CLANG_FORMAT="$candidate"
        break
    fi
done

if [[ -z "$CLANG_FORMAT" ]]; then
    echo "ERROR: clang-format not found." >&2
    echo "Install it, for example:" >&2
    echo "  macOS:   brew install clang-format" >&2
    echo "  Linux:   apt install clang-format" >&2
    echo "Then re-run $0." >&2
    exit 1
fi

if [[ ! -f "$CLANG_FORMAT_FILE" ]]; then
    echo "ERROR: .clang-format not found at $CLANG_FORMAT_FILE" >&2
    exit 1
fi

# Collect the C++ source files to check or format.
if (( ${#TARGETS[@]} == 0 )); then
    TARGETS=("$PROJECT_DIR/src")
fi

FILES=()
for target in "${TARGETS[@]}"; do
    if [[ -f "$target" ]]; then
        FILES+=("$target")
    elif [[ -d "$target" ]]; then
        while IFS= read -r -d '' file; do
            FILES+=("$file")
        done < <(find "$target" -type f \( -name '*.cpp' -o -name '*.h' \) -print0 | sort -z)
    else
        echo "ERROR: not a file or directory: $target" >&2
        exit 1
    fi
done

if (( ${#FILES[@]} == 0 )); then
    echo "No C++ source files found."
    exit 0
fi

run_clang_format() {
    "$CLANG_FORMAT" --style=file "$@"
}

if [[ "$APPLY" == true ]]; then
    echo "Applying clang-format to ${#FILES[@]} file(s)..."
    run_clang_format -i "${FILES[@]+"${FILES[@]}"}"
    echo "Done."
else
    echo "Checking formatting of ${#FILES[@]} file(s) with $CLANG_FORMAT..."
    NEEDS_FORMAT=()
    for file in "${FILES[@]+"${FILES[@]}"}"; do
        if ! run_clang_format --dry-run --Werror "$file" >/dev/null 2>&1; then
            NEEDS_FORMAT+=("$file")
        fi
    done
    if (( ${#NEEDS_FORMAT[@]} > 0 )); then
        echo "Formatting issues found in ${#NEEDS_FORMAT[@]} file(s):" >&2
        for file in "${NEEDS_FORMAT[@]+"${NEEDS_FORMAT[@]}"}"; do
            echo "  $file" >&2
        done
        echo "Run '$0 --fix' to apply formatting." >&2
        exit 1
    fi
    echo "All files are correctly formatted."
fi
