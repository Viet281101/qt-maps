# Internal real-data candidate operator notes

This archive is for owner-approved internal review on Ubuntu 26.04 x86-64.
It contains real Hanoi map/search/orthography data and the nationwide Vietnam
routing graph; it must not be uploaded to public CI or a public release.
Keep it on the local machine and hand it directly to the project owner only.
Verify the archive SHA-256 against the adjacent `SHA256SUMS` at handoff; do
not copy it to a shared drive or remote artifact service.
The position source remains simulated. The candidate has not passed the
reference-laptop benchmark/soak gate and is not v1.0.

Verify the adjacent `SHA256SUMS` before extraction. Read `manifest.json` and
`LICENSES/NOTICE`, then extract to a filesystem with enough space for both the
archive and an installed copy. With networking disabled, run
`bash install.sh install STAGE DEST` using the extracted `stage/` directory.
The installer verifies staged hashes and activates the map/search bundle and
the separate graph. On a suitable display, run `bash install.sh run DEST` and
verify vector, raster, permitted orthography, local search, a real route,
simulated replay, re-route and arrival. Retain the candidate manifest and
smoke result with the exact source commit. The Ubuntu 26.04 container smoke
checks the data and service path but does not replace GUI or performance proof.

Full VM and nationwide evidence is copied under `evidence/`. License texts
and draft notices are under `LICENSES/`; the exact runtime and transitive
notice review remains a release gate.
