# Application Building Context

Read the following files in order before implementing
or making any architectural decision:

1. `context/project-overview.md` — product definition,
   goals, features, and scope
2. `context/architecture.md` — system structure,
   boundaries, storage model, and invariants
3. `context/ui-context.md` — theme, colors, typography,
   and component conventions
4. `context/code-standards.md` — implementation rules
   and conventions
5. `context/ai-workflow-rules.md` — development workflow,
   scoping rules, and delivery approach
6. `context/progress-tracker.md` — current phase,
   completed work, open questions, and next steps

Update `context/progress-tracker.md` after each
meaningful implementation change.

If implementation changes the architecture, scope, or
standards documented in the context files, update the
relevant file before continuing.

QtMaps targets Qt 5.15.18 and uses CMake with Ninja. Run
`make QT_EXPECTED_VERSION=5.15.18 test` before every commit. Qt 5.15.13
is retained only as an explicit comparison configuration. Do not use
Qt 6 CMake APIs such as `qt_add_executable`,
`qt_add_qml_module`, or `qt_standard_project_setup`. Use the
Qt 5 pattern based on `find_package(Qt5 ...)`,
`CMAKE_AUTOMOC`/`CMAKE_AUTORCC`, `qt5_add_resources`, and
`add_executable`.

## Safety and integration constraints

While the i915 investigation remains open, do not start MapLibre/OpenGL apps,
benchmarks, or GPU rendering; use test-only/offscreen verification. Do not run
`apt autoremove`, alter GRUB/kernel/Mesa, push, or create a PR without explicit
owner approval. Keep temporary binaries and evidence under `generated/`, never
under `/tmp`. Read `/sys/module/i915/parameters/*` through `sudo`; unprivileged
reads can be denied and must not be bypassed. Commit `937f68a` intentionally
limits A+hooks tracing to liveness and is not a frame-rate/stability baseline.
