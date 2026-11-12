#!/usr/bin/env python3

import stat
import sys
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools/benchmark"))

import run_benchmarks  # noqa: E402


class BenchmarkRuntimeEnvironmentTest(unittest.TestCase):
    def test_uses_qmake_reported_qt_runtime_not_stale_generated_runtime(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            maplibre = root / "generated/unit-01b/maplibre-install"
            for path in (maplibre / "lib", maplibre / "plugins", maplibre / "qml"):
                path.mkdir(parents=True)
            qt_prefix = root / "qt"
            archdata = qt_prefix / "archdata"
            plugins = qt_prefix / "plugins"
            (archdata / "qml").mkdir(parents=True)
            plugins.mkdir(parents=True)
            qmake = qt_prefix / "bin/qmake"
            qmake.parent.mkdir(parents=True)
            qmake.write_text(
                "#!/bin/sh\n"
                "case \"$2\" in\n"
                f"  QT_INSTALL_ARCHDATA) echo '{archdata}' ;;\n"
                f"  QT_INSTALL_PLUGINS) echo '{plugins}' ;;\n"
                "  *) exit 2 ;;\n"
                "esac\n",
                encoding="utf-8",
            )
            qmake.chmod(qmake.stat().st_mode | stat.S_IXUSR)

            environment = run_benchmarks.runtime_environment(root, qt_prefix, "xcb")

            self.assertEqual(environment["QT_QPA_PLATFORM"], "xcb")
            self.assertIn(str(plugins), environment["QT_PLUGIN_PATH"])
            self.assertIn(str(archdata / "qml"), environment["QML2_IMPORT_PATH"])
            self.assertNotIn("generated/unit-01b/qml-runtime", environment["QML2_IMPORT_PATH"])


if __name__ == "__main__":
    unittest.main()
