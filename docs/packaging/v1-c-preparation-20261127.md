# V1-C candidate preparation — 2026-11-27

The owner approved a public CI preview containing Qt/MapLibre/Valhalla runtime
with only synthetic fixture data, retained seven days under GitHub's artifact
limits. Real map/search/orthography data and the nationwide graph are limited
to local storage and direct handoff to the project owner only.
Neither artifact is a v1.0 release.

## Synthetic CI preview

The Unit 14C workflow now bundles an operator note, source-commit manifest,
SHA256SUMS, selected runtime license texts, and hashed copies of the 14A,
V1-A and V1-B reports with its synthetic executable stage. Its networkless
Ubuntu 26.04 smoke checks those hashes after extraction. The earlier text-only
provenance revision passed [run #45921465230](https://github.com/Viet281101/qt-maps/actions/runs/45921465230).
The text-provenance-only workflow also passed [run #45926277701](https://github.com/Viet281101/qt-maps/actions/runs/45926277701)
on commit `f6389b9`; subsequent commits only changed documentation.
The owner authorized a seven-day public binary preview, but its upload remains
gated by the exact runtime/transitive notice review. CI retains only text
provenance for seven days until that review is complete. The synthetic graph and
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
workspace. It has not been uploaded or transferred. At handoff, verify its
SHA-256 against the adjacent `SHA256SUMS` and deliver it directly to the owner;
do not use a shared drive or remote artifact service.

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
nationwide graph. A clean Desktop VM/physical-target GUI run from this exact candidate and
an exact transitive-runtime notice review remain before V1-C can close.

## Offline GUI proof from the exact archive — 2026-11-28

Using the same local archive and its checked SHA-256, a fresh Ubuntu 26.04
container extracted and installed the candidate with `--network none`,
`--cap-drop ALL`, a 6 GiB memory limit and llvmpipe/Xvfb. The added
`tools/ci/unit14c-gui.Dockerfile` and `tools/ci/smoke_v1c_internal_gui.sh`
reproduce this local-only run. The script passed archive, installer, FTS5 and
two real-route checks, then started the actual Qt Quick window. The log
reported the active national graph selected, map readiness without an error,
and a Valhalla route of 7,294 m with 14 maneuvers. Visual inspection of the
1440×900 screenshot confirmed the Hanoi vector map, blue route, guidance
panel, Valhalla readiness and OSM attribution. The ignored local screenshot
is `generated/v1-c-gui-smoke-20261128-verified/gui.png` in the main workspace;
SHA-256 `759cdffd79fcf95417bdb0e3713d441a2d95aeaf9ed221e0720b98dbac55537b`.
No data archive or screenshot was uploaded.

From the PR worktree, build `qtmaps-v1c-runtime` with
`tools/ci/unit14c-runtime.Dockerfile`, then build `qtmaps-v1c-gui` with
`tools/ci/unit14c-gui.Dockerfile`. Run the GUI image with the candidate
directory mounted read-only at `/input`, a local writable directory at
`/output`, and the three smoke scripts mounted at `/gui-smoke.sh`,
`/smoke-archive.sh` and `/smoke-stage.sh`. Use `--network none --cap-drop ALL
--memory 6g --shm-size 256m` and execute `bash /gui-smoke.sh`. Keep the output
directory local; with all capabilities dropped, it must be writable by the
container process.

This is clean-container functional GUI evidence with software rendering. It
does not replace a clean Desktop VM/physical-target run, reference-laptop
benchmark or soak. Exact transitive-runtime notices also remain open.
