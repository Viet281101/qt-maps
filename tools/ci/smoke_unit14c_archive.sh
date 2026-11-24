#!/usr/bin/env bash
set -euo pipefail

archive="${1:?archive path required}"
checksums="${2:?SHA256SUMS path required}"
[[ "$(getconf GNU_LIBC_VERSION)" == 'glibc 2.43' ]] || {
    echo 'Unit 14C: Ubuntu 26.04/glibc 2.43 userland required' >&2
    exit 1
}
[[ -z "$(ip -4 route show default)" && -z "$(ip -6 route show default)" ]] || {
    echo 'Unit 14C: smoke container has a default route' >&2
    exit 1
}
printf 'OS: '; . /etc/os-release; echo "$PRETTY_NAME"
printf 'Network: '; ip -br addr
printf 'Routes: '; ip -4 route; ip -6 route
cd /input
sha256sum -c "$checksums"
mkdir -p /proof/archive
tar --no-same-owner -xzf "$archive" -C /proof/archive
cd /proof/archive
test -f manifest.json
python3 - <<'PY'
import json
o = json.load(open('manifest.json'))
assert o['kind'] == 'synthetic-ci-fixture', o
assert o['target'] == 'ubuntu-26.04-x86_64', o
assert o['search_rows'] == 1, o
print('Artifact scope:', o['kind'], o['target'])
PY
test -f LICENSES/NOTICE
test -f LICENSES/QtMaps-MIT.txt
test -f LICENSES/pyvalhalla-COPYING
test -f stage/metadata/SHA256SUMS
cd stage
sha256sum -c metadata/SHA256SUMS >/dev/null
export LD_LIBRARY_PATH="$PWD/runtime/maplibre/lib"
for elf in app/qtmaps runtime/maplibre/lib/libQMapLibre.so.3.0.0 \
    runtime/maplibre/plugins/geoservices/libqtgeoservices_maplibre.so \
    runtime/valhalla/valhalla/bin/valhalla_service; do
    output="$(ldd "$elf" 2>&1)"
    if grep -Eq 'not found|version .* not found' <<< "$output"; then
        printf 'Unresolved dependencies for %s:\n%s\n' "$elf" "$output" >&2
        exit 1
    fi
done
printf 'ELF dependency closure: app, MapLibre plugin and service resolved\n'

bash ../install.sh install "$PWD" /proof/installed
test -f /proof/installed/metadata/startup.json
python3 - <<'PY'
import json
from pathlib import Path
root = Path('/proof/installed')
o = json.loads((root / 'metadata/startup.json').read_text())
assert o['bundleValid'] and o['vectorOk'], o
assert not o.get('orthography'), o
assert not (root / 'metadata/rollback-smoke/failure/current').exists()
print('Activation, optional orthography absence and controlled failure: passed')
PY

export QT_QPA_PLATFORM=offscreen
export QT_PLUGIN_PATH="/proof/installed/runtime/maplibre/plugins:/usr/lib/x86_64-linux-gnu/qt5/plugins"
export QML2_IMPORT_PATH="/proof/installed/runtime/maplibre/qml:/usr/lib/x86_64-linux-gnu/qt5/qml"
/proof/installed/app/qtmaps --internal-resolve-startup \
    --bundle-activation-root /proof/installed/activation/bundle \
    --routing-activation-root /proof/installed/activation/routing \
    > /proof/restart.json
python3 - <<'PY'
import json
o = json.load(open('/proof/restart.json'))
assert o['bundleValid'] and o['vectorOk'], o
print('Restart resolution: passed')
PY

python3 - <<'PY'
import json
from pathlib import Path
src = Path('/proof/installed/generated/unit-04/valhalla/valhalla.json')
o = json.loads(src.read_text())
o['mjolnir']['tile_dir'] = '/proof/installed/data/graph/tiles'
Path('/proof/route-config.json').write_text(json.dumps(o))
PY
/proof/installed/runtime/valhalla/valhalla/bin/valhalla_service \
    /proof/route-config.json route \
    '{"locations":[{"lat":21.025,"lon":105.845},{"lat":21.025,"lon":105.855}],"costing":"auto"}' \
    > /proof/route.json
python3 - <<'PY'
import json
o = json.load(open('/proof/route.json'))['trip']
assert o['summary']['length'] > 0
assert len(o['legs'][0]['maneuvers']) > 0
print('Synthetic graph route and maneuvers: passed')
PY
echo 'Unit 14C offline software smoke: passed; no GUI/GPU or performance claim'
