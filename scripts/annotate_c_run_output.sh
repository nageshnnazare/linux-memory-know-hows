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

run_timeout_seconds="${RUN_TIMEOUT_SECONDS:-600}"
demo_timeout_seconds="${DEMO_TIMEOUT_SECONDS:-15}"

run_with_timeout() {
  local timeout_seconds="$1"
  shift

  local python_bin=""
  if command -v python3 >/dev/null 2>&1; then
    python_bin="python3"
  elif command -v python >/dev/null 2>&1; then
    python_bin="python"
  else
    echo "python3/python not found; cannot enforce timeouts" >&2
    return 127
  fi

  "$python_bin" - "$timeout_seconds" "$@" <<'PY'
import subprocess
import sys

timeout_seconds = int(sys.argv[1])
cmd = sys.argv[2:]

try:
    completed = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout_seconds)
    if completed.stdout:
        sys.stdout.write(completed.stdout)
    if completed.stderr:
        sys.stderr.write(completed.stderr)
    sys.exit(completed.returncode)
except subprocess.TimeoutExpired as exc:
    if exc.stdout:
        sys.stdout.write(exc.stdout)
    if exc.stderr:
        sys.stderr.write(exc.stderr)
    sys.exit(124)
PY
}

run_log="$(mktemp)"
trap 'rm -f "$run_log"' EXIT

if ! run_with_timeout "$run_timeout_seconds" make run >"$run_log" 2>&1; then
  echo "make run finished with a non-zero exit code or timed out; continuing to annotate successful demos."
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
  if ! output="$( (cd "$dir" && run_with_timeout "$demo_timeout_seconds" ./$target) 2>&1 )"; then
    status=$?
  fi

  if [[ -z "$output" ]]; then
    if [[ $status -eq 124 ]]; then
      output="(timed out after ${demo_timeout_seconds}s)"
    else
      output="(no output)"
    fi
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
