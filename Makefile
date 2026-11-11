SHELL := /bin/bash

.DEFAULT_GOAL := help

QT_PREFIX ?= /usr
QT_EXPECTED_VERSION ?= 5.15.18
QTMAPS_MIN_QT_VERSION ?= 5.15.18
BUILD_DIR ?= build-qt515
BUILD_TYPE ?= Release
DEBUG_BUILD_DIR ?= build-qt515-debug
REVIEW_BUILD_DIR ?= build-review-qt515
CMAKE ?= cmake
PYTHON ?= python3
CLANG_FORMAT ?= clang-format
GDB ?= gdb
APP_ARGS ?=
DEMO_ARGS ?=
BENCHMARK_ARGS ?=
HANOI_PBF ?= generated/unit-01b/source/vietnam-latest.osm.pbf
PLANETILER_JAR ?= generated/unit-01b/tools/planetiler.jar
MAP_PACKAGE ?= data/packages/current
MAPLIBRE_INSTALL ?= generated/unit-01b/maplibre-install
MAPLIBRE_SRC ?= generated/unit-01b/maplibre-native-qt
MAPLIBRE_BUILD_NEW ?= generated/unit-01b/maplibre-build-qt51518
MAPLIBRE_INSTALL_NEW ?= generated/unit-01b/maplibre-install-qt51518
QT_PRIVATE_OVERLAY ?= generated/unit-01b/qt51518-private
MAPLIBRE_EXTRA_CXX_FLAGS ?= -include cstdint
MAPLIBRE_BUILD_KEEP_GOING ?= 1
QTMAPS_SYSTEM_PACKAGES := qtbase5-dev qtdeclarative5-dev qtlocation5-dev qtpositioning5-dev
MAPLIBRE_SYSTEM_PACKAGES := $(QTMAPS_SYSTEM_PACKAGES) qtbase5-private-dev qtdeclarative5-private-dev libicu-dev
QML_RUNTIME ?= $(shell "$(QMAKE)" -query QT_INSTALL_ARCHDATA 2>/dev/null)
QT_RUNTIME_PLUGIN_PATH ?= $(shell "$(QMAKE)" -query QT_INSTALL_PLUGINS 2>/dev/null)

QMAKE := $(QT_PREFIX)/bin/qmake
QTMAPS := $(BUILD_DIR)/qtmaps
MAPLIBRE_NINJA_KEEP_GOING_ARGS := $(if $(filter 0 false no off,$(MAPLIBRE_BUILD_KEEP_GOING)),,-- -k 0)

ifneq (,$(filter maplibre-qt51518,$(MAKECMDGOALS)))
ifeq ($(wildcard $(QT_PRIVATE_OVERLAY)),)
$(error maplibre-qt51518 requires Qt 5.15.18 private-header overlay directory: $(QT_PRIVATE_OVERLAY))
endif
ifeq ($(wildcard $(QT_PRIVATE_OVERLAY)/qt5-private-targets.cmake),)
$(error maplibre-qt51518 requires $(QT_PRIVATE_OVERLAY)/qt5-private-targets.cmake)
endif
MAPLIBRE_QT51518_VERSION := $(shell "$(QMAKE)" -query QT_VERSION 2>/dev/null)
ifneq ($(MAPLIBRE_QT51518_VERSION),5.15.18)
$(error maplibre-qt51518 requires qmake Qt 5.15.18 at $(QMAKE), found '$(MAPLIBRE_QT51518_VERSION)')
endif
endif

.PHONY: help requirements requirements-apt maplibre-requirements maplibre-requirements-apt configure build test run run-app benchmark debug-build debug-run debug-gdb check \
	review format format-check diagnostic-tiles vector-fixture vector-real-style hanoi-package hanoi-raster-render hanoi-raster-package \
	raster serve-diagnostic-tiles serve-vector-fixture run-mapboxgl-probe maplibre-qt51518 clean clean-old-builds

help:
	@echo "QtMaps development targets"
	@echo "  make requirements          Verify the Qt/C++ toolchain for normal QtMaps builds"
	@echo "  make requirements-apt      Print normal-build APT restore/manual-mark commands"
	@echo "  make maplibre-requirements Verify private Qt/ICU prerequisites for a MapLibre rebuild"
	@echo "  make maplibre-requirements-apt Print MapLibre rebuild APT restore/manual-mark commands"
	@echo "  make configure             Configure a CMake build with Ninja"
	@echo "  make build                 Configure and build QtMaps"
	@echo "  make test                  Build and run CTest with failures shown"
	@echo "  make run                   Build and run the full local Release demo"
	@echo "  make run-app               Build and run the raw application (APP_ARGS=...)"
	@echo "  make benchmark             Run Unit 06 scenarios and generate Markdown/JSON reports"
	@echo "  make debug-build           Build an isolated Debug configuration"
	@echo "  make debug-run             Build and run the full local Debug demo"
	@echo "  make debug-gdb             Build and debug the full local demo in GDB"
	@echo "  make check                 Run normal pre-commit formatting, tests, and diff checks"
	@echo "  make review                Run isolated Release tests and branch diff checks"
	@echo "  make format                Format tracked C/C++ sources and tests in place"
	@echo "  make format-check          Check tracked C/C++ formatting without editing"
	@echo "  make diagnostic-tiles      Generate the non-geographic XYZ fixture"
	@echo "  make serve-diagnostic-tiles Serve the fixture on 127.0.0.1:8765"
	@echo "  make vector-fixture        Generate the synthetic local MVT/style/glyph/sprite fixture"
	@echo "  make vector-real-style     Generate local style/glyph/sprite resources for real Planetiler tiles"
	@echo "  make hanoi-package         Build ignored versioned Hanoi MBTiles package from approved local inputs"
	@echo "  make hanoi-raster-package  Package locally rendered z12-z16 PNG tiles; requires RASTER_TILES"
	@echo "  make hanoi-raster-render   Render local z12-z16 PNG tiles from MAP_PACKAGE"
	@echo "  make raster                Re-render and package raster after style/schema migration"
	@echo "  make maplibre-qt51518      Deliberately rebuild MapLibre for Qt 5.15.18 (~16 min, ~1.1 GiB peak RSS)"
	@echo "  make serve-vector-fixture  Serve the vector fixture on 127.0.0.1:8766"
	@echo "  make run-mapboxgl-probe    Run probe (vector fixture server must be separate)"
	@echo "  make clean                 Clean compiled outputs through CMake"
	@echo "  make clean-old-builds      Remove other root build/build-* CMake directories; keep BUILD_DIR"
	@echo
	@echo "Overrides: QT_PREFIX, BUILD_DIR, BUILD_TYPE, DEBUG_BUILD_DIR, REVIEW_BUILD_DIR, DEMO_ARGS, APP_ARGS, BENCHMARK_ARGS, GDB, CLANG_FORMAT"
	@echo "MapLibre rebuild overrides: MAPLIBRE_SRC, MAPLIBRE_BUILD_NEW, MAPLIBRE_INSTALL_NEW, QT_PRIVATE_OVERLAY, MAPLIBRE_EXTRA_CXX_FLAGS, MAPLIBRE_BUILD_KEEP_GOING"

requirements:
	@command -v "$(CMAKE)" >/dev/null || { echo "error: CMake is required" >&2; exit 1; }
	@command -v ninja >/dev/null || { echo "error: Ninja is required" >&2; exit 1; }
	@command -v ccache >/dev/null || { echo "error: ccache is required" >&2; exit 1; }
	@command -v g++ >/dev/null || { echo "error: g++ is required" >&2; exit 1; }
	@command -v "$(PYTHON)" >/dev/null || { echo "error: Python 3 is required" >&2; exit 1; }
	@test -x "$(QMAKE)" || { echo "error: Qt qmake not found at $(QMAKE)" >&2; exit 1; }
	@qt_version="$$($(QMAKE) -query QT_VERSION)"; \
		test "$$qt_version" = "$(QT_EXPECTED_VERSION)" || { \
			echo "error: expected Qt $(QT_EXPECTED_VERSION) at $(QT_PREFIX), found $$qt_version" >&2; \
			exit 1; \
		}
	@missing=""; \
		for package in $(QTMAPS_SYSTEM_PACKAGES); do \
			dpkg-query -W -f='$${db:Status-Status}' "$$package" 2>/dev/null | grep -Fxq installed || missing="$$missing $$package"; \
		done; \
		test -z "$$missing" || { \
			echo "error: missing required development packages:$$missing" >&2; \
			echo "hint: run make requirements-apt, then execute the printed install command" >&2; \
			exit 1; \
		}
	@echo "requirements: OK (Qt $(QT_EXPECTED_VERSION), normal Qt development packages, CMake, Ninja, ccache, g++; Python 3 for tools)"

requirements-apt:
	@echo "This target does not run APT or sudo. Review and execute manually:"
	@echo "  sudo apt install --no-install-recommends $(QTMAPS_SYSTEM_PACKAGES)"
	@echo "  sudo apt-mark manual $(QTMAPS_SYSTEM_PACKAGES)"

maplibre-requirements: requirements
	@missing=""; \
		for package in qtbase5-private-dev qtdeclarative5-private-dev libicu-dev; do \
			dpkg-query -W -f='$${db:Status-Status}' "$$package" 2>/dev/null | grep -Fxq installed || missing="$$missing $$package"; \
		done; \
		test -z "$$missing" || { \
			echo "error: missing MapLibre rebuild development packages:$$missing" >&2; \
			echo "hint: run make maplibre-requirements-apt, then execute the printed install command" >&2; \
			exit 1; \
		}
	@test -f /usr/include/unicode/uchar.h || { echo "error: ICU development header missing: /usr/include/unicode/uchar.h" >&2; exit 1; }
	@test -e /usr/lib/x86_64-linux-gnu/libicuuc.so || { echo "error: ICU development library missing: /usr/lib/x86_64-linux-gnu/libicuuc.so" >&2; exit 1; }
	@echo "maplibre-requirements: OK (private Qt/ICU development packages for the explicit rebuild target)"

maplibre-requirements-apt:
	@echo "This target does not run APT or sudo. Review and execute manually:"
	@echo "  sudo apt install --no-install-recommends $(MAPLIBRE_SYSTEM_PACKAGES)"
	@echo "  sudo apt-mark manual $(MAPLIBRE_SYSTEM_PACKAGES)"

configure: requirements
	"$(CMAKE)" -S . -B "$(BUILD_DIR)" -G Ninja \
		-DCMAKE_BUILD_TYPE="$(BUILD_TYPE)" \
		-DCMAKE_PREFIX_PATH="$(QT_PREFIX)" \
		-DQTMAPS_MIN_QT_VERSION="$(QTMAPS_MIN_QT_VERSION)" \
		-DCMAKE_CXX_COMPILER_LAUNCHER=ccache

build: configure
	"$(CMAKE)" --build "$(BUILD_DIR)" --parallel

maplibre-qt51518: maplibre-requirements
	@echo "maplibre-qt51518: intentional Qt 5.15.18 rebuild; historical estimate ~16 min and ~1.1 GiB peak RSS"
	"$(CMAKE)" --fresh -S "$(MAPLIBRE_SRC)" -B "$(MAPLIBRE_BUILD_NEW)" -G Ninja \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_PREFIX_PATH=/usr \
		-DQt5_DIR=/usr/lib/x86_64-linux-gnu/cmake/Qt5 \
		-DQT_VERSION_MAJOR=5 \
		-DMLN_QT_WITH_WIDGETS=OFF \
		-DCMAKE_CXX_FLAGS="$(MAPLIBRE_EXTRA_CXX_FLAGS)" \
		-DCMAKE_PROJECT_INCLUDE="$(abspath $(QT_PRIVATE_OVERLAY)/qt5-private-targets.cmake)" \
		-DCMAKE_INSTALL_PREFIX="$(abspath $(MAPLIBRE_INSTALL_NEW))"
	"$(CMAKE)" --build "$(MAPLIBRE_BUILD_NEW)" --parallel 2 $(MAPLIBRE_NINJA_KEEP_GOING_ARGS)
	"$(CMAKE)" --install "$(MAPLIBRE_BUILD_NEW)"
	@test -f "$(MAPLIBRE_INSTALL_NEW)/lib/libQMapLibre.so.3.0.0" || { \
		echo "error: MapLibre install is missing libQMapLibre.so.3.0.0 under $(MAPLIBRE_INSTALL_NEW)/lib" >&2; \
		exit 1; \
	}
	@strings "$(MAPLIBRE_INSTALL_NEW)/lib/libQMapLibre.so.3.0.0" | grep -Fxq "5.15.18" || { \
		echo "error: MapLibre payload does not contain the required Qt 5.15.18 version marker" >&2; \
		exit 1; \
	}
	@echo "maplibre-qt51518: installed payload contains the Qt 5.15.18 marker"

test: build
	@test -f "$(QT_RUNTIME_PLUGIN_PATH)/sqldrivers/libqsqlite.so" || { \
		echo "error: Qt SQLite runtime plugin not found under $(QT_RUNTIME_PLUGIN_PATH); set QT_RUNTIME_PLUGIN_PATH to a Qt 5.15 plugin root" >&2; \
		exit 1; \
	}
	env QTMAPS_QML_RUNTIME="$(abspath $(QML_RUNTIME))" \
		QTMAPS_MAPLIBRE_INSTALL="$(abspath $(MAPLIBRE_INSTALL))" \
		QT_PLUGIN_PATH="$(abspath $(QT_RUNTIME_PLUGIN_PATH))$${QT_PLUGIN_PATH:+:$$QT_PLUGIN_PATH}" \
		ctest --test-dir "$(BUILD_DIR)" --output-on-failure

run: build
	QTMAPS_APP="$(abspath $(QTMAPS))" QTMAPS_QT_PREFIX="$(QT_PREFIX)" \
		QTMAPS_MAPLIBRE_INSTALL="$(abspath $(MAPLIBRE_INSTALL))" ./tools/run-demo.sh $(DEMO_ARGS)

run-app: build
	"$(QTMAPS)" $(APP_ARGS)

benchmark: build
	"$(PYTHON)" tools/benchmark/run_benchmarks.py \
		--app "$(abspath $(QTMAPS))" --skip-build $(BENCHMARK_ARGS)

debug-build:
	+$(MAKE) BUILD_DIR="$(DEBUG_BUILD_DIR)" BUILD_TYPE=Debug build

debug-run: debug-build
	QTMAPS_APP="$(abspath $(DEBUG_BUILD_DIR)/qtmaps)" QTMAPS_QT_PREFIX="$(QT_PREFIX)" \
		QTMAPS_MAPLIBRE_INSTALL="$(abspath $(MAPLIBRE_INSTALL))" ./tools/run-demo.sh $(DEMO_ARGS)

debug-gdb: debug-build
	@command -v "$(GDB)" >/dev/null || { \
		echo "error: GDB is required; set GDB to its executable" >&2; \
		exit 1; \
	}
	QTMAPS_APP="$(abspath $(DEBUG_BUILD_DIR)/qtmaps)" QTMAPS_QT_PREFIX="$(QT_PREFIX)" \
		QTMAPS_MAPLIBRE_INSTALL="$(abspath $(MAPLIBRE_INSTALL))" QTMAPS_GDB=1 QTMAPS_GDB_BIN="$(GDB)" \
		./tools/run-demo.sh $(DEMO_ARGS)

check: format-check test
	@git diff --check
	@git diff --cached --check
	@echo "check: formatting, tests, and working-tree diff checks passed"

review: format-check
	@status="$$(git status --short)"; \
		if test -n "$$status"; then \
			echo "error: review requires a clean worktree; commit or stash these changes:" >&2; \
			printf '%s\n' "$$status" >&2; \
			exit 1; \
		fi
	+$(MAKE) BUILD_DIR="$(REVIEW_BUILD_DIR)" BUILD_TYPE=Release test
	@git rev-parse --verify origin/main >/dev/null || { \
		echo "error: origin/main is required; run git fetch origin first" >&2; \
		exit 1; \
	}
	@git diff --check
	@git diff --cached --check
	@git diff --check origin/main...HEAD
	@echo "review: isolated Release tests and branch diff checks passed; worktree is clean"

format:
	@command -v "$(CLANG_FORMAT)" >/dev/null || { \
		echo "error: clang-format is required; set CLANG_FORMAT to its executable" >&2; \
		exit 1; \
	}
	@git ls-files -z -- '*.c' '*.cc' '*.cpp' '*.cxx' '*.h' '*.hh' '*.hpp' '*.hxx' | \
		xargs -0 -r "$(CLANG_FORMAT)" --style=file -i
	@echo "format: C/C++ sources formatted"

format-check:
	@command -v "$(CLANG_FORMAT)" >/dev/null || { \
		echo "error: clang-format is required; set CLANG_FORMAT to its executable" >&2; \
		exit 1; \
	}
	@git ls-files -z -- '*.c' '*.cc' '*.cpp' '*.cxx' '*.h' '*.hh' '*.hpp' '*.hxx' | \
		xargs -0 -r "$(CLANG_FORMAT)" --style=file --dry-run --Werror
	@echo "format-check: C/C++ formatting is clean"

diagnostic-tiles:
	"$(PYTHON)" tools/generate_diagnostic_tiles.py

serve-diagnostic-tiles: diagnostic-tiles
	"$(PYTHON)" -m http.server 8765 --bind 127.0.0.1 --directory generated/diagnostic-tiles

vector-fixture:
	"$(PYTHON)" tools/generate_vector_fixture.py

vector-real-style:
	"$(PYTHON)" tools/generate_vector_fixture.py --real-style-only --output generated/unit-01b/site

hanoi-package:
	"$(PYTHON)" tools/data-pipeline/build_hanoi_package.py \
		--pbf "$(HANOI_PBF)" --planetiler-jar "$(PLANETILER_JAR)"

hanoi-raster-package:
	@test -n "$(RASTER_TILES)" || { echo "error: set RASTER_TILES to z/x/y.png root" >&2; exit 1; }
	"$(PYTHON)" tools/data-pipeline/build_hanoi_raster_package.py \
		--tiles "$(RASTER_TILES)" --vector-package "$(MAP_PACKAGE)"

hanoi-raster-render: build
	env LD_LIBRARY_PATH="$(abspath $(MAPLIBRE_INSTALL))/lib$${LD_LIBRARY_PATH:+:$$LD_LIBRARY_PATH}" \
		QT_PLUGIN_PATH="$(abspath $(MAPLIBRE_INSTALL))/plugins:$(abspath $(QML_RUNTIME))/plugins$${QT_PLUGIN_PATH:+:$$QT_PLUGIN_PATH}" \
		QML2_IMPORT_PATH="$(abspath $(MAPLIBRE_INSTALL))/qml:$(abspath $(QML_RUNTIME))/qml$${QML2_IMPORT_PATH:+:$$QML2_IMPORT_PATH}" \
		"$(BUILD_DIR)/qtmaps_raster_renderer" \
		--vector-package "$(MAP_PACKAGE)" \
		--output "$(if $(RASTER_TILES),$(RASTER_TILES),generated/unit-07c/rendered-tiles)"

raster: RASTER_TILES := $(if $(RASTER_TILES),$(RASTER_TILES),generated/unit-07c/rendered-tiles)
raster: hanoi-raster-render hanoi-raster-package

serve-vector-fixture: vector-fixture
	"$(PYTHON)" -m http.server 8766 --bind 127.0.0.1 --directory generated/vector-fixture

run-mapboxgl-probe: build
	"$(QTMAPS)" --mapboxgl-probe \
		--dataset-id synthetic-vector-mechanics-v1 \
		--dataset-coverage "synthetic local MVT around Hanoi coordinates; not geographic data" \
		--network-mode localhost-only

clean:
	@if test -d "$(BUILD_DIR)"; then \
		"$(CMAKE)" --build "$(BUILD_DIR)" --target clean; \
	else \
		echo "clean: $(BUILD_DIR) does not exist"; \
	fi

clean-old-builds:
	@set -eu; \
		repository_root="$$(pwd -P)"; \
		active_build="$$(realpath -m "$(BUILD_DIR)")"; \
		removed=0; \
		for candidate in build build-*; do \
			test -d "$$candidate" || continue; \
			candidate_path="$$(realpath -m "$$candidate")"; \
			case "$$candidate_path" in "$$repository_root"/build|"$$repository_root"/build-*) ;; *) \
				echo "clean-old-builds: refusing path outside root build directories: $$candidate" >&2; exit 1;; esac; \
			if test "$$candidate_path" = "$$active_build"; then \
				echo "clean-old-builds: keeping active BUILD_DIR $$candidate"; continue; \
			fi; \
			if test ! -f "$$candidate/CMakeCache.txt"; then \
				echo "clean-old-builds: skipping non-CMake directory $$candidate"; continue; \
			fi; \
			echo "clean-old-builds: removing $$candidate"; \
			rm -rf -- "$$candidate"; \
			removed=1; \
		done; \
		if test "$$removed" = 0; then \
			echo "clean-old-builds: no old CMake build directories found"; \
		fi
