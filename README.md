# QtMaps

**A local-first desktop map and navigation demo for Hanoi, Vietnam.**

QtMaps combines Qt/QML, local map data, car routing, and simulated navigation.
It labels local, fixture-backed, and incomplete behavior honestly.

[Tiếng Việt](README_VN.md) · [Project board](https://github.com/users/Viet281101/projects/6) · [Roadmap](context/project-overview.md)

## What works today

| Capability | Current state |
| --- | --- |
| Hanoi vector map | Bounded local data with Vietnamese labels |
| Raster fallback | Local generated recovery fallback |
| Car routing | Local Valhalla service for the Hanoi graph |
| Navigation | Deterministic replay, maneuvers, and re-routing flow |
| Performance | Release FPS, CPU, and memory instrumentation |

The data covers a **bounded Hanoi area**, not nationwide Vietnam. Live traffic,
GPS, voice guidance, mobile/web, hosted services, POI search, and long soak
testing are not product capabilities yet.

## Quick start

QtMaps targets **Qt 5.15.18**, CMake, Ninja, C++, QML/Qt Quick, and MapLibre
Native Qt 3.0.0 on Linux. Qt 5.15.13 is comparison-only.

```bash
make requirements
make test
make run
```

Use `make check` during development and `make review` before a PR. The latter
requires a clean worktree. `make requirements-apt` only prints reviewed package
commands when a prerequisite is missing.

## Current snapshot

- Hanoi v3 includes bounded water, landuse, road hierarchy, and local Noto
  Sans Vietnamese labels.
- The matching local raster fallback has 847 tiles and rejects incompatible
  vector/style contracts.
- The Release matrix completed 30 direct-GPU runs across five scenarios and
  online/offline modes; every locked threshold passed.

That is reference-machine evidence, not a performance guarantee. See the
[benchmark guide](docs/benchmark/README.md) for methodology.

## Find details

| Need | Start here |
| --- | --- |
| Build, architecture, UI, and rules | [`context/`](context/) |
| Delivery status | [Progress tracker](context/progress-tracker.md) |
| Hanoi map contract | [`docs/map/`](docs/map/) |
| Benchmarks | [`docs/benchmark/`](docs/benchmark/) |
| Third-party notices | [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) |

## License and attribution

Map data uses OpenStreetMap attribution. See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
