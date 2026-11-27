#!/usr/bin/env bash
set -euo pipefail

# Run only inside the Ubuntu 26.04 container with --network none. The real
# owner-provisioned stage is bind-mounted read-only and never uploaded to CI.
stage="${1:-/input/stage}"
installer="${2:-/input/install.sh}"
[[ "$(getconf GNU_LIBC_VERSION)" == 'glibc 2.43' ]]
[[ -z "$(ip -4 route show default)" && -z "$(ip -6 route show default)" ]]
cd "$stage"
sha256sum -c metadata/SHA256SUMS >/dev/null
bash "$installer" install "$stage" /proof/installed

export LD_LIBRARY_PATH=/proof/installed/runtime/maplibre/lib
export QT_PLUGIN_PATH=/proof/installed/runtime/maplibre/plugins:/usr/lib/x86_64-linux-gnu/qt5/plugins
export QML2_IMPORT_PATH=/proof/installed/runtime/maplibre/qml:/usr/lib/x86_64-linux-gnu/qt5/qml
export QT_QPA_PLATFORM=offscreen
/proof/installed/app/qtmaps --internal-resolve-startup \
    --bundle-activation-root /proof/installed/activation/bundle \
    --routing-activation-root /proof/installed/activation/routing \
    >/proof/installed/metadata/restart.json

python3 - <<'PY'
import json
import pathlib
import sqlite3
import subprocess

root = pathlib.Path('/proof/installed')
startup = json.loads((root / 'metadata/restart.json').read_text())
assert startup['bundleValid'] and startup['vectorOk'], startup
contract = json.loads((root / 'data/graph/routing.json').read_text())
assert contract['source_pbf_sha256'] == '4e5243c3116dc089a298f4cd7218e1257adf4b48355e42936360df5ce3200d43'
assert len(list((root / 'data/graph/tiles').rglob('*.gph'))) == 734
with sqlite3.connect(f'file:{root}/data/bundle/search/places.sqlite?mode=ro', uri=True) as db:
    assert db.execute('select count(*) from places').fetchone()[0] == 9094

config = json.loads((root / 'generated/unit-04/valhalla/valhalla.json').read_text())
config['mjolnir']['tile_dir'] = str(root / 'data/graph/tiles')
config_path = pathlib.Path('/proof/real-route-config.json')
config_path.write_text(json.dumps(config))
service = root / 'runtime/valhalla/valhalla/bin/valhalla_service'
for name, origin, destination in (
    ('hanoi-haiphong', (21.0278, 105.8342), (20.8449, 106.6881)),
    ('thudaumot-dian', (10.9804, 106.6519), (10.9077, 106.7695)),
):
    request = {'locations': [{'lat': origin[0], 'lon': origin[1]},
                             {'lat': destination[0], 'lon': destination[1]}],
               'costing': 'auto'}
    result = subprocess.run([str(service), str(config_path), 'route', json.dumps(request)],
                            check=True, capture_output=True, text=True, timeout=120)
    trip = json.loads(result.stdout)['trip']
    assert trip['summary']['length'] > 0
    assert trip['legs'][0]['shape'] and trip['legs'][0]['maneuvers']
    print(name, trip['summary']['length'], len(trip['legs'][0]['maneuvers']))
print('Real stage offline container smoke: passed; GUI and performance not tested')
PY
