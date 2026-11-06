# Unit 07 Integration PR Drafts

All PRs are local drafts only. Create each against the then-current `main`, link
the stated issue(s), set the active milestone, add it to Project #6 with the
matching labels, and use **Squash and merge only**. After every squash, rebase
the next local branch onto updated `main`, resolve only the affected boundary,
run the required test command, and force-push only after owner approval.

## Ordered stack

| Order | Source / issues | Draft title | Main conflict risk |
| --- | --- | --- | --- |
| 1 | 07A / #24–#27 | `chore: prepare repository structure for Unit 07` | repository layout and CI paths |
| 2 | 07B / #3 | `feat: add the Unit 07 demo shell UI` | QML resource list and top-level shell |
| 3 | 07C / #11 | `feat: add offline raster fallback` | map-source lifecycle and QML layer state |
| 4 | 07D style-only / #28, #29 | `style: refine Hanoi cartography without schema migration` | style JSON and style validation only |
| 5 | #30 + #32 | `feat: migrate Hanoi style and v2 package contract atomically` | builder, manifest validator, package schema |
| 6 | perf / Refs #7 | `perf: retain validated Unit 07 measurement instrumentation` | benchmark harness; never merge into feature PRs accidentally |

## Draft descriptions

### 07A — Refs #24, Refs #25, Refs #26, Refs #27

Prepare the Unit 07 repository structure and verification scaffolding. This is
non-product infrastructure; no generated map artifact is tracked.

Verification: run the branch's documented Release checks. On the current Qt
5.15.18 workstation use `make QT_EXPECTED_VERSION=5.15.18 test`; retain the
Qt 5.15.13 comparison job when the target branch CI requires it.

### 07B — Refs #3

Add the bounded, keyboard-accessible demo shell UI and explicit loading,
offline, and degraded states. It does not alter MapLibre packaging or data
contracts.

Verification: QML geometry/focus tests at 960x640 and 1366x768, plus the
required Qt test command. Rebase risk: `qml/resources.qrc`, `Main.qml`, Theme,
and shared controls.

### 07C — Refs #11

Add lazy offline raster fallback with bounded caches, package validation, and
visible provenance/attribution. Generated raster packages remain ignored.

Verification: lifecycle, rejection, and QML state tests; required Qt test
command. Rebase risk: map-source ownership and route-overlay lifetime.

### 07D style-only — Refs #28, Refs #29

Apply the approved cartographic style-only refinement. This PR does not change
the v1 package schema, generate fonts, or include #30/#32 migration work.

Verification: style-schema tests and the required Qt test command. Rebase
risk: only style inputs and validators; resolve against the v1 contract.

### Atomic #30 + #32 — Refs #30, Refs #32

Switch the Hanoi style and package migration contract as one atomic squash.
The builder, manifest validation, source style, and migration documentation
must land together; v1 remains the active rollback package until this merge.

Verification: regeneration/validation evidence plus the required Qt test
command. Do not split this PR or mix it with #31.

### Clean performance follow-up — Refs #7

Keep the validated performance instrumentation and evidence separate from
product/UI changes. Do not retune locks or claim the interrupted i915 matrix as
valid performance evidence.

Verification: benchmark unit tests and the required Qt test command; actual GPU
measurement resumes only after the i915 recovery gate.

## Rebase checklist

1. Fetch `origin/main`; inspect the next branch's merge-base and diff before rebase.
2. Rebase one local branch, resolve its listed boundary, and avoid importing the
   performance harness into a product branch.
3. Run `make QT_EXPECTED_VERSION=5.15.18 test` locally. Where CI still pins
   Qt 5.15.13, run its isolated comparison command rather than changing the
   project default.
4. Confirm `git diff --check`, no generated artifacts, correct issue links,
   milestone, Project #6 fields, and labels before opening the PR.
