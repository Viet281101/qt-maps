# AI Workflow Rules

## Approach

Build QtMaps incrementally with a spec-driven workflow. Read `AGENTS.md` and its six context files in order before implementation or architectural decisions. Work from the current unit spec and record evidence instead of inferring behavior. A proposed technology remains proposed until its spike evidence is recorded and the owner approves the decision.

Until the first management demo on 2026-11-03, optimize the sequence for the shortest stable end-to-end demo path. A thin vertical slice may cross existing feature-unit boundaries when that is necessary to demonstrate map rendering, routing, navigation presentation, and performance together. Keep each implementation increment independently verifiable, record any intentionally deferred completion criteria, and return to the full unit gates after the demo.

## Scoping Rules

- Normally work on one feature unit at a time. Before the first management demo, a documented vertical-slice increment may cross unit boundaries when it directly improves the agreed demo flow.
- Prefer increments that can be verified within one to three four-hour sessions.
- Prioritize visible, stable, end-to-end demo behavior over nationwide coverage, packaging, exhaustive edge-case coverage, and production hardening until the first demo.
- Do not misrepresent fixtures, simulated position, online-only behavior, partial coverage, or unavailable features as completed offline production behavior.
- Do not combine unrelated system boundaries in one implementation step.
- Do not install packages, download large datasets, or change the build system without owner approval.
- Do not add product behavior, dependencies, or technologies absent from the context without explaining the need and marking the addition Proposed.
- If an implementation step cannot be verified end to end quickly, split it before writing code.

## When to Split Work

Split an implementation step when it combines:

- QML/UI changes with background service lifecycle or protocol changes.
- Data acquisition/generation with runtime rendering changes.
- Routing service integration with camera/navigation behavior.
- Performance instrumentation with unrelated product behavior.
- More than one independently testable failure boundary.
- Behavior not clearly defined in the context files.

## Handling Missing Requirements

- Do not invent product behavior, benchmark values, dataset coverage, licenses, or build commands.
- If a requirement is ambiguous, add it to `context/progress-tracker.md` and resolve it in the relevant context/spec before implementation.
- Verify version-specific APIs against Qt 5.13, not current Qt documentation.
- Stop and ask the owner if instructions conflict with the repository or require installation, credentials, external publication, pushing, merging, or destructive data changes.

## Protected Files and Artifacts

Do not modify the following unless explicitly instructed:

- Third-party or vendored library source and generated dependency files.
- Generated MBTiles, routing graphs, geocoding databases, orthography tiles, and cache contents.
- Raw downloaded source datasets.
- Historical benchmark CSV files and published weekly reports; create a corrected successor rather than silently rewriting evidence.
- Files outside this repository.

Do not commit generated datasets, build directories, caches, credentials, or machine-specific secrets.

## Keeping Docs in Sync

Update the relevant context file before continuing whenever implementation changes:

- Product scope or success criteria.
- System architecture, boundaries, service model, or storage model.
- Code, UI, test, or benchmark conventions.
- Dependency choice or decision status.

Update `context/progress-tracker.md` after every meaningful implementation change with completed work, evidence, decisions, risks, and the exact next step.

## Unit Completion Gate

Before moving to the next unit:

1. The unit works end to end within the scope and verification steps in its spec.
2. No invariant in `context/architecture.md` is violated.
3. Relevant automated checks and documented manual scenarios pass.
4. `context/progress-tracker.md` contains the result, evidence, new decisions, and remaining risks.
5. **BUILD PASS** is recorded using the validated Release configure/build commands in `context/code-standards.md` and `context/progress-tracker.md`.
6. A unit that affects rendering records the latest Release-build FPS, total RAM, and total CPU results using the current fixed scenario and includes child processes.
7. Generated artifacts and licenses/attributions are accounted for, and no protected artifact was hand-edited.
