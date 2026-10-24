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

## Repository and Project Identity

- The GitHub Project identity is fixed as Project #6; its current display title is `QtMaps`. Do not rename this Project or replace it with another board.
- Do not rename the `qt-maps` GitHub repository unless the owner explicitly requests the rename, the necessity is documented, and the impact on remotes, links, automation, documentation, and integrations has been reviewed before the change.
- Treat repository and Project URLs, numbers, and names as shared external contracts. A feature, cleanup, or documentation task does not implicitly authorize changing them.

## Continuous Impact Verification

After every implementation, repair, configuration, documentation, commit, or external GitHub metadata change:

1. Inspect the resulting diff or live state and identify affected code paths, build targets, tests, documentation, links, Issues, milestones, and Project fields.
2. Run verification proportional to the change before pushing. At minimum run `git diff --check` for repository changes; code, build, QML, runtime, data-pipeline, and benchmark changes also require their relevant automated checks and documented manual scenario.
3. Check `git status` to ensure generated files, secrets, unrelated edits, and user-owned changes are not accidentally included.
4. Re-read the changed context and the exact next step for contradictions or stale status. Update `context/progress-tracker.md` when the change is meaningful.
5. For GitHub mutations, query the live PR, Issue, milestone, repository, or Project state after the mutation instead of assuming the command succeeded.
6. Do not continue to the next change while a newly introduced regression, unexplained side effect, broken link, metadata mismatch, or failed check remains unresolved or explicitly recorded as a blocker.

## Keeping Docs in Sync

Update the relevant context file before continuing whenever implementation changes:

- Product scope or success criteria.
- System architecture, boundaries, service model, or storage model.
- Code, UI, test, or benchmark conventions.
- Dependency choice or decision status.

Update `context/progress-tracker.md` after every meaningful implementation change with completed work, evidence, decisions, risks, and the exact next step.

## Automatic Context Compaction

Keep the six mandatory context files as concise operational state, not append-only session logs. After every meaningful context update, check their line and byte counts. A context file must be compacted in the same change before work continues when any of these conditions is true:

- it exceeds 250 lines or 24 KiB;
- historical narrative or repeated evidence materially obscures current requirements and next actions; or
- the same fact is maintained in more than one context file without a clear authoritative owner.

Compaction is routine documentation maintenance and does not require separate owner approval. During compaction:

1. Preserve current scope, committed/proposed decision status, invariants, active assumptions, unresolved risks, acceptance criteria, working commands, evidence links, and the exact next step.
2. Replace completed chronological narration with a short outcome summary and links to the authoritative spec, benchmark/report, pull request/issue, or a dated archive under `context/history/`. Git history may be used as the archive for an already committed snapshot when the retained file names the relevant commit.
3. Never rewrite or discard raw benchmark evidence, published reports, license/attribution records, or unresolved owner decisions. Do not present an archived or compacted item as completed unless its cited evidence supports that status.
4. Remove duplicate facts from non-authoritative files and leave a link to the authoritative file instead. Do not create a second summary that must be kept manually in sync.
5. Re-read all changed sections, validate local links, run `git diff --check`, and confirm the compacted files remain sufficient for a new agent to resume without reconstructing the project from commit history.

## Unit Completion Gate

Before moving to the next unit:

1. The unit works end to end within the scope and verification steps in its spec.
2. No invariant in `context/architecture.md` is violated.
3. Relevant automated checks and documented manual scenarios pass.
4. `context/progress-tracker.md` contains the result, evidence, new decisions, and remaining risks.
5. **BUILD PASS** is recorded using the validated Release configure/build commands in `context/code-standards.md` and `context/progress-tracker.md`.
6. A unit that affects rendering records the latest Release-build FPS, total RAM, and total CPU results using the current fixed scenario and includes child processes.
7. Generated artifacts and licenses/attributions are accounted for, and no protected artifact was hand-edited.

## Pull Request Tracking Gate

After creating a pull request for QtMaps:

1. Identify the canonical tracking issue before editing GitHub metadata. Add `Refs #N` to the pull request description while acceptance criteria remain open; use a closing keyword only when the pull request fully satisfies the issue and closing it on merge is intended.
2. Assign the active delivery milestone to the pull request.
3. Add the pull request to [Project #6, QtMaps](https://github.com/users/Viet281101/projects/6) without creating a duplicate item.
4. Align the pull request's Priority and Demo Phase project fields with its primary tracking issue. Set Status to `In Progress` while the pull request is open and to `Done` only after the work is merged or otherwise completed.
5. Move the primary tracking issue to `In Progress` when implementation or review has started, while leaving the issue open until all of its acceptance criteria are satisfied.
6. Verify the live pull request description, milestone, Project item, and Project fields after all mutations. Record the pull request, linked issue, milestone, and Project status in `context/progress-tracker.md`.
