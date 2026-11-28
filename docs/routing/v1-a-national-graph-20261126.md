# V1-A nationwide graph, first build and route proof

On 2026-11-26 the native Valhalla 3.9.1 builder used the approved **uncropped**
Geofabrik Vietnam PBF, SHA-256
`4e5243c3116dc089a298f4cd7218e1257adf4b48355e42936360df5ce3200d43`.
The machine had about 10 GiB available RAM and 102 GiB free disk at preflight.
`tools/routing/build_nationwide_graph.py` copied the verified Unit 04 config,
changed only the graph directory, worker count (2), and loopback port. The
generated config SHA-256 was
`b031be45bf0b787a16b87fedd2b326fb5a3d1428707ad03e9959bc18fd5d3a60`.

The build exited 0 in 6:01.84 wall time with peak RSS 4,242,968 KiB. It made
734 `.gph` files, 1,164,439,264 bytes. Raw config, build logs, time output,
tiles and route responses remain under ignored
`generated/v1-a-national-graph-20261126/` in the main workspace. These
measurements describe the build, not an application performance benchmark.

The frozen matrix was run offline through the local `valhalla_service` binary
using `tools/routing/verify_nationwide_graph.py`. Each successful route had
nonempty encoded geometry and maneuvers:

| Route | Length | Maneuvers | Road evidence in response |
| --- | ---: | ---: | --- |
| Hanoi–Hai Phong | 126.714 km | 33 | CT.04 |
| Hue–Da Nang | 94.746 km | 23 | QL.1 |
| Ho Chi Minh City–Can Tho | 161.770 km | 18 | QL.91B |
| Thai Nguyen–Bac Kan | 87.902 km | 13 | QL.3, QL.37 |
| Thu Dau Mot–Di An (supplemental provincial probe) | 17.905 km | 18 | ĐT.743A / Đường tỉnh 743A |

A request from (0, 0) to (0.01, 0.01) returned `No suitable edges near
location` with no route. The supplemental route establishes provincial-road traversal from the
maneuver road names. Next activate this graph through the Unit 12/13
application path for V1-B. Do not represent this CLI proof as
end-to-end application navigation evidence.
