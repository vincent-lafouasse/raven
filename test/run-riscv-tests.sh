#!/usr/bin/env bash
# Builds and runs the vendored rv32ui riscv-tests on raven.
#
#   test/run-riscv-tests.sh            run every test
#   test/run-riscv-tests.sh add lw     run only these
#
# A test exits through `ecall` with a7 = 93, and raven exits with a0:
#   0                      pass
#   odd, >= 3              test case (code >> 1) failed
#   anything else          raven itself failed (decode error, fault, ...)
# raven's own errors use exit code 1, which a test can never produce.
#
# Each test's raven output goes to build/riscv-tests/<name>.log.
# Override the emulator with RAVEN=path, the time limit with TIMEOUT=seconds.

set -uo pipefail

root=$(cd "$(dirname "$0")/.." && pwd)
raven=${RAVEN:-$root/build/debug/bin/raven}
timeout_s=${TIMEOUT:-10}
src_dir=$root/third_party/riscv-tests/isa/rv32ui
out_dir=$root/build/riscv-tests

# fence_i needs Zifencei (self-modifying code), ma_data tests misaligned
# accesses, which raven rejects by design
skip=(fence_i ma_data)

if [ ! -x "$raven" ]; then
    echo "raven not found at $raven, build it first (make)" >&2
    exit 1
fi

if [ $# -gt 0 ]; then
    names=("$@")
else
    names=()
    for f in "$src_dir"/*.S; do
        names+=("$(basename "$f" .S)")
    done
fi

# runs "$@" for at most $timeout_s seconds; macOS has no `timeout`
run_with_timeout() {
    "$@" &
    local pid=$!
    ( sleep "$timeout_s" && kill -9 "$pid" 2>/dev/null ) &
    local watcher=$!
    wait "$pid"
    local status=$?
    kill "$watcher" 2>/dev/null
    wait "$watcher" 2>/dev/null
    return $status
}

mkdir -p "$out_dir"
pass=0 fail=0 error=0 skipped=0

for name in "${names[@]}"; do
    if [[ " ${skip[*]} " == *" $name "* ]]; then
        printf '%-8s %s\n' SKIP "$name"
        skipped=$((skipped + 1))
        continue
    fi

    bin=$out_dir/$name.bin
    log=$out_dir/$name.log
    rel_log=${log#"$root"/}

    if ! "$root/test/compile.sh" --flat "$src_dir/$name.S" "$bin" \
        >"$log" 2>&1; then
        printf '%-8s %s (build failed, see %s)\n' ERROR "$name" "$rel_log"
        error=$((error + 1))
        continue
    fi

    run_with_timeout "$raven" --flat "$bin" >"$log" 2>&1
    code=$?

    if [ $code -eq 0 ]; then
        printf '%-8s %s\n' PASS "$name"
        pass=$((pass + 1))
    elif [ $code -ge 3 ] && [ $code -lt 128 ] && [ $((code % 2)) -eq 1 ]; then
        printf '%-8s %s (test case %d, see %s)\n' FAIL "$name" \
            $((code >> 1)) "$rel_log"
        fail=$((fail + 1))
    elif [ $code -eq 137 ]; then
        printf '%-8s %s (killed after %ss, see %s)\n' ERROR "$name" \
            "$timeout_s" "$rel_log"
        error=$((error + 1))
    else
        printf '%-8s %s (raven exited %d, see %s)\n' ERROR "$name" \
            "$code" "$rel_log"
        error=$((error + 1))
    fi
done

echo
echo "$pass passed, $fail failed, $error errors, $skipped skipped"
[ $fail -eq 0 ] && [ $error -eq 0 ]
