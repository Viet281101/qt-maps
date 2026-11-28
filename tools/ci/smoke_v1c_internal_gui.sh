#!/usr/bin/env bash
set -euo pipefail

# Run inside unit14c-gui.Dockerfile with --network none, the archive mounted
# read-only at /input and a local output directory mounted at /output.
[[ -z "$(ip -4 route show default)" && -z "$(ip -6 route show default)" ]]
[[ -d /input && -d /output ]]
bash /smoke-archive.sh > /output/archive-smoke.log 2>&1

Xvfb :99 -screen 0 1440x900x24 -nolisten tcp > /output/xvfb.log 2>&1 &
xvfb_pid=$!
trap 'kill "$xvfb_pid" 2>/dev/null || true' EXIT
sleep 2
export DISPLAY=:99 QT_QPA_PLATFORM=xcb LIBGL_ALWAYS_SOFTWARE=1 GALLIUM_DRIVER=llvmpipe
export LD_LIBRARY_PATH=/proof/installed/runtime/maplibre/lib
export QT_PLUGIN_PATH=/proof/installed/runtime/maplibre/plugins:/usr/lib/x86_64-linux-gnu/qt5/plugins
export QML2_IMPORT_PATH=/proof/installed/runtime/maplibre/qml:/usr/lib/x86_64-linux-gnu/qt5/qml
cd /proof/installed
/proof/installed/app/qtmaps --unit03-local-map --routing-demo --network-mode offline-local \
    --valhalla-service /proof/installed/runtime/valhalla/valhalla/bin/valhalla_service \
    --bundle-activation-root /proof/installed/activation/bundle \
    --routing-activation-root /proof/installed/activation/routing \
    > /output/app.log 2>&1 &
app_pid=$!
sleep 18
kill -0 "$app_pid"
import -display :99 -window root /output/gui.png
identify /output/gui.png
python3 - <<'PY'
from pathlib import Path
log = Path('/output/app.log').read_text()
for marker in ('selected: true', 'routing mapReady=true error=0',
               'Valhalla route applied:'):
    assert marker in log, f'missing GUI proof marker: {marker}'
print('Offline GUI selected active graph, rendered map and applied route')
PY
kill "$app_pid" 2>/dev/null || true
wait "$app_pid" 2>/dev/null || true
