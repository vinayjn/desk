#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "$0")/.." && pwd)"
test_dir="$(mktemp -d "${TMPDIR:-/tmp}/desk-tests.XXXXXX")"
trap 'rm -rf "$test_dir"' EXIT

"${CXX:-c++}" -std=c++11 -Wall -Wextra -Werror \
  -fsanitize=address,undefined -fno-omit-frame-pointer \
  -I "$project_root" "$project_root/tests/core_tests.cpp" \
  "$project_root/Frame.cpp" "$project_root/FrameBuilder.cpp" \
  -o "$test_dir/core_tests"
"$test_dir/core_tests"
