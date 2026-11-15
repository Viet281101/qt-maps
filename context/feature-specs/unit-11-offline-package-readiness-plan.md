# Unit 11 — Offline Package Readiness Preflight Plan

## Status and tracking

**Planned; no runtime work started.** This unit delivers
[issue #64](https://github.com/Viet281101/qt-maps/issues/64) after Unit 10 is
merged. It is intentionally a bounded operator-readiness slice, not a data
pipeline, benchmark, packaging, or distribution initiative.

## Objective

Before the local demo starts, show one concise offline-readiness result for the
installed vector, raster, search, and orthography packages. The result must
identify which package is required, optional, available, missing, corrupt, or
incompatible, while preserving each feature's existing honest fallback.

## Fixed boundaries

- Reuse the existing package validators and their checksums; do not duplicate
  manifest schemas or introduce a second data-validation contract.
- Keep validation and filesystem/database work off the UI thread. The preflight
  has bounded completion and cancellation behavior and cannot block map start.
- Vector is required for the ordinary local-map start. Raster, search, and
  orthography remain optional and retain their current recovery/unavailable
  behavior.
- Present concise Vietnamese status and recovery guidance without exposing raw
  filesystem paths, source checksums, credentials, or implementation errors.
- No source download, dataset regeneration, runtime networking, new dependency,
  benchmark, soak, GPU/i915 work, or broad application packaging is in scope.

## Ordered slices

1. **Contract and model.** Define a small read-only readiness model owned by
   `src/core/` or the narrowest existing boundary. It records package role,
   required/optional status, and a normalized availability reason; it must not
   own layer switching or package data.
2. **Asynchronous validation adapter.** Invoke the existing vector, raster,
   search, and orthography validators through their appropriate worker paths.
   Report one bounded result per configured package and preserve normal startup
   if an optional package is unavailable.
3. **Presentation.** Add a compact preflight/status affordance to the existing
   map-first shell. It must be readable at 900x580, not hide attribution,
   route, navigation, or layer controls, and make a recovery action clear.
4. **Verification.** Add focused valid/missing/corrupt/incompatible tests and
   one cancellation/timeout test. Run the Qt 5.15.18 Release review gate and
   local functional visual review at 900x580, 1366x768, and 1920x1080.

## Acceptance criteria

- The operator can distinguish the required vector failure from an optional
  raster, search, or orthography absence before attempting a layer switch.
- A missing or invalid optional package does not stall the UI, silently fetch
  network data, or relabel another source as available.
- Existing layer, search, and routing fallback labels remain source-accurate.
- The preflight produces no generated artifacts in Git and adds no dependency.
- Automated and visual evidence covers all supported viewports; visual review
  is functional evidence only, never a performance claim.

## Deferred decisions

Whether the application should distribute or auto-provision its ignored local
packages remains out of scope and requires a separate owner decision covering
license, storage, update, and installation behavior.
