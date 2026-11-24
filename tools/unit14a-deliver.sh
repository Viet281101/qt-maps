#!/usr/bin/env bash
set -euo pipefail

# Unit 14A: prepare on the Linux source; install on a clean, offline VM.
die() { printf 'Unit 14A: %s\n' "$*" >&2; exit 1; }
usage() {
    printf 'Usage:\n  %s prepare STAGE APP MAPLIBRE VALHALLA_SITE VALHALLA_CONFIG VECTOR RASTER SEARCH ORTHOGRAPHY_OR_DASH GRAPH_TILES [BUNDLE_ID [GRAPH_SOURCE_SHA256 [GRAPH_ID]]]\n  %s install STAGE DEST\n  %s verify-rollback OLDER_STAGE DEST\n  %s run DEST\n' "$0" "$0" "$0" "$0" >&2
    exit 2
}
[[ $# -ge 1 ]] || usage
mode="$1"
shift

case "$mode" in
prepare)
    [[ $# -ge 10 && $# -le 13 ]] || usage
    stage="$1"; app="$2"; maplibre="$3"; site="$4"; config="$5"
    vector="$6"; raster="$7"; search="$8"; ortho="$9"; graph="${10}"
    bundle_id="${11:-hanoi-unit14a}"
    graph_source_sha256="${12:-04d37da6a99d95364a5aecbbd643fa56467c7659bfee84bc0a6f7b9865e9ae73}"
    graph_id="${13:-hanoi-unit14a}"
    [[ "$bundle_id" =~ ^[a-zA-Z0-9._-]+$ ]] || die 'bundle ID must use letters, numbers, dots, underscores or hyphens'
    [[ "$graph_id" =~ ^[a-zA-Z0-9._-]+$ && "$graph_source_sha256" =~ ^[0-9a-f]{64}$ ]] || die 'graph ID or source SHA-256 is invalid'
    [[ ! -e "$stage" ]] || die "stage already exists: $stage"
    [[ -x "$app" && -d "$maplibre/lib" && -d "$maplibre/plugins" && -d "$maplibre/qml" ]] || die 'app/MapLibre input missing'
    [[ -x "$site/valhalla/bin/valhalla_service" && -d "$site/pyvalhalla.libs" && -f "$config" ]] || die 'Valhalla input missing'
    for directory in "$vector" "$raster" "$search" "$graph"; do
        [[ -d "$directory" ]] || die "data directory missing: $directory"
    done
    [[ "$ortho" == - || -d "$ortho" ]] || die "orthography directory missing: $ortho"
    [[ -f "$vector/manifest.json" && -f "$raster/manifest.json" && -f "$search/manifest.json" ]] || die 'data manifest missing'
    [[ "$ortho" == - || -f "$ortho/manifest.json" ]] || die 'orthography manifest missing'
    [[ -n "$(find "$graph" -type f -name '*.gph' -print -quit)" ]] || die 'graph has no tiles'
    mkdir -p "$stage/app" "$stage/runtime/valhalla/valhalla" "$stage/data/bundle" "$stage/data/graph" "$stage/metadata" "$stage/generated/unit-04/valhalla"
    cp -a "$app" "$stage/app/qtmaps"
    cp -aL "$maplibre" "$stage/runtime/maplibre"
    cp -a "$site/valhalla/bin" "$stage/runtime/valhalla/valhalla/bin"
    cp -a "$site/pyvalhalla.libs" "$stage/runtime/valhalla/pyvalhalla.libs"
    cp -a "$config" "$stage/generated/unit-04/valhalla/valhalla.json"
    cp -a "$vector" "$stage/data/bundle/vector"
    cp -a "$raster" "$stage/data/bundle/raster"
    cp -a "$search" "$stage/data/bundle/search"
    if [[ "$ortho" != - ]]; then cp -a "$ortho" "$stage/data/bundle/orthography"; fi
    cp -a "$graph" "$stage/data/graph/tiles"
    python3 - "$stage" "$bundle_id" "$graph_source_sha256" "$graph_id" <<'PY'
import hashlib, json, pathlib, sys
root = pathlib.Path(sys.argv[1])
bundle = root / 'data/bundle'
vector = json.loads((bundle / 'vector/manifest.json').read_text())
search = json.loads((bundle / 'search/manifest.json').read_text())
assert 'OpenStreetMap' in vector['attribution'] and 'ODbL' in vector['attribution']
assert 'OpenStreetMap' in search['attribution'] and 'ODbL' in search['attribution']
ortho_path = bundle / 'orthography/manifest.json'
ortho = json.loads(ortho_path.read_text()) if ortho_path.exists() else None
if ortho:
    assert 'Copernicus Sentinel' in ortho['attribution']
size = lambda p: sum(f.stat().st_size for f in p.rglob('*') if f.is_file())
components = dict(vector='vector', raster='raster', search='search')
if ortho:
    components['orthography'] = 'orthography'
source_name = ('redistributable synthetic CI fixture' if sys.argv[2].startswith('ci-')
               else 'owner-approved local Hanoi packages')
contract = dict(schema_version='qtmaps-local-map-search-bundle-v1',
                bundle_id=sys.argv[2], components=components,
                source=dict(name=source_name, retrieved_on=search['source']['retrieved_on']),
                attribution=vector['attribution'] + ('; ' + ortho['attribution'] if ortho else ''),
                installed_size_bytes=size(bundle))
(bundle / 'bundle.json').write_text(json.dumps(contract, indent=2) + '\n')
graph = root / 'data/graph'
graph_contract = dict(schema_version='qtmaps-routing-graph-activation-v1',
                      graph_id=sys.argv[4], graph_path='tiles',
                      source_pbf_sha256=sys.argv[3],
                      attribution=vector['attribution'], installed_size_bytes=size(graph))
(graph / 'routing.json').write_text(json.dumps(graph_contract, indent=2) + '\n')
entries = []
for path in sorted(root.rglob('*')):
    if path.is_symlink():
        raise SystemExit(f'unsafe stage symlink: {path}')
    if path.is_file() and path.relative_to(root).as_posix() != 'metadata/SHA256SUMS':
        entries.append(f'{hashlib.sha256(path.read_bytes()).hexdigest()}  {path.relative_to(root).as_posix()}')
(root / 'metadata/SHA256SUMS').write_text('\n'.join(entries) + '\n')
print('Hashed staged files:', len(entries))
PY
    du -sb "$stage"
    printf 'Prepared %s. Transfer only through the approved channel, then isolate VM networking.\n' "$stage"
    ;;
install)
    [[ $# -eq 2 ]] || usage
    stage="$(realpath -- "$1")"; dest="$(realpath -m -- "$2")"
    [[ -d "$stage" && -f "$stage/metadata/SHA256SUMS" ]] || die 'staging manifest missing'
    [[ ! -e "$dest" ]] || die "destination already exists; refusing overwrite: $dest"
    [[ -z "$(ip -4 route show default)" && -z "$(ip -6 route show default)" ]] || die 'VM still has a default route; disable outbound networking first'
    (cd "$stage" && sha256sum -c metadata/SHA256SUMS >/dev/null) || die 'stage checksum failed'
    [[ -z "$(find "$stage" -type l -print -quit)" ]] || die 'stage contains symlinks'
    needed="$(du -sb "$stage" | cut -f1)"
    available="$(df -B1 --output=avail "$(dirname "$dest")" | tail -n1 | tr -d ' ')"
    (( available > needed * 2 )) || die 'insufficient space for stage copy and rollback'
    cp -a "$stage" "$dest"
    (cd "$dest" && sha256sum -c metadata/SHA256SUMS >/dev/null) || die 'installed checksum failed'
    app="$dest/app/qtmaps"
    export LD_LIBRARY_PATH="$dest/runtime/maplibre/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
    export QT_PLUGIN_PATH="$dest/runtime/maplibre/plugins:/usr/lib/x86_64-linux-gnu/qt5/plugins"
    export QML2_IMPORT_PATH="$dest/runtime/maplibre/qml:/usr/lib/x86_64-linux-gnu/qt5/qml"
    export QT_QPA_PLATFORM=offscreen
    "$app" --install-local-bundle "$dest/data/bundle" --bundle-activation-root "$dest/activation/bundle"
    "$app" --activate-routing-graph "$dest/data/graph/routing.json" --routing-activation-root "$dest/activation/routing"
    "$app" --internal-resolve-startup --bundle-activation-root "$dest/activation/bundle" --routing-activation-root "$dest/activation/routing" > "$dest/metadata/startup.json"
    python3 - "$dest/metadata/startup.json" <<'PY'
import json, sys
o = json.load(open(sys.argv[1], encoding='utf-8'))
if not o.get('bundleValid') or not o.get('vectorOk'):
    raise SystemExit('Unit 14A: startup validation failed')
print('Startup metadata: bundle and vector valid; GUI/route smoke still required')
PY
    python3 - "$dest/data/bundle/search/places.sqlite" "$dest/data/bundle/search/manifest.json" <<'PY'
import json, sqlite3, sys
c = sqlite3.connect('file:' + sys.argv[1] + '?mode=ro', uri=True)
n = c.execute('select count(*) from places').fetchone()[0]
c.execute('select rowid from places_fts limit 1').fetchone()
expected = json.load(open(sys.argv[2], encoding='utf-8'))['places']['count']
if expected < 1 or n != expected:
    raise SystemExit(f'Unit 14A: search manifest expects {expected} rows, found {n}')
print('Local FTS5 search rows:', n)
PY
    # Exercise rollback in an isolated activation root without changing the active bundle.
    smoke="$dest/metadata/rollback-smoke"
    "$app" --install-local-bundle "$dest/data/bundle" --bundle-activation-root "$smoke/bundle"
    "$app" --install-local-bundle "$dest/data/bundle" --bundle-activation-root "$smoke/bundle"
    "$app" --rollback-local-bundle --bundle-activation-root "$smoke/bundle"
    "$app" --activate-routing-graph "$dest/data/graph/routing.json" --routing-activation-root "$smoke/routing"
    "$app" --activate-routing-graph "$dest/data/graph/routing.json" --routing-activation-root "$smoke/routing"
    "$app" --rollback-routing-graph --routing-activation-root "$smoke/routing"
    if "$app" --install-local-bundle "$dest/data/nonexistent" --bundle-activation-root "$smoke/failure" > "$dest/metadata/failure.log" 2>&1; then
        die 'invalid bundle unexpectedly activated'
    fi
    [[ ! -e "$smoke/failure/current" ]] || die 'invalid bundle altered activation root'
    printf 'Installed at %s. Run: %s run %s\n' "$dest" "$0" "$dest"
    ;;
verify-rollback)
    [[ $# -eq 2 ]] || usage
    older="$(realpath -- "$1")"; dest="$(realpath -- "$2")"
    [[ -f "$older/metadata/SHA256SUMS" && -x "$dest/app/qtmaps" ]] || die 'older stage or installed app missing'
    [[ -z "$(ip -4 route show default)" && -z "$(ip -6 route show default)" ]] || die 'target still has a default route'
    (cd "$older" && sha256sum -c metadata/SHA256SUMS >/dev/null) || die 'older stage checksum failed'
    python3 - "$older/data/bundle" "$dest/data/bundle" <<'PY'
import json, pathlib, sys
old, new = (pathlib.Path(p) for p in sys.argv[1:])
def read(root, name): return json.loads((root / name).read_text())
old_bundle, new_bundle = read(old, 'bundle.json'), read(new, 'bundle.json')
old_raster, new_raster = read(old, 'raster/manifest.json'), read(new, 'raster/manifest.json')
old_ortho, new_ortho = read(old, 'orthography/manifest.json'), read(new, 'orthography/manifest.json')
if old_bundle['bundle_id'] == new_bundle['bundle_id'] or old_raster['package_id'] == new_raster['package_id'] or old_ortho['package_id'] == new_ortho['package_id']:
    raise SystemExit('Unit 14A: rollback requires distinct bundle, raster and orthography versions')
print('Rollback versions:', old_bundle['bundle_id'], old_raster['package_id'], old_ortho['package_id'], '->', new_bundle['bundle_id'], new_raster['package_id'], new_ortho['package_id'])
PY
    proof="$dest/metadata/rollback-two-version"
    [[ ! -e "$proof" ]] || die "rollback proof already exists: $proof"
    export LD_LIBRARY_PATH="$dest/runtime/maplibre/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
    export QT_PLUGIN_PATH="$dest/runtime/maplibre/plugins:/usr/lib/x86_64-linux-gnu/qt5/plugins"
    export QML2_IMPORT_PATH="$dest/runtime/maplibre/qml:/usr/lib/x86_64-linux-gnu/qt5/qml"
    export QT_QPA_PLATFORM=offscreen
    app="$dest/app/qtmaps"
    "$app" --install-local-bundle "$older/data/bundle" --bundle-activation-root "$proof"
    [[ "$(realpath -- "$proof/current")" == "$older/data/bundle" ]] || die 'older version not active'
    "$app" --install-local-bundle "$dest/data/bundle" --bundle-activation-root "$proof"
    [[ "$(realpath -- "$proof/current")" == "$dest/data/bundle" ]] || die 'newer version not active'
    [[ "$(realpath -- "$proof/previous")" == "$older/data/bundle" ]] || die 'previous version not retained'
    "$app" --rollback-local-bundle --bundle-activation-root "$proof"
    [[ "$(realpath -- "$proof/current")" == "$older/data/bundle" ]] || die 'rollback did not restore older version'
    [[ "$(realpath -- "$proof/previous")" == "$dest/data/bundle" ]] || die 'rollback did not retain newer version'
    "$app" --internal-resolve-startup --bundle-activation-root "$proof" --routing-activation-root "$dest/activation/routing" > "$dest/metadata/rollback-two-version-startup.json"
    python3 - "$dest/metadata/rollback-two-version-startup.json" <<'PY'
import json, sys
x=json.load(open(sys.argv[1]))
if not x.get('bundleValid') or not x.get('vectorOk'):
    raise SystemExit('Unit 14A: rolled-back startup validation failed')
print('Distinct-version rollback and restart resolution passed')
PY
    ;;
run)
    [[ $# -eq 1 ]] || usage
    dest="$(realpath -- "$1")"; [[ -x "$dest/app/qtmaps" ]] || die 'installed app missing'
    [[ -z "$(ip -4 route show default)" && -z "$(ip -6 route show default)" ]] || die 'VM has a default route'
    export LIBGL_ALWAYS_SOFTWARE=1 GALLIUM_DRIVER=llvmpipe
    export LD_LIBRARY_PATH="$dest/runtime/maplibre/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
    export QT_PLUGIN_PATH="$dest/runtime/maplibre/plugins:/usr/lib/x86_64-linux-gnu/qt5/plugins"
    export QML2_IMPORT_PATH="$dest/runtime/maplibre/qml:/usr/lib/x86_64-linux-gnu/qt5/qml"
    cd "$dest"
    exec "$dest/app/qtmaps" --unit03-local-map --routing-demo --network-mode offline-local \
        --valhalla-service "$dest/runtime/valhalla/valhalla/bin/valhalla_service" \
        --bundle-activation-root "$dest/activation/bundle" \
        --routing-activation-root "$dest/activation/routing"
    ;;
*) usage ;;
esac
