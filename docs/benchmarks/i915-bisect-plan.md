# i915 App-side Bisect Plan — 2026-11-06

## Scope and stop rule

This is a **future, owner-authorised GPU experiment**, not evidence that a
particular QtMaps change caused the i915 hang. Do not run it until the system
preflight in [the i915 hang report](i915-gpu-hang-report.md) is complete and
the owner creates `generated/system/OWNER_APPROVED_GPU_RUN`.

The plan isolates the render commits without changing their code. Every target
uses a clean, detached worktree, a five-second post-exposure warm-up, one
60-second `navigation_replay`/online xcb run, and a 60-second cooldown. Follow
`journalctl -kf -o cat | grep -iE 'i915|gpu hang|reset'` throughout. On the
first new `GPU HANG`, stop, retain the output, and make no further parameter or
rendering change without owner direction.

## Target pairs

| Question | Baseline | Candidate | Reason |
| --- | --- | --- | --- |
| Does threaded Canvas FBO introduce the symptom? | `3f199cbe` | `2aa4629` | `2aa4629` adds the threaded FBO route overlay |
| Does zoom-route caching change it? | `5aec728` | `7df22ba` | `7df22ba` adds cached simplified display routes |
| Does the retained revert head change it? | `a2bd79b` | `0c20b97` | `0c20b97` is the revert retained as performance HEAD |

`937f68a` is explicitly excluded from frame-rate and stability comparison: it
limits the old A+hooks instrumentation to liveness tracing and produced the
known anomalous ~1 FPS probe. If a full-instrumentation A reference is needed,
identify or reconstruct it in a separate review worktree before this plan;
never silently substitute `937f68a`.

## Guarded helper

`tools/i915-bisect-run.sh` defaults to dry-run and only accepts the six commits
in the table. It does not check out commits, mutate GRUB, use sudo, or create
the owner marker. Example review of a planned probe:

```bash
bash tools/i915-bisect-run.sh --dry-run \
  --worktree /path/to/qtmaps-2aa4629 \
  --target 2aa4629 --label threaded-fbo --platform xcb
```

The only execution form is intentionally explicit:

```bash
touch generated/system/OWNER_APPROVED_GPU_RUN
bash tools/i915-bisect-run.sh --run \
  --worktree /path/to/qtmaps-2aa4629 \
  --target 2aa4629 --label threaded-fbo --platform xcb
```

It refuses a missing marker, dirty worktree, mismatched target, or unlisted
commit. The marker must be removed by the owner after the approved run window.
Generated output is isolated under `generated/benchmarks/i915-bisect-*`.

## Interpretation

One 60-second run is only a failure-localisation probe, not a performance
result. Absence of a hang is not proof of safety. Compare only targets run with
the same kernel, Mesa, i915 parameters, backend, power state, and workload;
do not mix these results into the Unit 07 performance matrix.
