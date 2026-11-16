# Unit 11 — Offline Readiness and Application Preferences Plan

## Status and tracking

**Implemented and verified locally on 2026-11-16; PR handoff pending.** This unit delivers
[issue #64](https://github.com/Viet281101/qt-maps/issues/64) after Unit 10 is
merged. It has three independently testable slices: package readiness,
Vietnamese/English application preferences, and consolidation of the idle map
control/status cluster. It is not a data pipeline, benchmark, packaging, or
distribution initiative.

## Objective

Before the local demo starts, show one concise offline-readiness result for the
installed vector, raster, search, and orthography packages. Also provide a
small app-owned settings control with a persisted Vietnamese/English language
choice. The result must identify package readiness and preserve each feature's
existing honest fallback without turning the map into a collection of idle
status labels.

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
- Introduce only Vietnamese and English application translations. Use Qt 5's
  translation runtime and source-controlled translation catalogs; do not add a
  third-party localization dependency or promise further languages.
- Store the selected language in the existing application configuration scope.
  The implementation must verify the Qt 5.15 retranslation path before claiming
  an immediate language switch; if a restart is needed, state that explicitly
  in the settings UI rather than partially translating the active screen.
- Add a 36 px or larger circular settings button at the map's upper-right safe
  edge. It opens a compact, keyboard-accessible settings panel whose first
  control is the language choice (`Tiếng Việt` / `English`). It uses an existing
  licensed/textual treatment; no new icon asset is implied.
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
3. **Translation base and settings.** Add the narrow language preference
   controller, Qt 5 translation catalog/build wiring, and language selector.
   Translate app-owned user-facing strings only; source attributions, product
   names, and raw package values retain their legally required forms.
4. **Control/status consolidation.** Keep the active layer as one interactive
   layer-selector control, not a duplicate passive label. In the normal ready
   state, move the duplicate `Valhalla (cục bộ)` floating pill to the route
   panel's existing readiness wording; show a map-level service status only for
   loading, degraded, or error recovery. Place the settings button above this
   compact action cluster without overlapping search, route, navigation, or
   attribution at 900x580.
5. **Verification.** Add focused valid/missing/corrupt/incompatible tests and
   one cancellation/timeout test. Test the two language choices, persistence,
   translation fallback, keyboard focus, and settings-panel dismissal. Run the
   Qt 5.15.18 Release review gate and local functional visual review at 900x580,
   1366x768, and 1920x1080.

## Acceptance criteria

- The operator can distinguish the required vector failure from an optional
  raster, search, or orthography absence before attempting a layer switch.
- A missing or invalid optional package does not stall the UI, silently fetch
  network data, or relabel another source as available.
- Existing layer, search, and routing fallback labels remain source-accurate.
- Every app-owned visible string covered by the selected scope has Vietnamese
  and English translations; missing translations fall back predictably and are
  testable.
- The upper-right settings button is reachable by keyboard, shows its purpose
  without an ambiguous icon-only interaction, and persists the selected
  language without persisting secrets or package paths.
- At idle, the map has one interactive layer control rather than two redundant
  floating labels. Service failures remain immediately visible and actionable.
- The preflight produces no generated artifacts in Git and adds no dependency.
- Automated and visual evidence covers all supported viewports; visual review
  is functional evidence only, never a performance claim.

## Deferred decisions

Whether the application should distribute or auto-provision its ignored local
packages remains out of scope and requires a separate owner decision covering
license, storage, update, and installation behavior.

Additional languages, locale-specific measurement/date formatting, voice
guidance language, and translation of third-party/map attribution are outside
this unit.
