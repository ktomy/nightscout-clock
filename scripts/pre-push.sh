#!/usr/bin/env bash
set -e

BOLD="\033[1m"
GREEN="\033[0;32m"
YELLOW="\033[0;33m"
RED="\033[0;31m"
CYAN="\033[0;36m"
DIM="\033[2m"
RESET="\033[0m"

PROJECT_DIR=$(git rev-parse --show-toplevel)

# Locate clang-format (needed to decide whether format:fix can run).
CLANG_FORMAT=""
for candidate in clang-format clang-format-18 clang-format-17 clang-format-16 \
                 clang-format-15 clang-format-14; do
    if command -v "$candidate" >/dev/null 2>&1; then
        CLANG_FORMAT="$candidate"
        break
    fi
done

if [[ -z "$CLANG_FORMAT" ]]; then
    printf "${YELLOW}⚡ pre-push:${RESET} clang-format not found; running tests only.\n" >&2
    SKIP_FORMAT=1
else
    SKIP_FORMAT=0
fi

FORMAT_SCRIPT="$PROJECT_DIR/scripts/check_format.sh"

tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT

names=()
logs=()
pids=()

# run_check <name> [command...]: runs the given command in the background,
# capturing its exit status and output for the spinner/report phase.
run_check() {
  local name="$1"
  shift
  local log="$tmpdir/$name.log"
  names+=("$name")
  logs+=("$log")
  printf "  ${CYAN}·${RESET} ${name}\n"
  (set +e; "$@" >"$log" 2>&1; echo $? >"$tmpdir/$name.status") &
  pids+=($!)
}

printf "${CYAN}${BOLD}🚀 pre-push: running checks in parallel...${RESET}\n"

# Format + auto-fix: delegate to scripts/check_format.sh --fix.
# It exits 0 even when it reformats files, so we must check the working tree
# afterwards. If it changed any .cpp/.h files, abort the push so the user can
# review and commit the formatting before pushing again.
if [ "$SKIP_FORMAT" -eq 0 ]; then
  run_check "format:fix" bash "$FORMAT_SCRIPT" --fix
fi

# Test check: the host-side test suite (C++ + Node.js).
run_check "test" bash "$PROJECT_DIR/test/run_tests.sh"

n=${#names[@]}
spinners=("⠋" "⠙" "⠹" "⠸" "⠼" "⠴" "⠦" "⠧" "⠇" "⠏")
tick=0

# Rewrite bullet lines in-place: spinner while running, ✔/✘ on completion.
while true; do
  spinner="${spinners[$((tick % ${#spinners[@]}))]}"
  all_done=1
  for i in "${!names[@]}"; do
    name="${names[$i]}"
    lines_up=$((n - i))
    [ -f "$tmpdir/$name.reported" ] && continue
    printf "\033[${lines_up}A\r\033[2K"
    if [ -f "$tmpdir/$name.status" ]; then
      status=$(cat "$tmpdir/$name.status")
      touch "$tmpdir/$name.reported"
      if [ "$status" -eq 0 ]; then
        printf "  ${GREEN}${BOLD}✔${RESET} ${name}"
      else
        printf "  ${RED}${BOLD}✘${RESET} ${name}"
      fi
    else
      all_done=0
      printf "  ${CYAN}${spinner}${RESET} ${name}"
    fi
    printf "\033[${lines_up}B\r"
  done
  [ "$all_done" -eq 1 ] && break
  sleep 0.1
  tick=$((tick + 1))
done

# After all checks finish, determine whether format:fix modified any files.
# clang-format -i modifies files on disk; git diff HEAD shows what changed.
reformatted=""
if [ "$SKIP_FORMAT" -eq 0 ]; then
  reformatted=$(git -C "$PROJECT_DIR" diff --name-only HEAD -- src/ | grep -E '\.(cpp|h)$' || true)
fi

# Print output for failed checks below the bullet list.
failed=0
for i in "${!names[@]}"; do
  name="${names[$i]}"
  status=$(cat "$tmpdir/$name.status")
  if [ "$status" -ne 0 ]; then
    failed=1
    printf "\n${RED}${BOLD}✘ ${name}${RESET}\n"
    printf "${DIM}"
    sed 's/^/  /' "$tmpdir/$name.log"
    printf "${RESET}"
  fi
done

# If format:fix reformatted files, treat that as a failure (abort push).
if [ -n "$reformatted" ]; then
  failed=1
  printf "\n${RED}${BOLD}✘ format:fix${RESET}\n"
  printf "${DIM}  clang-format reformatted the following files:${RESET}\n"
  for f in $reformatted; do
    printf "    ${CYAN}·${RESET} $f\n"
  done
  printf "${DIM}  Commit the formatting changes, then push again.${RESET}\n"
fi

for pid in "${pids[@]}"; do
  wait "$pid" 2>/dev/null || true
done

printf "\n"
if [ $failed -eq 0 ]; then
  printf "${GREEN}${BOLD}✅ pre-push: all checks passed.${RESET}\n"
else
  printf "${RED}${BOLD}❌ pre-push: push aborted.${RESET}\n"
fi

exit $failed
