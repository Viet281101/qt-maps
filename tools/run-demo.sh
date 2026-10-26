#!/usr/bin/env bash
set -euo pipefail

repository_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repository_root"

qml_runtime="$repository_root/generated/unit-01b/qml-runtime/usr/lib/x86_64-linux-gnu/qt5"
maplibre_install="$repository_root/generated/unit-01b/maplibre-install"
app="${QTMAPS_APP:-$repository_root/build-qt515/qtmaps}"
service="$repository_root/generated/unit-04/valhalla-venv/lib/python3.12/site-packages/valhalla/bin/valhalla_service"
config="$repository_root/generated/unit-04/valhalla/valhalla.json"

if [[ ! -x "$app" ]]; then
    printf 'QtMaps demo is not built: %s\n' "$app" >&2
    exit 1
fi

export LD_LIBRARY_PATH="$maplibre_install/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export QT_PLUGIN_PATH="$maplibre_install/plugins:$qml_runtime/plugins${QT_PLUGIN_PATH:+:$QT_PLUGIN_PATH}"
export QML2_IMPORT_PATH="$maplibre_install/qml:$qml_runtime/qml${QML2_IMPORT_PATH:+:$QML2_IMPORT_PATH}"
export QSG_RENDER_LOOP="${QSG_RENDER_LOOP:-threaded}"

command=(
    "$app"
    --unit03-local-map
    --routing-demo
    --map-package "$repository_root/data/packages/current"
    --valhalla-service "$service"
    --valhalla-config "$config"
    "$@"
)

if [[ "${QTMAPS_GDB:-0}" == "1" ]]; then
    debugger="${QTMAPS_GDB_BIN:-gdb}"
    command -v "$debugger" >/dev/null || {
        printf 'QtMaps debugger is not available: %s\n' "$debugger" >&2
        exit 1
    }
    exec "$debugger" -ex run --args "${command[@]}"
fi

exec "${command[@]}"
