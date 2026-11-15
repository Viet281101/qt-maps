#!/usr/bin/env bash
set -euo pipefail

# Check and launch the bounded management-demo flow.  This is intentionally a
# functional rehearsal: it does not enable instrumentation or create benchmark
# output.

repository_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repository_root"

usage() {
    cat <<'EOF'
Usage: tools/rehearse-demo.sh [--check] [--help]

Validate the locally provisioned QtMaps demo payload and, unless --check is
given, launch the existing release demo through tools/run-demo.sh.

The application owns the loopback Valhalla service. Do not start a separate
service, change GPU configuration, or pass benchmark arguments for rehearsal.
See docs/rehearsal/demo-handoff.md for the operator checklist and fallbacks.
EOF
}

check_file() {
    local path="$1"
    local description="$2"
    [[ -f "$path" ]] || {
        printf 'QtMaps rehearsal error: missing %s: %s\n' "$description" "$path" >&2
        return 1
    }
}

check_executable() {
    local path="$1"
    local description="$2"
    [[ -x "$path" ]] || {
        printf 'QtMaps rehearsal error: %s is not executable: %s\n' "$description" "$path" >&2
        return 1
    }
}

check_only=false
case "${1:-}" in
    "") ;;
    --check) check_only=true ;;
    --help|-h) usage; exit 0 ;;
    *)
        usage >&2
        exit 2
        ;;
esac

app="${QTMAPS_APP:-$repository_root/build-qt515/qtmaps}"
vector_package="$repository_root/data/packages/current"
raster_package="$repository_root/data/raster-packages/current"
valhalla_service="$repository_root/generated/unit-04/valhalla-venv/lib/python3.12/site-packages/valhalla/bin/valhalla_service"
valhalla_config="$repository_root/generated/unit-04/valhalla/valhalla.json"

check_executable "$app" "release demo application"
check_file "$vector_package/manifest.json" "active vector manifest"
check_file "$vector_package/tiles.mbtiles" "active vector tiles"
check_file "$raster_package/manifest.json" "active raster manifest"
check_file "$raster_package/raster.mbtiles" "active raster tiles"
check_executable "$valhalla_service" "Valhalla service"
check_file "$valhalla_config" "Valhalla configuration"
"$repository_root/tools/run-demo.sh" --print-env >/dev/null

printf 'QtMaps rehearsal preflight passed.\n'
printf '  vector package: %s\n' "$(readlink -f "$vector_package")"
printf '  raster package: %s\n' "$(readlink -f "$raster_package")"
printf '  routing config: %s\n' "$valhalla_config"

if "$check_only"; then
    exit 0
fi

printf 'Launching functional rehearsal (not a benchmark).\n'
exec "$repository_root/tools/run-demo.sh" --disable-instrumentation
