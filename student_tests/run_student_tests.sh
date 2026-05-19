#!/usr/bin/env bash
set -u

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
STUDENT_DIR="$ROOT_DIR/student_tests"

CXX=${CXX:-g++}
COMMON_FLAGS=( -Wall -Wpedantic --std=c++17 -I"$ROOT_DIR" -g )

run_cmd() {
  local label="$1"; shift
  echo ""
  echo "=== ${label} ==="
  "$@"
}

compile_and_run() {
  local src="$1"
  local exe="$2"
  shift 2

  run_cmd "compile $(basename "$src")" "$CXX" "${COMMON_FLAGS[@]}" "$ROOT_DIR"/*.cpp "$src" -o "$exe"
  run_cmd "run $(basename "$src")" timeout 10s "$exe"
}

compile_and_run_asan() {
  local src="$1"
  local exe="$2"

  local asan_flags=( -fsanitize=address -fno-omit-frame-pointer )

  run_cmd "compile (ASAN) $(basename "$src")" "$CXX" "${COMMON_FLAGS[@]}" "${asan_flags[@]}" "$ROOT_DIR"/*.cpp "$src" -o "$exe"

  run_cmd "run (ASAN) $(basename "$src")" env ASAN_OPTIONS=detect_stack_use_after_return=true timeout 10s "$exe"
}

fail_count=0

# Regular student tests (test1..test8), but test5/test8 are wrappers that don't fit multi-file projects.
for t in "$STUDENT_DIR"/test*.cpp; do
  base="$(basename "$t")"
  case "$base" in
    test5.cpp)
      echo ""
      echo "=== ${base} ==="
      echo "Skipping wrapper; running intended ASAN scenario DoNotRunThis.cpp instead."
      if ! compile_and_run_asan "$STUDENT_DIR/DoNotRunThis.cpp" "/tmp/os1_test5_asan"; then
        echo "FAIL"
        fail_count=$((fail_count+1))
      else
        echo "OK"
      fi
      ;;
    test8.cpp)
      echo ""
      echo "=== ${base} ==="
      echo "Skipping wrapper; running intended ASAN scenario DoNotRunThis2.cpp instead."
      if ! compile_and_run_asan "$STUDENT_DIR/DoNotRunThis2.cpp" "/tmp/os1_test8_asan"; then
        echo "FAIL"
        fail_count=$((fail_count+1))
      else
        echo "OK"
      fi
      ;;
    *)
      exe="/tmp/os1_${base%.cpp}"
      if ! compile_and_run "$t" "$exe"; then
        rc=$?
        if [ $rc -eq 124 ]; then
          echo "TIMEOUT"
        else
          echo "FAIL (exit code $rc)"
        fi
        fail_count=$((fail_count+1))
      else
        echo "OK"
      fi
      ;;
  esac

done

echo ""
if [ $fail_count -eq 0 ]; then
  echo "All student tests passed."
  exit 0
fi

echo "$fail_count student test(s) failed."
exit 1
