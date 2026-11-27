# Linux v1.0 operator handoff (candidate review)

Target: Ubuntu 26.04 x86-64, Qt 5.15.18. The CI archive currently contains a
**synthetic fixture** for offline packaging checks; it is not the v1.0 release
and cannot establish nationwide or GUI acceptance. Do not distribute the
real OSM/Sentinel packages or graph until the internal channel and recipients
are settled. Runtime redistribution still needs a complete notice inventory
for the exact packaged binaries before any v1.0 release claim.

The owner approved a public synthetic CI preview containing the runtime,
retained for seven days under GitHub's default artifact limits. Binary upload
waits for an exact runtime notice inventory; until then CI retains only text
manifest and hashes for seven days. This approval does not include real
map/search/imagery or routing graph data. Those use a separate internal
transfer.

For the CI fixture, verify the adjacent `SHA256SUMS`, extract the archive, read
`manifest.json` and `LICENSES/NOTICE`, then use `install.sh install STAGE DEST`
on a clean Ubuntu 26.04 target with networking disabled. The installer checks
the staged hashes, activates bundle and graph pointers, and rejects invalid
inputs. The CI job exercises this inside a `--network none` container and
checks restart, FTS5 search, ELF closure and a synthetic Valhalla route.

A real-data handoff additionally needs the separately authorized package
transfer, its checksums/provenance, and the clean-target GUI checklist in
`docs/rehearsal/unit-14a-7480-vm-results.md`. The nationwide graph and
navigation evidence is in `docs/routing/v1-a-national-graph-20261126.md` and
`docs/routing/v1-b-real-route-replay-20261126.md`. A final v1.0 handoff must
add the reference-laptop benchmark and soak report after the i915 freeze is
explicitly lifted, then record every gate against the tagged commit.

Known product limits: Hanoi-bounded detailed map/search/orthography, simulated
position rather than live GNSS, and Linux-only v1.0. A missing optional
orthography package must be shown as unavailable. No Windows binary is part
of this handoff.
