# Code Standards

## General

- Keep modules small, single-purpose, and aligned with the boundaries in `architecture.md`.
- Fix root causes instead of layering silent fallbacks or duplicate state.
- Keep UI, data generation, rendering, routing, search, and performance collection separable and independently testable.
- Do not add a dependency without documenting its purpose, compatibility with Qt 5.13, license, footprint, and decision status.
- Treat external files, network responses, environment variables, and child-process output as untrusted input.
- Avoid deprecated APIs. If Qt 5.13 offers no non-deprecated alternative, document the exception beside the use and in the relevant spec.

## C++

- Use only the C++ language level selected in unit 01. C++14 is proposed; code must not assume a newer standard until the decision is committed.
- Use RAII for ownership and cleanup. Prefer values, `std::unique_ptr`, and `std::shared_ptr` only where ownership is genuinely shared.
- For `QObject` hierarchies, use clear Qt parent ownership or an owning smart pointer, never ambiguous dual ownership.
- Avoid raw owning pointers, manual `new`/`delete` pairs, global mutable state, and unchecked narrowing conversions.
- Use `nullptr`, `override`, scoped enums, const-correct interfaces, and explicit constructors where applicable.
- Keep public headers minimal; use forward declarations or implementation classes when they materially reduce coupling.
- Represent expected failures explicitly through result/status types, signals, or documented error objects; do not use exceptions across Qt signal, QML, or process boundaries.

## Qt and QML

- Use Qt 5.13 APIs only. Check the Qt 5.13 documentation before accepting code suggested for newer Qt releases.
- QML files use explicit versioned imports no newer than the modules shipped with Qt/Qt Quick 2.13.
- QML owns presentation and interaction state; C++ services own I/O, process control, data parsing, and domain behavior.
- Expose narrow models and controllers to QML instead of broad singleton service locators.
- Never block the GUI thread. Use queued signals, asynchronous Qt APIs, or bounded workers for slow operations.
- Reusable visual values come from the QML theme singleton described in `ui-context.md`; do not scatter duplicate color, spacing, font, or radius literals.
- Bindings should be side-effect free. Use named functions for multi-step actions and avoid binding loops.

## CMake

- Use CMake with the Ninja generator. The validated Release configure command is `cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/home/viet/Qt/5.13.2/gcc_64 -DCMAKE_CXX_COMPILER_LAUNCHER=ccache`; build with `cmake --build build --parallel`.
- The root `Makefile` is a convenience wrapper around the validated CMake/Ninja workflow, not a second build system. Keep CMake authoritative and expose machine-specific paths through overridable variables such as `QT_PREFIX`, `BUILD_DIR`, and `BUILD_TYPE`.
- Target Qt 5 explicitly with `find_package(Qt5 REQUIRED COMPONENTS ...)` and `Qt5::` imported targets.
- Use the Qt 5-compatible building blocks `CMAKE_AUTOMOC`, `CMAKE_AUTORCC`, `qt5_add_resources`, and `add_executable` as required by the project layout.
- Do not use Qt 6-only CMake APIs, including `qt_add_executable`, `qt_add_qml_module`, `qt_standard_project_setup`, or other unverified `qt_*` convenience APIs introduced after Qt 5.13.
- Keep generator-specific assumptions out of `CMakeLists.txt`; select Ninja with the configure command rather than embedding a machine-specific generator path.

## Naming

- C++ types and QML component files: `PascalCase`.
- C++ functions, local variables, data members, QML properties, signals, and IDs: `camelCase`.
- Private C++ data members: trailing underscore, for example `routeModel_`.
- Compile-time constants: `kPascalCase`.
- Namespaces: short `lowercase` names; the project namespace is `qtmaps` unless unit 01 discovers a conflict.
- Tests name the behavior and expected result, not the implementation detail.

## Error Handling and Logging

- Validate configuration, paths, database contents, service responses, and coordinate ranges at their boundary.
- User-facing errors state what failed and the next recovery action without exposing secrets or excessive internals.
- Log structured context such as subsystem, operation, duration, and stable error code where useful.
- Use Qt logging categories by subsystem. Do not use ad hoc standard output as the permanent logging interface.
- Never log credentials, tokens, full private paths, or unrestricted request/response payloads.
- A fallback must be visible in state and logs; do not silently change engines, datasets, or online/offline mode.

## Concurrency and Processes

- Document the owner, lifetime, cancellation path, and thread affinity for every worker and `QObject` crossing a thread boundary.
- Communicate across threads with queued signals/slots or carefully owned immutable values.
- Do not access a `QObject` from a thread other than the one that owns it.
- Every asynchronous operation needs a success, failure, cancellation, and bounded-timeout path where applicable.
- Parse child-process output incrementally and cap retained output to avoid unbounded memory growth.
- Shut down child services gracefully, then use a documented bounded forced-termination path if needed.

## Testing

- Use Qt Test for C++ units/integration boundaries and Qt Quick Test for QML behavior.
- Add focused tests for coordinate conversions, polyline precision, response parsing, cache bounds, and failure paths as those modules appear.
- Use small deterministic fixtures; do not require the full Vietnam dataset for routine tests.
- Any external-service integration test must state its dataset, ports, process lifecycle, timeout, and offline assumptions.
- UI/render units include a manual or automated end-to-end verification on the reference hardware and current FPS/RAM/CPU results when required by their spec.

## Data and Storage

- Generated MBTiles, routing graphs, geocoding databases, orthography tiles, caches, and benchmark output are artifacts, not hand-maintained source.
- Record source URL, retrieval date, license/attribution, geographic coverage, zoom range where applicable, and generator configuration for every dataset.
- Use transactions and prepared statements for SQLite access.
- Bound caches by an explicit resource limit and test eviction behavior.

## File Organization

- `app/` — executable entry point and lifecycle wiring.
- `qml/` — screens, controls, overlays, and theme resources.
- `src/core/`, `src/map/`, `src/routing/`, `src/search/`, `src/perf/` — C++ code for the corresponding architecture boundary.
- `tests/` — Qt Test and Qt Quick Test code and compact fixtures.
- `tools/data-pipeline/` — reproducible data-preparation tooling.
- `docs/` — benchmark and management-facing reports.
- `context/` — authoritative project context and feature-unit specs.

Pair implementation files consistently (`Name.h` and `Name.cpp`) unless the selected build system or Qt tooling establishes a different convention. The final source layout and build-file placement must be recorded during unit 01.
