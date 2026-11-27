# V1-C candidate preparation — 2026-11-27

The owner approved a public CI preview containing Qt/MapLibre/Valhalla runtime
with only synthetic fixture data, retained seven days under GitHub's artifact
limits. Real map/search/orthography data and the nationwide graph are limited
to internal transfer. The exact transfer channel/recipients remain to be named.
Neither artifact is a v1.0 release.

## Synthetic CI preview

The Unit 14C workflow now bundles an operator note, source-commit manifest,
SHA256SUMS, selected runtime license texts, and hashed copies of the 14A,
V1-A and V1-B reports with its synthetic executable stage. Its networkless
Ubuntu 26.04 smoke checks those hashes after extraction. The earlier text-only
provenance revision passed [run #45921465230](https://github.com/Viet281101/qt-maps/actions/runs/45921465230).
The seven-day binary-preview upload is in the subsequent hosted run; record
its result before considering that part accepted. The synthetic graph and
one-row search fixture do not prove real-data behavior.

## Internal real-data archive

`tools/unit14a-deliver.sh prepare` built an ignored 1,498,646,440-byte stage
from the PR Release executable, existing Hanoi vector/raster/search/Sentinel
packages and the approved nationwide graph. The stage contains 1,092 hashed
files; SHA-256 of its `metadata/SHA256SUMS` is
`72f5bb9784e0fc170b0042727ce7f6cbdc3e30153e51bf1ff930354dd6d2ecab`.
The application binary SHA-256 is
`215613412303bc5003f0975d9ce03cafb99927557435342011116b4c1f6ecf97`.

`tools/ci/package_v1c_internal_candidate.py` packaged the stage, operator
instructions, selected license texts, notices, hashed acceptance reports and
manifest into an ignored `.tar.zst` tied to commit `3016987`. The archive is
543,496,479 bytes with SHA-256
`26cb328c7b670c888e03ab71310acf82f4843d5591e335152db47e612fec3f45`.
It remains local at `generated/v1-c-internal-candidate-20261127/` in the main
workspace. It has not been uploaded or transferred.

`tools/ci/smoke_v1c_internal_archive.sh` verified the adjacent checksums,
extracted the archive inside Ubuntu 26.04, verified the embedded manifest,
report hashes, notice and app hash, then reused the Unit 14A installer with
network disabled. The container passed activation and restart, 9,094 FTS5
rows and `MATCH`, a 734-tile graph sourced from the approved PBF, and real
Valhalla routes Hanoi–Hai Phong (126.714 km, 33 maneuvers) and Thu Dau Mot–Di
An (17.905 km, 18 maneuvers). The final run used `--network none --cap-drop ALL`
and exited 0. Two earlier direct-stage container attempts failed because
`cp -a` could not preserve host-user ownership/permissions with all Linux
capabilities dropped; the direct-stage retry without capability dropping
passed. The extracted archive normalizes ownership and passed with all
capabilities dropped. This was an operator/container condition, not a route
or data failure.

The container smoke did not render GUI, demonstrate real-data imagery on a
clean target, or measure FPS/RAM/CPU. The earlier clean VM GUI proof used the
Hanoi graph; V1-B direct-rendering review on the development laptop used the
nationwide graph. A clean Ubuntu 26.04 GUI run from this exact candidate and
an exact transitive-runtime notice review remain before V1-C can close.
