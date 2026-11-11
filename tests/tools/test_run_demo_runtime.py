#!/usr/bin/env python3
"""Validate the non-GPU runtime selection path of tools/run-demo.sh."""

import os
import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
LAUNCHER = ROOT / "tools" / "run-demo.sh"


def make_qt_prefix(root: Path, version: str) -> Path:
    prefix = root / "qt"
    qml_root = prefix / "runtime"
    plugin_root = prefix / "plugins"
    (qml_root / "qml").mkdir(parents=True)
    plugin_root.mkdir()
    qmake = prefix / "bin" / "qmake"
    qmake.parent.mkdir()
    qmake.write_text(
        "#!/usr/bin/env bash\n"
        "case \"$2\" in\n"
        f"  QT_VERSION) echo {version} ;;\n"
        f"  QT_INSTALL_ARCHDATA) echo {qml_root} ;;\n"
        f"  QT_INSTALL_PLUGINS) echo {plugin_root} ;;\n"
        "  *) exit 2 ;;\n"
        "esac\n",
        encoding="utf-8",
    )
    qmake.chmod(0o755)
    return prefix


def make_maplibre(root: Path, version: str) -> Path:
    install = root / "maplibre"
    (install / "lib").mkdir(parents=True)
    (install / "plugins").mkdir()
    (install / "qml").mkdir()
    (install / "lib" / "libQMapLibre.so.3.0.0").write_text(version + "\n", encoding="utf-8")
    return install


class RunDemoRuntimeTest(unittest.TestCase):
    def run_launcher(self, qt_prefix: Path, maplibre: Path):
        environment = os.environ.copy()
        environment.update(
            {"QTMAPS_QT_PREFIX": str(qt_prefix), "QTMAPS_MAPLIBRE_INSTALL": str(maplibre)}
        )
        return subprocess.run(
            [str(LAUNCHER), "--print-env"],
            cwd=ROOT,
            env=environment,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )

    def test_print_env_uses_selected_qt_runtime(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            result = self.run_launcher(make_qt_prefix(root, "5.15.18"), make_maplibre(root, "5.15.18"))
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("QT_VERSION=5.15.18", result.stdout)
        self.assertNotIn("generated/unit-01b/qml-runtime", result.stdout)
        self.assertNotIn("sqldrivers", result.stdout)

    def test_rejects_wrong_qt_or_maplibre_version(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            result = self.run_launcher(
                make_qt_prefix(root / "wrong-qt", "5.15.13"),
                make_maplibre(root / "wrong-qt", "5.15.18"),
            )
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("expected Qt 5.15.18", result.stderr)
            result = self.run_launcher(
                make_qt_prefix(root / "wrong-maplibre", "5.15.18"),
                make_maplibre(root / "wrong-maplibre", "5.15.13"),
            )
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("MapLibre payload does not declare Qt 5.15.18", result.stderr)


if __name__ == "__main__":
    unittest.main()
