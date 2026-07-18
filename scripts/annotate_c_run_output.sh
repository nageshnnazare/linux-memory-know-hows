#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"

sources=()

if [[ $# -gt 0 ]]; then
  for arg in "$@"; do
    sources+=("${arg#./}")
  done
else
  while IFS= read -r -d '' path; do
    sources+=("${path#./}")
  done < <(find . -type f -name '*.c' -print0 | sort -z)
fi

run_log="$(mktemp)"
trap 'rm -f "$run_log"' EXIT

if ! make run >"$run_log" 2>&1; then
  echo "make run finished with a non-zero exit code; continuing to annotate successful demos."
fi
cat "$run_log"

for rel_path in "${sources[@]}"; do
  if [[ "$rel_path" == .github/* ]] || [[ "$rel_path" == scripts/* ]]; then
    continue
  fi

  src="$repo_root/$rel_path"
  if [[ ! -f "$src" ]]; then
    continue
  fi

  dir="$(dirname "$rel_path")"
  base="$(basename "$rel_path" .c)"
  target="$base"

  echo "Annotating $rel_path"
  if ! make -C "$dir" "$target" >/tmp/annotate-build.log 2>&1; then
    echo "Skipping $rel_path because the build failed."
    cat /tmp/annotate-build.log
    continue
  fi

  status=0
  output=""
  if ! output="$( (cd "$dir" && ./$target) 2>&1 )"; then
    status=$?
  fi

  if [[ -z "$output" ]]; then
    output="(no output)"
  fi

  escaped_output="$(printf '%s' "$output" | sed 's#\*/#* /#g')"
  temp_file="$(mktemp)"

  awk '
    /AUTO-GENERATED RUN OUTPUT START/ {skip=1; next}
    /AUTO-GENERATED RUN OUTPUT END/ {skip=0; next}
    skip==0 {print}
  ' "$src" > "$temp_file"
  mv "$temp_file" "$src"

  {
    echo ""
    echo "/*"
    echo " * AUTO-GENERATED RUN OUTPUT START"
    echo " * Source: $rel_path"
    echo " * Command: make -C ${dir#./} $target"
    echo " * Exit status: $status"
    echo " * Output:"
    while IFS= read -r line; do
      echo " * $line"
    done <<< "$escaped_output"
    echo " * AUTO-GENERATED RUN OUTPUT END"
    echo " */"
  } >> "$src"

done
