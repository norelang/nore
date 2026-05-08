#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../../.." && pwd)"
COMPILER_BIN="${NORE_BIN:-$ROOT_DIR/norec}"
BIN_DIR="$ROOT_DIR/tmp/bootstrap/bins"

mkdir -p "$BIN_DIR"

run_case_file() {
    local name="$1"
    local expected_code="$2"
    local require_location="$3"
    local src="$4"
    local output=""
    local status=0

    cat > "$src"

    set +e
    output=$("$COMPILER_BIN" --run "$src" 2>&1)
    status=$?
    set -e

    if [ "$status" -ne 2 ]; then
        echo "$name exited with $status, expected 2"
        echo "$output"
        exit 1
    fi

    if ! printf '%s\n' "$output" | grep -q -- "error\\[$expected_code\\]"; then
        echo "$name output does not include $expected_code"
        echo "$output"
        exit 1
    fi

    if [ "$require_location" = "yes" ]; then
        if ! printf '%s\n' "$output" | grep -Eq -- "$(basename "$src"):[0-9]+:[0-9]+: error\\[$expected_code\\]"; then
            echo "$name output does not include source location"
            echo "$output"
            exit 1
        fi
    fi
}

run_case() {
    local name="$1"
    local expected_code="$2"
    local require_location="$3"
    local src="$BIN_DIR/bounds_${name}.nore"

    run_case_file "$name" "$expected_code" "$require_location" "$src"
}

run_success_case() {
    local name="$1"
    local src="$BIN_DIR/bounds_${name}.nore"
    local output=""
    local status=0

    cat > "$src"

    set +e
    output=$("$COMPILER_BIN" --run "$src" 2>&1)
    status=$?
    set -e

    if [ "$status" -ne 0 ]; then
        echo "$name exited with $status, expected 0"
        echo "$output"
        exit 1
    fi
}

run_case "array_read" "R002" "yes" <<'EOF'
func main(): void = {
    val xs: [i64; 2] = [1, 2]
    val n: i64 = xs[2]
    assert n == 0
}
EOF

run_case "array_write" "R002" "yes" <<'EOF'
func main(): void = {
    mut xs: [i64; 2] = [1, 2]
    xs[2] = 3
}
EOF

run_case "slice_read" "R002" "yes" <<'EOF'
func main(): void = {
    mut mem: Arena = arena(64)
    val xs: [i64] = arena_alloc(mut ref mem, 2)
    val n: i64 = xs[2]
    assert n == 0
}
EOF

run_case "slice_write" "R002" "yes" <<'EOF'
func main(): void = {
    mut mem: Arena = arena(64)
    mut xs: [i64] = arena_alloc(mut ref mem, 2)
    xs[2] = 7
}
EOF

run_case "string_index" "R002" "yes" <<'EOF'
func main(): void = {
    val text: str = "ab"
    val ch: u8 = text[2]
    assert ch == 'x'
}
EOF

run_case "table_column" "R002" "yes" <<'EOF'
table Pair {
    item: i64
}

func main(): void = {
    mut mem: Arena = arena(64)
    val pairs: Pair = table_alloc(mut ref mem, 1)
    val n: i64 = pairs.item[1]
    assert n == 0
}
EOF

run_case "table_get" "R002" "yes" <<'EOF'
table Pair {
    item: i64
}

func main(): void = {
    mut mem: Arena = arena(64)
    mut pairs: Pair = table_alloc(mut ref mem, 1)
    val row: Pair.Row = table_get(ref pairs, 0)
    assert row.item == 0
}
EOF

run_case_file "table_get_escaped_path" "R002" "yes" "$BIN_DIR/bounds_table_get_escaped_\"path.nore" <<'EOF'
table Pair {
    item: i64
}

func main(): void = {
    mut mem: Arena = arena(64)
    mut pairs: Pair = table_alloc(mut ref mem, 1)
    val row: Pair.Row = table_get(ref pairs, 0)
}
EOF

run_case "slice_closed" "R004" "yes" <<'EOF'
func main(): void = {
    val xs: [i64; 2] = [1, 2]
    val view: [i64] = xs[0..3]
    assert view.len == 0
}
EOF

run_case "slice_open" "R004" "yes" <<'EOF'
func main(): void = {
    mut mem: Arena = arena(64)
    val xs: [i64] = arena_alloc(mut ref mem, 2)
    val view: [i64] = xs[3..]
    assert view.len == 0
}
EOF

if [ "${COMPILER_TEST_MODE:-norec}" != "stage0" ]; then
    run_success_case "single_eval" <<'EOF'
func next_slice(mut ref mem: Arena, mut ref calls: i64): [i64] = {
    calls += 1
    arena_alloc(mut ref mem, 3)
}

func next_bound(mut ref calls: i64, n: i64): i64 = {
    calls += 1
    n
}

func main(): void = {
    mut mem: Arena = arena(256)
    mut calls: i64 = 0
    val n: i64 = next_slice(mut ref mem, mut ref calls)[1]
    assert calls == 1
    assert n == 0

    mut bound_calls: i64 = 0
    val data: [i64] = arena_alloc(mut ref mem, 4)
    val view: [i64] = data[..next_bound(mut ref bound_calls, 2)]
    assert bound_calls == 1
    assert view.len == 2

    mut both_calls: i64 = 0
    val view2: [i64] = next_slice(mut ref mem, mut ref both_calls)[next_bound(mut ref both_calls, 1)..next_bound(mut ref both_calls, 3)]
    assert both_calls == 3
    assert view2.len == 2
}
EOF
fi
