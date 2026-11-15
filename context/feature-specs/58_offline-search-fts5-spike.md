# Issue 58 — Offline SQLite FTS5 Search Spike

## Status

**Proposed; capability spike in progress.** This work proves only that the
Qt 5.15.18 `QSQLITE` driver can create and query the bounded fixture schema.
It does not replace the existing three-place fixture search or claim broader
offline place coverage.

## Goal and boundaries

The eventual search package is an ignored, immutable local SQLite database at
`data/search-packages/hanoi-search-<yyyymmdd>-v1-r*/places.sqlite`, accompanied
by a checksummed manifest and an ignored `current` pointer. It has no network
client, service, cache, or runtime download path. A missing, corrupt, or
incompatible package must leave the existing fixture search active with a
truthful unavailable message; it must not silently call remote geocoding.

The package generator and real place/address source selection remain separate
work. Before they start, the source URL, license/attribution, retrieval date,
coverage, source checksum, and generator revision must be approved and recorded
in the manifest. Generated databases never enter Git.

## Proposed schema and query contract

The package has one canonical `places` table with a stable source identifier,
display name, aliases, category, latitude, longitude, and source provenance.
Its content-synchronised `places_fts` FTS5 table indexes `name` and `aliases`
using `unicode61 remove_diacritics 2`. The schema version, data version,
coverage, source metadata, database SHA-256, and generator revision are in the
manifest.

The future runtime normalizes input to Unicode NFC, case-folds it, splits it
into Unicode letter/number tokens, and binds the resulting safe FTS expression
instead of interpolating raw user text. Empty input is idle; input without
tokens is empty. A bounded query returns at most ten results ordered by
`bm25(places_fts, 10.0, 3.0)`, then a stable source identifier. The exact
ranking weights and result fields remain proposed until a real licensed source
and representative fixture are available.

## Spike acceptance

1. The deployed Qt `QSQLITE` plugin creates the declared FTS5 table in a
   temporary database and executes a bound Vietnamese fixture query.
2. The query matches `Hoàn Kiếm` for the normalized `hoan kiem` input and has
   a deterministic order for tied fixtures.
3. The test uses no generated package, network request, or production search
   controller change.
4. If FTS5 is unavailable, the test fails with the SQLite driver error and the
   proposed technology remains blocked rather than falling back silently.

## Next decision

After the spike passes, select a licensed local source and set explicit
coverage/storage limits before implementing the generator, package validation,
or runtime switch. The current fixture search stays the only supported product
search until that separate slice is accepted.
