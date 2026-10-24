# QtMaps Benchmark Record

Copy this template for each benchmark session. Report project performance only from a Release build. Do not replace missing values with estimates; use `Not measured` and explain why.

## Run Identity

| Field | Value |
| --- | --- |
| Date and local timezone | TODO |
| Operator | TODO |
| Git commit | TODO |
| Build type | Release |
| Configure/build command | `make build` (CMake/Ninja Release wrapper) |
| Qt version and source | TODO |
| Scenario | TODO |
| Duration and warm-up | TODO |
| Window/viewport size | TODO |
| Online/offline mode | TODO |
| Dataset ID, date, coverage, zoom | TODO |
| App PID and included child services | TODO |
| CSV schema version and file | TODO |

## Reference Configuration

| Component | Reference value |
| --- | --- |
| Machine | Dell Latitude 5480 |
| OS | Linux; exact distribution/build TODO |
| CPU | Intel Core i5-6300U, 2 cores/4 threads |
| RAM | 16 GiB |
| Storage | 512 GB NVMe SSD |
| Compiler | GCC/G++ 13.3.0 |
| Build tools currently present | CMake 3.28.3; GNU Make 4.3; Ninja 1.11.1; ccache 4.9.1 |
| Tools intentionally absent from `PATH` | qmake/Qt (use the selected absolute Qt prefix); Clang |
| GPU | Intel HD Graphics 520 (Skylake GT2) |
| Driver | Mesa 25.2.8 |
| Direct rendering | Yes |
| Maximum reported OpenGL | 4.6 |

Record any difference from the reference configuration:

- TODO

## Method

| Item | Value |
| --- | --- |
| FPS definition and sampler | TODO |
| RAM definition and sampler | TODO; total process-tree RSS expected |
| CPU definition, normalization, and sampler | TODO; state whether 100% means one logical CPU |
| Sampling interval | TODO |
| Cache state | TODO |
| Network conditions | TODO |
| User/scripted actions | TODO |
| Instrumentation enabled | TODO |

## Results

| Scenario | Avg FPS | FPS low/percentile | Avg CPU % | Peak CPU % | Avg RSS MiB | Peak RSS MiB | Child count | Notes |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| Idle | TODO | TODO | TODO | TODO | TODO | TODO | TODO | TODO |
| Urban pan/zoom | TODO | TODO | TODO | TODO | TODO | TODO | TODO | TODO |
| Long-route navigation | TODO | TODO | TODO | TODO | TODO | TODO | TODO | TODO |
| Layer change | TODO | TODO | TODO | TODO | TODO | TODO | TODO | TODO |
| Online | TODO | TODO | TODO | TODO | TODO | TODO | TODO | TODO |
| Offline | TODO | TODO | TODO | TODO | TODO | TODO | TODO | TODO |
| 30-to-60-minute soak | TODO | TODO | TODO | TODO | TODO | TODO | TODO | TODO |

## Observations and Conclusion

- Rendering anomalies or dropped-frame pattern: TODO
- Memory growth over time: TODO
- CPU hotspots or child-service behavior: TODO
- Errors, retries, or fallbacks: TODO
- Comparison with previous run: TODO
- Result against the current proposed targets: TODO
- Follow-up action or decision: TODO
