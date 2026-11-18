# Unit 13 local runtime handoff — 2026-11-17

## Scope and preparation

Functional verification used the existing owner-provisioned Hanoi vector,
raster, search packages and existing Valhalla tiles on the development machine.
Temporary bundle contracts, component links and activation roots were created
under ignored `generated/unit-13-functional-yb8b6ucb/`. The two test bundles
intentionally omitted optional orthography. No data was downloaded or generated,
and no routing graph, source dataset or source Valhalla configuration was changed.

## Operator workflow

1. Prepare a local bundle and independent graph contract using the Unit 12
   operator checklist; activate them with `--install-local-bundle` and
   `--activate-routing-graph`, specifying activation roots as needed.
2. Start `tools/run-demo.sh` with `--bundle-activation-root` and
   `--routing-activation-root` when using non-default roots. The default roots
   are `data/package-activation` and `data/routing-activation`, relative to the
   process working directory. No per-component path is required.
3. Check Settings: absent or invalid graph data is reported separately from an
   installed but unselected graph, a selected graph whose service is not ready,
   and a selected, available graph. Orthography is optional.
4. Explicit component CLI paths take precedence. An explicit `--valhalla-config`
   is passed through unchanged; a mismatch with the active graph is reported as
   installed but not selected. Without that override, the app creates a private,
   temporary loopback configuration with only `mjolnir.tile_dir` changed.

## Evidence and limits

The local provisioned-data test validated both bundles, activated them, resolved
component paths and explicitly rolled back to the first; it confirmed that no
orthography is required. An override run confirmed explicit map, search and
Valhalla paths win and that a graph mismatch is reported.

The offline headless run started the app from fresh activation roots. Inspection
of the actual Valhalla child process and its config showed that `tile_dir` was
the canonical activated graph and `tile_extract` was empty; the source config
SHA-256 was unchanged. Valhalla returned a 7,294 m route with 14 maneuvers and
HTTP 200. The temporary runtime config was removed when the application exited.
Valhalla logged warnings for absent optional tile/traffic extracts. The headless
software backend had no OpenGL context, so that run is not visual map evidence.
A separate direct-GPU functional review captured map and Settings frames at the
supported sizes; the 900x580 Settings label was fixed and visually rechecked in
`generated/unit-13-visual-final-20261117/`. This is not a benchmark and does not
replace a separate clean-target transfer test.

## Review fixes verified on 2026-11-18

Activation pointers are resolved to one stable bundle or graph snapshot before
metadata is read. Component consumers validate independently so a missing
search package does not replace the vector or raster paths. A cancellable helper
resolves startup metadata and prevalidates vector data; probe-only modes skip it,
and a 10-second timeout
fails closed. Synthetic tests run by default and cover bundle resolution and
rollback, concurrent pointer changes, CLI precedence, and blocked startup.
The blocked-I/O timeout and an unavailable search with a healthy local map were
also checked manually. A fresh local route still returned 7,294 m / 14 maneuvers.
The vector tile-source loader also has a bounded responsive startup. A synthetic
FIFO test checked its timeout, event processing, worker detachment and the rule
that raster fallback must not reread a vector package that timed out. Qt 5.15.18
Release tests passed 25/25, and the direct-GPU functional visual review captured
vector, raster, orthography and Settings frames in
`generated/unit-13-review-fixes-final-20261118/`; this is not a benchmark.

## Follow-up review checks

Synthetic integration tests verify that a valid explicit vector remains usable
when bundle metadata blocks, that the separately activated graph remains
selected, and that all four explicit component paths skip the unused bundle. A repeated vector-start timeout now clears the prior running
state and URL. Both tests exercise the real affected startup boundaries.

## Routing isolation review check

Synthetic tests exercise a graph contract blocked on a FIFO: fixture mode skips
graph I/O, while live-routing mode times out the graph helper without discarding
a healthy map bundle. The reverse case also verifies that a blocked bundle does
not discard a separately validated graph or explicit vector override. Routing
readiness distinguishes an unchecked graph from one that could not be checked.
Qt 5.15.18 Release tests passed 25/25, the real local route still returned
7,294 m / 14 maneuvers, and a direct-GPU functional review captured Settings
in `generated/unit-13-routing-isolation-20261118/`. No benchmark was run.
