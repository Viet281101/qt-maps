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

QtMaps targets Qt 5.15.13 and uses CMake with Ninja. Qt 5.13.2
is retained only as an explicit comparison configuration. Do not use
Qt 6 CMake APIs such as `qt_add_executable`,
`qt_add_qml_module`, or `qt_standard_project_setup`. Use the
Qt 5 pattern based on `find_package(Qt5 ...)`,
`CMAKE_AUTOMOC`/`CMAKE_AUTORCC`, `qt5_add_resources`, and
`add_executable`.
