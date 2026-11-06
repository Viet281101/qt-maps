# Progress Tracker

This is the operational snapshot. Historical outcomes and benchmark chronology
are archived in [progress-tracker-20261106.md](../docs/archive/progress-tracker-20261106.md).

## Current Phase

- The 2026-11-03 management-demo milestone has passed. Units 03--05 were
  squash-merged; Unit 07 is a local-only integration stack.
- Performance implementation is closed at `0c20b97`; no render-path or locked
  threshold change is authorised. Wayland remains the default.
- The active map package is v1. #30 and #32 must be integrated atomically;
  #31 remains deferred.

## Current Goal

Complete safe, test-only quality/design/documentation backlog and prepare the
07A--07E squash sequence. Do not push, create a PR, or change runtime, package,
kernel, Mesa, GRUB, or GPU behaviour without owner approval.

## Unit 07 Status

| Unit | Local branch / head | State |
| --- | --- | --- |
| 07A | `chore/07a-repo-structure-maintenance` / `cccfcb7` | ready for first rebase/integration PR |
| 07B | `feat/07b-demo-shell-ui` / `1ce45e4` | stacked after 07A |
| 07C | `feat/07c-offline-raster` / `a4227ba` | stacked after 07B |
| 07D | `feat/07d-hanoi-cartography` / `bd854fa` | style-only; keep migration separate |
| 07E | `unit-07e-verify` | verification, safe backlog, and integration preparation |

## Decisions Waiting for Owner

1. Whether xcb/XWayland should replace Wayland as the production default.
2. Local privileged GuC preflight and, only if nonzero, the reversible
   `i915.enable_guc=0` experiment.
3. Any restart of GPU benchmarking after the i915 stability gate.
4. Atomic merge decision for #30 plus #32; v1 remains the rollback package.

## i915 Blocker

Three hard reboots were attributed to i915 GPU hangs. The partial backend
matrix is invalid and must not be used as performance evidence. No GPU run,
sudo, package/GRUB/kernel/Mesa change, reboot, or suspend is allowed in the
safe backlog. Evidence, backup SHA-256, GuC preflight, rollback, and the gate
for a new matrix are in [i915-gpu-hang-report.md](../docs/benchmarks/i915-gpu-hang-report.md).
The owner-gated app bisect plan is
[i915-bisect-plan.md](../docs/benchmarks/i915-bisect-plan.md).

## Local-only Refs

- Remote `origin/main`: `e0a8086`; Unit 07 branches and tags are not pushed.
- Performance references: `perf/a-plus-hooks` `937f68a`,
  `perf/nav-head-probe` `0c20b97`, `perf/validity-demo-original` `905908e`.
- Tags: `demo-candidate-0211`, `demo-candidate-0211-qt51518-verified`, and
  `post-demo-perf-b`. Complete ref snapshot: `generated/backup/refs.txt`.

## Working Rules

- Per commit run `make QT_EXPECTED_VERSION=5.15.18 test`; Qt 5.15.13 remains
  an explicit comparison/CI configuration only.
- Preserve user-owned untracked paths `context/feature-specs/hanoi-cartography-style-upgrade.md`
  and `docs/map/`.
- Generated datasets, benchmark runs, and backups remain untracked. Save a
  verified Git bundle after each five commits.
- Latest verified backup: `generated/backup/qtmaps-20261106-143921.bundle`.

## Next Up

1. Safe Track A design assets and reusable UI icon component outside MapCanvas.
2. Test-only lifecycle, validation, QML geometry/focus coverage; then isolated
   non-render refactors and style commits.
3. Rebase/integrate 07A, then 07B, 07C, 07D style-only, atomic #30+#32, and
   clean performance work. Drafts: `docs/pr-drafts/07-integration-sequence.md`.

## Latest Verification

- `make QT_EXPECTED_VERSION=5.15.18 test` passes 13/13 after every current
  safe-backlog commit.
- Current safe-backlog commits: `998869c`, `4d1cd41`, `252622f`, `00c3294`,
  `8ede216`, and `d84ebc1`.
