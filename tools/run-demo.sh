#!/usr/bin/env bash
set -euo pipefail

repository_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repository_root"

qt_prefix="${QTMAPS_QT_PREFIX:-/usr}"
qmake="$qt_prefix/bin/qmake"
expected_qt_version="${QTMAPS_QT_EXPECTED_VERSION:-5.15.18}"
maplibre_install="${QTMAPS_MAPLIBRE_INSTALL:-$repository_root/generated/unit-01b/maplibre-install-qt51518}"
app="${QTMAPS_APP:-$repository_root/build-qt515/qtmaps}"
service="$repository_root/generated/unit-04/valhalla-venv/lib/python3.12/site-packages/valhalla/bin/valhalla_service"
config="$repository_root/generated/unit-04/valhalla/valhalla.json"

fail() {
    printf 'QtMaps runtime error: %s\n' "$1" >&2
    exit 1
}

[[ -x "$qmake" ]] || fail "qmake is not executable under QTMAPS_QT_PREFIX: $qmake"
qt_version="$("$qmake" -query QT_VERSION)"
[[ "$qt_version" == "$expected_qt_version" ]] || \
    fail "expected Qt $expected_qt_version from $qmake, found $qt_version"
qml_runtime="$("$qmake" -query QT_INSTALL_ARCHDATA)"
qt_plugin_path="$("$qmake" -query QT_INSTALL_PLUGINS)"
[[ -d "$qml_runtime/qml" ]] || fail "Qt QML runtime is missing: $qml_runtime/qml"
[[ -d "$qt_plugin_path" ]] || fail "Qt plugin root is missing: $qt_plugin_path"
[[ -d "$maplibre_install/lib" && -d "$maplibre_install/plugins" && -d "$maplibre_install/qml" ]] || \
    fail "MapLibre install is incomplete: $maplibre_install"
maplibre_core="$maplibre_install/lib/libQMapLibre.so.3.0.0"
[[ -f "$maplibre_core" ]] || fail "MapLibre core is missing: $maplibre_core"
grep -Fxq "$expected_qt_version" < <(strings "$maplibre_core") || \
    fail "MapLibre payload does not declare Qt $expected_qt_version: $maplibre_core"

if [[ "${1:-}" == "--print-env" ]]; then
    printf 'QT_VERSION=%s\n' "$qt_version"
    printf 'QML_RUNTIME=%s\n' "$qml_runtime"
    printf 'QT_PLUGIN_PATH=%s\n' "$maplibre_install/plugins:$qt_plugin_path"
    printf 'QML2_IMPORT_PATH=%s\n' "$maplibre_install/qml:$qml_runtime/qml"
    printf 'LD_LIBRARY_PATH=%s\n' "$maplibre_install/lib"
    exit 0
fi

[[ -x "$app" ]] || fail "demo application is not built: $app"

export LD_LIBRARY_PATH="$maplibre_install/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export QT_PLUGIN_PATH="$maplibre_install/plugins:$qt_plugin_path${QT_PLUGIN_PATH:+:$QT_PLUGIN_PATH}"
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
