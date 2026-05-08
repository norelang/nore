#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../../.." && pwd)"
COMPILER_BIN="${NORE_BIN:-$ROOT_DIR/norec}"
BIN_DIR="$ROOT_DIR/tmp/bootstrap/bins"
ASSERT_SRC="$BIN_DIR/assert_runtime_failure.nore"

mkdir -p "$BIN_DIR"
cat > "$ASSERT_SRC" <<'EOF'
func bump_and_false(mut ref n: i64): bool = {
    n = n + 1
    false
}

func main(): void = {
    mut n: i64 = 0
    assert bump_and_false(mut ref n)
}
EOF

assert_output=""
set +e
assert_output=$("$COMPILER_BIN" --run "$ASSERT_SRC" 2>&1)
assert_status=$?
set -e

if [ "$assert_status" -ne 2 ]; then
    echo "failed assert exited with $assert_status, expected 2"
    echo "$assert_output"
    exit 1
fi

if ! printf '%s\n' "$assert_output" | grep -q -- "error\\[R001\\]"; then
    echo "failed assert output does not include R001"
    echo "$assert_output"
    exit 1
fi

if ! printf '%s\n' "$assert_output" | grep -q -- "Assertion failed"; then
    echo "failed assert output does not include assertion message"
    echo "$assert_output"
    exit 1
fi

ASSERT_ESCAPED_SRC="$BIN_DIR/assert_runtime_escaped_\"path.nore"

cat > "$ASSERT_ESCAPED_SRC" <<'EOF'
func main(): void = {
    assert false
}
EOF

escaped_output=""
set +e
escaped_output=$("$COMPILER_BIN" --run "$ASSERT_ESCAPED_SRC" 2>&1)
escaped_status=$?
set -e

if [ "$escaped_status" -ne 2 ]; then
    echo "escaped-path assert exited with $escaped_status, expected 2"
    echo "$escaped_output"
    exit 1
fi

if ! printf '%s\n' "$escaped_output" | grep -Eq -- "$(basename "$ASSERT_ESCAPED_SRC"):[0-9]+:[0-9]+: error\\[R001\\]"; then
    echo "escaped-path assert output does not include source location"
    echo "$escaped_output"
    exit 1
fi
