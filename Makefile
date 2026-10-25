SHELL := /bin/bash

.DEFAULT_GOAL := help

QT_PREFIX ?= /usr
QT_EXPECTED_VERSION ?= 5.15.13
QTMAPS_MIN_QT_VERSION ?= 5.15.13
BUILD_DIR ?= build-qt515
BUILD_TYPE ?= Release
CMAKE ?= cmake
PYTHON ?= python3
HANOI_PBF ?= generated/unit-01b/source/vietnam-latest.osm.pbf
PLANETILER_JAR ?= generated/unit-01b/tools/planetiler.jar
QT_RUNTIME_PLUGIN_PATH ?= generated/unit-01b/qml-runtime/usr/lib/x86_64-linux-gnu/qt5/plugins

QMAKE := $(QT_PREFIX)/bin/qmake
QTMAPS := $(BUILD_DIR)/qtmaps

.PHONY: help requirements configure build test run diagnostic-tiles vector-fixture vector-real-style hanoi-package \
	serve-diagnostic-tiles serve-vector-fixture run-mapboxgl-probe clean clean-old-builds

help:
	@echo "QtMaps development targets"
	@echo "  make requirements          Verify the Qt/C++ toolchain and helper tools"
	@echo "  make configure             Configure a CMake build with Ninja"
	@echo "  make build                 Configure and build QtMaps"
	@echo "  make test                  Build and run CTest with failures shown"
	@echo "  make run                   Build and run (fixture server must be separate)"
	@echo "  make diagnostic-tiles      Generate the non-geographic XYZ fixture"
	@echo "  make serve-diagnostic-tiles Serve the fixture on 127.0.0.1:8765"
	@echo "  make vector-fixture        Generate the synthetic local MVT/style/glyph/sprite fixture"
	@echo "  make vector-real-style     Generate local style/glyph/sprite resources for real Planetiler tiles"
	@echo "  make hanoi-package         Build ignored versioned Hanoi MBTiles package from approved local inputs"
	@echo "  make serve-vector-fixture  Serve the vector fixture on 127.0.0.1:8766"
	@echo "  make run-mapboxgl-probe    Run probe (vector fixture server must be separate)"
	@echo "  make clean                 Clean compiled outputs through CMake"
	@echo "  make clean-old-builds      Remove other root build/build-* CMake directories; keep BUILD_DIR"
	@echo
	@echo "Overrides: QT_PREFIX, QT_EXPECTED_VERSION, QTMAPS_MIN_QT_VERSION, BUILD_DIR, BUILD_TYPE, CMAKE, PYTHON, QT_RUNTIME_PLUGIN_PATH, HANOI_PBF, PLANETILER_JAR"

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
	@echo "requirements: OK (Qt $(QT_EXPECTED_VERSION), CMake, Ninja, ccache, g++; Python 3 for tools)"

configure: requirements
	"$(CMAKE)" -S . -B "$(BUILD_DIR)" -G Ninja \
		-DCMAKE_BUILD_TYPE="$(BUILD_TYPE)" \
		-DCMAKE_PREFIX_PATH="$(QT_PREFIX)" \
		-DQTMAPS_MIN_QT_VERSION="$(QTMAPS_MIN_QT_VERSION)" \
		-DCMAKE_CXX_COMPILER_LAUNCHER=ccache

build: configure
	"$(CMAKE)" --build "$(BUILD_DIR)" --parallel

test: build
	@test -f "$(QT_RUNTIME_PLUGIN_PATH)/sqldrivers/libqsqlite.so" || { \
		echo "error: Qt SQLite runtime plugin not found under $(QT_RUNTIME_PLUGIN_PATH); set QT_RUNTIME_PLUGIN_PATH to a Qt 5.15 plugin root" >&2; \
		exit 1; \
	}
	env QT_PLUGIN_PATH="$(abspath $(QT_RUNTIME_PLUGIN_PATH))$${QT_PLUGIN_PATH:+:$$QT_PLUGIN_PATH}" \
		ctest --test-dir "$(BUILD_DIR)" --output-on-failure

run: build
	"$(QTMAPS)"

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
