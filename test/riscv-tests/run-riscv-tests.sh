#!/usr/bin/env bash
# Builds and runs one suite of the vendored riscv-tests on raven.
#
#   test/riscv-tests/run-riscv-tests.sh                   all of rv32ui
#   test/riscv-tests/run-riscv-tests.sh add lw            only these
#   SUITE=rv32mi test/riscv-tests/run-riscv-tests.sh      all of rv32mi
#
# Each test is built against the upstream env/p and run on run_riscv_test, a
# bench around raven_core that watches tohost and exits with:
#   0                      pass
#   3..127, odd            test case (code >> 1) failed
#   255                    failed, test case unknown (number too large, or an
#                          unexpected trap garbled it): see the log
#   anything else          the bench itself failed (load error, ...)
#
# Each test's output goes to build/riscv-tests/<suite>/<name>.log.
# Choose the suite with SUITE=rv32ui|rv32mi (default rv32ui), the bench with
# RAVEN=path, the time limit with TIMEOUT=seconds.

set -uo pipefail

root=$(cd "$(dirname "$0")/../.." && pwd)
raven=${RAVEN:-$root/build/debug/bin/run_riscv_test}
timeout_s=${TIMEOUT:-10}
suite=${SUITE:-rv32ui}
src_dir=$root/third_party/riscv-tests/isa/$suite
out_dir=$root/build/riscv-tests/$suite

# tests that need something raven leaves out on purpose
case "$suite" in
    rv32ui)
        # fence_i needs Zifencei (self-modifying code), ma_data tests
        # misaligned accesses, which raven rejects by design
        skip=(fence_i ma_data)
        ;;
    rv32mi)
        # pmpaddr needs physical memory protection, out of scope
        skip=(pmpaddr)
        ;;
    *)
        echo "unknown suite $suite (expected rv32ui or rv32mi)" >&2
        exit 1
        ;;
esac

if [ ! -x "$raven" ]; then
    echo "test bench not found at $raven, build it first (make)" >&2
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

# colors only when stdout is a terminal, and never when NO_COLOR is set
if [ -t 1 ] && [ -z "${NO_COLOR:-}" ]; then
    c_pass=$'\e[32m'          # green: the test passed
    c_fail=$'\e[31m'          # red: raven ran it and a check failed
    c_error=$'\e[1;35m'       # bold magenta: build or raven itself broke
    c_skip=$'\e[2m'           # dim
    c_reset=$'\e[0m'
else
    c_pass='' c_fail='' c_error='' c_skip='' c_reset=''
fi

# report STATUS NAME [DETAIL]
report() {
    local color
    case "$1" in
        PASS) color=$c_pass ;;
        FAIL) color=$c_fail ;;
        ERROR) color=$c_error ;;
        *) color=$c_skip ;;
    esac
    # pad before coloring so the escape codes don't break the alignment
    printf '%s%-5s%s %s%s\n' "$color" "$1" "$c_reset" "$2" "${3:+ ($3)}"
}

mkdir -p "$out_dir"
pass=0 fail=0 error=0 skipped=0

for name in "${names[@]}"; do
    if [[ " ${skip[*]} " == *" $name "* ]]; then
        report SKIP "$name"
        skipped=$((skipped + 1))
        continue
    fi

    bin=$out_dir/$name.bin
    log=$out_dir/$name.log
    rel_log=${log#"$root"/}

    if ! "$root/test/riscv-tests/compile.sh" --flat "$src_dir/$name.S" "$bin" \
        >"$log" 2>&1; then
        report ERROR "$name" "build failed, see $rel_log"
        error=$((error + 1))
        continue
    fi

    run_with_timeout "$raven" --flat "$bin" >"$log" 2>&1
    code=$?

    if [ $code -eq 0 ]; then
        report PASS "$name"
        pass=$((pass + 1))
    elif [ $code -ge 3 ] && [ $code -lt 128 ] && [ $((code % 2)) -eq 1 ]; then
        report FAIL "$name" "test case $((code >> 1)), see $rel_log"
        fail=$((fail + 1))
    elif [ $code -eq 255 ]; then
        report FAIL "$name" "test case unknown, see $rel_log"
        fail=$((fail + 1))
    elif [ $code -eq 137 ]; then
        report ERROR "$name" "killed after ${timeout_s}s, see $rel_log"
        error=$((error + 1))
    else
        report ERROR "$name" "bench exited $code, see $rel_log"
        error=$((error + 1))
    fi
done

echo
printf '%s%d passed%s, %s%d failed%s, %s%d errors%s, %d skipped\n' \
    "$c_pass" "$pass" "$c_reset" "$c_fail" "$fail" "$c_reset" \
    "$c_error" "$error" "$c_reset" "$skipped"
[ $fail -eq 0 ] && [ $error -eq 0 ]
