SHELL := /bin/bash

.DEFAULT_GOAL := help

QT_PREFIX ?= /home/viet/Qt/5.13.2/gcc_64
BUILD_DIR ?= build
BUILD_TYPE ?= Release
CMAKE ?= cmake
PYTHON ?= python3

QMAKE := $(QT_PREFIX)/bin/qmake
QTMAPS := $(BUILD_DIR)/qtmaps

.PHONY: help requirements configure build run diagnostic-tiles \
	serve-diagnostic-tiles serve-mapboxgl-style run-mapboxgl-probe clean

help:
	@echo "QtMaps development targets"
	@echo "  make requirements          Verify the Qt/C++ toolchain and helper tools"
	@echo "  make configure             Configure a CMake build with Ninja"
	@echo "  make build                 Configure and build QtMaps"
	@echo "  make run                   Build and run (fixture server must be separate)"
	@echo "  make diagnostic-tiles      Generate the non-geographic XYZ fixture"
	@echo "  make serve-diagnostic-tiles Serve the fixture on 127.0.0.1:8765"
	@echo "  make serve-mapboxgl-style  Serve the style probe on 127.0.0.1:8766"
	@echo "  make run-mapboxgl-probe    Run probe (style server must be separate)"
	@echo "  make clean                 Clean compiled outputs through CMake"
	@echo
	@echo "Overrides: QT_PREFIX, BUILD_DIR, BUILD_TYPE, CMAKE, PYTHON"

requirements:
	@command -v "$(CMAKE)" >/dev/null || { echo "error: CMake is required" >&2; exit 1; }
	@command -v ninja >/dev/null || { echo "error: Ninja is required" >&2; exit 1; }
	@command -v ccache >/dev/null || { echo "error: ccache is required" >&2; exit 1; }
	@command -v g++ >/dev/null || { echo "error: g++ is required" >&2; exit 1; }
	@command -v "$(PYTHON)" >/dev/null || { echo "error: Python 3 is required" >&2; exit 1; }
	@test -x "$(QMAKE)" || { echo "error: Qt qmake not found at $(QMAKE)" >&2; exit 1; }
	@qt_version="$$($(QMAKE) -query QT_VERSION)"; \
		test "$$qt_version" = "5.13.2" || { \
			echo "error: expected Qt 5.13.2 at $(QT_PREFIX), found $$qt_version" >&2; \
			exit 1; \
		}
	@echo "requirements: OK (Qt 5.13.2, CMake, Ninja, ccache, g++; Python 3 for tools)"

configure: requirements
	"$(CMAKE)" -S . -B "$(BUILD_DIR)" -G Ninja \
		-DCMAKE_BUILD_TYPE="$(BUILD_TYPE)" \
		-DCMAKE_PREFIX_PATH="$(QT_PREFIX)" \
		-DCMAKE_CXX_COMPILER_LAUNCHER=ccache

build: configure
	"$(CMAKE)" --build "$(BUILD_DIR)" --parallel

run: build
	"$(QTMAPS)"

diagnostic-tiles:
	"$(PYTHON)" tools/generate_diagnostic_tiles.py

serve-diagnostic-tiles: diagnostic-tiles
	"$(PYTHON)" -m http.server 8765 --bind 127.0.0.1 --directory generated/diagnostic-tiles

serve-mapboxgl-style:
	"$(PYTHON)" -m http.server 8766 --bind 127.0.0.1 --directory fixtures/mapboxgl

run-mapboxgl-probe: build
	"$(QTMAPS)" --mapboxgl-probe

clean:
	@if test -d "$(BUILD_DIR)"; then \
		"$(CMAKE)" --build "$(BUILD_DIR)" --target clean; \
	else \
		echo "clean: $(BUILD_DIR) does not exist"; \
	fi
