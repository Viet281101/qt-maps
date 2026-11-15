# Unit 09 — Offline Search Package Integration

## Delivery mode

Issue #61 and PR #62 are the single integration delivery for this unit. Keep
each slice as a small local commit with its own test evidence, but do not open
another PR until this unit is accepted or a protected decision blocks it.

## Ordered slices

1. Package contract: build the checksummed, capped FTS5 SQLite package from
   prepared fixture records. **Complete in PR #62.**
2. Extractor: derive approved named-place records from the local Geofabrik PBF;
   no download or new dependency without approval.
3. Runtime: validate/load the immutable package off the UI thread, preserve the
   fixture search on failure, and expose truthful package state.
4. Query/UI: bounded normalized FTS5 search, explicit source labels, endpoint
   actions, and supported-viewport visual review.
5. Final review: run Release checks, verify generated data remains ignored,
   update tracker, and review the full integration diff.

## Guardrails

The package remains Hanoi-only, capped at 20 MiB, attributed to OpenStreetMap
under ODbL, and offline-only. Never claim fixture or synthetic records are
production coverage. Split into a new PR only for an owner decision, external
access, dependency installation, benchmark/GPU work, or an unrelated boundary.
