#!/usr/bin/env bash
# Guarded, owner-triggered i915 bisect probe. Default action is dry-run.
set -euo pipefail

repository_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
mode="dry-run"
worktree=""
target=""
label=""
platform="xcb"

usage() {
    cat <<'EOF'
Usage: bash tools/i915-bisect-run.sh [--dry-run|--run] --worktree PATH --target SHA --label LABEL

Runs exactly one 60-second online navigation probe, then a 60-second cooldown.
--dry-run is the default and never builds or launches QtMaps.
--run requires generated/system/OWNER_APPROVED_GPU_RUN, a clean target worktree,
and one of the documented i915 bisect commits. It aborts on a new GPU HANG.
EOF
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --dry-run) mode="dry-run" ;;
        --run) mode="run" ;;
        --worktree) worktree="${2:?missing worktree}"; shift ;;
        --target) target="${2:?missing target}"; shift ;;
        --label) label="${2:?missing label}"; shift ;;
        --platform) platform="${2:?missing platform}"; shift ;;
        --help|-h) usage; exit 0 ;;
        *) printf 'unknown argument: %s\n' "$1" >&2; usage >&2; exit 64 ;;
    esac
    shift
done

[[ -n "$worktree" && -n "$target" && -n "$label" ]] || {
    usage >&2
    exit 64
}
[[ "$platform" == "xcb" || "$platform" == "wayland" ]] || {
    printf 'platform must be xcb or wayland\n' >&2
    exit 64
}

allowed_targets=(
    3f199cbe85786ca176d0cedc91639d3efed8e409
    2aa462978477c22f0055fa11a5668d8cc22d2bf2
    5aec728cc3cbabc25e39de4ee1adb2f110fcc550
    7df22ba982ee928050c541a0596f1ad02cb98996
    a2bd79b7dba281beebd8d23746024e693a62547c
    0c20b97d4cd4454590828094e0a771f9b20800f3
)
target_full="$(git -C "$repository_root" rev-parse "$target^{commit}")"
printf '%s\n' "${allowed_targets[@]}" | grep -qx "$target_full" || {
    printf 'target is not in the documented bisect set: %s\n' "$target_full" >&2
    exit 64
}

if [[ "$mode" == "dry-run" ]]; then
    cat <<EOF
DRY RUN ONLY — no build and no GPU process will start.
target=$target_full
worktree=$worktree
label=$label
platform=$platform
Would require: $repository_root/generated/system/OWNER_APPROVED_GPU_RUN
Would verify target worktree is clean and at target, build it, follow i915 logs,
run one 60-second navigation_replay/online probe after five-second warm-up,
then cooldown for 60 seconds. A line containing "GPU HANG" aborts immediately.
EOF
    exit 0
fi

approval="$repository_root/generated/system/OWNER_APPROVED_GPU_RUN"
[[ -f "$approval" ]] || {
    printf 'refusing GPU run without owner marker: %s\n' "$approval" >&2
    exit 77
}
[[ -d "$worktree/.git" || -f "$worktree/.git" ]] || {
    printf 'not a Git worktree: %s\n' "$worktree" >&2
    exit 64
}
[[ -z "$(git -C "$worktree" status --porcelain)" ]] || {
    printf 'refusing dirty worktree: %s\n' "$worktree" >&2
    exit 65
}
[[ "$(git -C "$worktree" rev-parse HEAD)" == "$target_full" ]] || {
    printf 'worktree HEAD does not match target\n' >&2
    exit 65
}

timestamp="$(date -u +%Y%m%d-%H%M%S)"
output="$repository_root/generated/benchmarks/i915-bisect-$timestamp-$label-$platform"
mkdir -p "$output"
log="$output/i915-kernel.log"
hang_marker="$output/GPU_HANG"
journalctl -kf -o cat 2>&1 | grep --line-buffered -iE 'i915|gpu hang|reset' >"$log" &
log_pid=$!
cleanup() { kill "$log_pid" 2>/dev/null || true; }
trap cleanup EXIT

make -C "$worktree" build
QT_QPA_PLATFORM="$platform" python3 "$worktree/tools/benchmark/run_benchmarks.py" \
    --app "$worktree/build-qt515/qtmaps" --skip-build --skip-overhead \
    --scenarios navigation_replay --mode online --repeat 1 --warmup 5 --duration 60 \
    --platform "$platform" --code-label "$label" --code-revision "$target_full" \
    --output "$output" &
run_pid=$!
while kill -0 "$run_pid" 2>/dev/null; do
    if grep -qi 'GPU HANG' "$log"; then
        : >"$hang_marker"
        kill "$run_pid" 2>/dev/null || true
        wait "$run_pid" || true
        printf 'GPU HANG seen; benchmark stopped: %s\n' "$output" >&2
        exit 70
    fi
    sleep 1
done
wait "$run_pid"
sleep 60
grep -qi 'GPU HANG' "$log" && {
    : >"$hang_marker"
    printf 'GPU HANG seen during cooldown: %s\n' "$output" >&2
    exit 70
}
printf 'probe completed without a new GPU HANG: %s\n' "$output"
