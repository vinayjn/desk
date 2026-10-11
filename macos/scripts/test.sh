#!/usr/bin/env bash
set -euo pipefail
project_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
test_dir="$(mktemp -d "${TMPDIR:-/tmp}/desky-tests.XXXXXX")"
trap 'rm -rf "$test_dir"' EXIT
xcrun swiftc -swift-version 6 -strict-concurrency=complete -default-isolation MainActor -parse-as-library \
  "$project_dir/Desky/DeskProtocol.swift" \
  "$project_dir/Desky/DeskTransport.swift" \
  "$project_dir/Desky/ContentViewModel.swift" \
  "$project_dir/Desky/ContentView.swift" \
  "$project_dir/tests/desk_tests.swift" \
  -o "$test_dir/desk-tests"
"$test_dir/desk-tests"
