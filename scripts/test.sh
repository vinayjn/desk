#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "$0")/.." && pwd)"
test_dir="$(mktemp -d "${TMPDIR:-/tmp}/desk-tests.XXXXXX")"
trap 'rm -rf "$test_dir"' EXIT

"${CXX:-c++}" -std=c++11 -Wall -Wextra -Werror \
  -fsanitize=address,undefined -fno-omit-frame-pointer \
  -I "$project_root" "$project_root/tests/core_tests.cpp" \
  "$project_root/Frame.cpp" "$project_root/FrameBuilder.cpp" \
  "$project_root/DeskState.cpp" \
  -o "$test_dir/core_tests"
"$test_dir/core_tests"

"${CXX:-c++}" -std=c++11 -Wall -Wextra -Werror \
  -fsanitize=address,undefined -fno-omit-frame-pointer \
  -I "$project_root/tests/fakes" -I "$project_root" \
  "$project_root/tests/controller_tests.cpp" "$project_root/DeskController.cpp" \
  "$project_root/SerialProxy.cpp" "$project_root/Logger.cpp" \
  "$project_root/BLEInterface.cpp" \
  "$project_root/Frame.cpp" "$project_root/FrameBuilder.cpp" "$project_root/DeskState.cpp" \
  -o "$test_dir/controller_tests"
"$test_dir/controller_tests"
