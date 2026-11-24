#!/usr/bin/env python3
"""Build the small Qt Location/Positioning private-header overlay for MapLibre."""

import argparse
from pathlib import Path
import re
import shutil


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("qtlocation_source", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    source = args.qtlocation_source.resolve()
    output = args.output.resolve()
    if output.exists():
        parser.error("output already exists")
    include_root = output / "include"
    count = 0
    for module in ("QtLocation", "QtPositioning"):
        forwarding = source / "include" / module / "5.15.18" / module / "private"
        if not forwarding.is_dir():
            parser.error(f"private forwarding headers missing: {forwarding}")
        for header in sorted(forwarding.glob("*.h")):
            match = re.fullmatch(r'#include "([^"]+)"\s*', header.read_text())
            if not match:
                parser.error(f"unexpected forwarding header: {header}")
            target = (header.parent / match.group(1)).resolve()
            if source not in target.parents or not target.is_file():
                parser.error(f"header points outside Qt source: {header}")
            dest = include_root / module / "private" / header.name
            dest.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(target, dest)
            count += 1
    if count != 163:
        parser.error(f"Qt 5.15.18 private-header count changed: {count}")
    cmake = output / "qt5-private-targets.cmake"
    cmake.write_text(
        f'set(_qtmaps_qt_private_root "{include_root}")\n'
        'if(NOT TARGET Qt5::PositioningPrivate)\n'
        '    add_library(Qt5::PositioningPrivate INTERFACE IMPORTED)\n'
        '    set_target_properties(Qt5::PositioningPrivate PROPERTIES\n'
        '        INTERFACE_INCLUDE_DIRECTORIES "${_qtmaps_qt_private_root}"\n'
        '        INTERFACE_LINK_LIBRARIES "Qt5::Positioning;Qt5::CorePrivate")\n'
        'endif()\n'
        'if(NOT TARGET Qt5::LocationPrivate)\n'
        '    add_library(Qt5::LocationPrivate INTERFACE IMPORTED)\n'
        '    set_target_properties(Qt5::LocationPrivate PROPERTIES\n'
        '        INTERFACE_INCLUDE_DIRECTORIES "${_qtmaps_qt_private_root}"\n'
        '        INTERFACE_LINK_LIBRARIES\n'
        '            "Qt5::Location;Qt5::PositioningPrivate;Qt5::QuickPrivate;Qt5::QmlPrivate")\n'
        'endif()\n'
    )
    print(f"Qt 5.15.18 private overlay: {count} headers, {cmake}")


if __name__ == "__main__":
    main()
