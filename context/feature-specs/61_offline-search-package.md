# Issue 61 — Offline FTS5 Search Package

## Approved decision

Build an ignored immutable Hanoi search package from prepared place records
whose lineage is the already approved local Geofabrik Vietnam OSM PBF. The
package is limited to `105.78,20.97,105.90,21.10`, has a 20 MiB storage cap,
and retains `© OpenStreetMap contributors, ODbL 1.0` attribution. The source
URL is `https://download.geofabrik.de/asia/vietnam-latest.osm.pbf`; a real run
must record its local PBF SHA-256 and actual retrieval date in the manifest.

## This increment

Create a deterministic offline packager from compact prepared records, plus a
synthetic fixture test. It validates IDs, names, coordinates, bounds, FTS5
schema, provenance, package checksum, and the size cap. It neither downloads
nor parses a PBF, commits generated databases, replaces the existing fixture
search, or adds remote geocoding.

## Contract

The ignored output is
`data/search-packages/hanoi-search-<yyyymmdd>-v1-r*/` with `places.sqlite`, a
checksummed manifest, and `current` pointer. Prepared records carry a stable
source ID, display name, aliases, category, latitude, and longitude. The
manifest records the source URL/PBF checksum/retrieval date, ODbL attribution,
bounds, 20 MiB cap, database checksum, schema version, and generator revision.

## Deferred

The OSM PBF extractor, source-field selection, address coverage, runtime
validator/worker, query controller replacement, UI labels, and visual review
are separate slices. A missing or invalid package must leave the current
fixture search active and visible as fixture-only.
