#!/usr/bin/env bash
set -euo pipefail

# Read-only inventory for Unit 14A. Nothing is copied or activated here.
usage() {
    printf 'Usage: %s APP VALHALLA_SERVICE MAP_BUNDLE GRAPH_CONTRACT QT_PREFIX MAPLIBRE_ROOT\n' "$0" >&2
    exit 2
}

[[ $# -eq 6 ]] || usage
app="$1"
service="$2"
bundle="$3"
graph="$4"
qt_prefix="$5"
maplibre="$6"

fail() { printf 'Unit 14A preflight: %s\n' "$1" >&2; exit 1; }
[[ -x "$app" ]] || fail "application is not executable: $app"
[[ -x "$service" ]] || fail "Valhalla service is not executable: $service"
[[ -d "$bundle" ]] || fail "map/search bundle directory is missing: $bundle"
[[ -f "$graph" ]] || fail "routing graph contract is missing: $graph"
[[ -x "$qt_prefix/bin/qmake" ]] || fail "qmake is missing: $qt_prefix/bin/qmake"
[[ -d "$maplibre/lib" && -d "$maplibre/plugins" && -d "$maplibre/qml" ]] || \
    fail "MapLibre runtime directories are incomplete: $maplibre"

qt_version="$("$qt_prefix/bin/qmake" -query QT_VERSION)"
[[ "$qt_version" == 5.15.18 ]] || fail "Qt 5.15.18 required; found $qt_version"
command -v ldd >/dev/null || fail 'ldd is required for ELF inventory'
command -v python3 >/dev/null || fail 'python3 is required for JSON inventory'

# Only inspect JSON shape here. Application validators remain authoritative.
python3 - "$bundle" "$graph" <<'PY'
import json
import pathlib
import sys

bundle, graph = map(pathlib.Path, sys.argv[1:])
if not any(bundle.glob('*.json')):
    raise SystemExit('Unit 14A preflight: bundle has no top-level JSON contract')
for path in list(bundle.glob('*.json')) + [graph]:
    with path.open(encoding='utf-8') as source:
        if not isinstance(json.load(source), dict):
            raise SystemExit(f'Unit 14A preflight: JSON object expected: {path}')
PY

printf 'Qt version: %s\n' "$qt_version"
printf 'Application: %s\nValhalla: %s\nBundle: %s\nGraph contract: %s\n' \
    "$app" "$service" "$bundle" "$graph"
printf 'MapLibre root: %s\n' "$maplibre"
printf 'ELF dependency inventory (unresolved entries fail):\n'

elfs=("$app" "$service")
while IFS= read -r -d '' library; do
    elfs+=("$library")
done < <(find "$maplibre/lib" "$maplibre/plugins" -type f -name '*.so*' -print0)
[[ ${#elfs[@]} -gt 2 ]] || fail 'MapLibre shared libraries/plugins not found'

for elf in "${elfs[@]}"; do
    printf '\n%s\n' "$elf"
    dependencies="$(LD_LIBRARY_PATH="$maplibre/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" ldd "$elf")" || \
        fail "ldd failed: $elf"
    printf '%s\n' "$dependencies"
    if [[ "$dependencies" == *'not found'* ]]; then
        fail "unresolved shared library: $elf"
    fi
done

printf '\nPreflight passed. No installation, activation, offline smoke, or GUI check was performed.\n'
