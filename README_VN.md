# QtMaps

QtMaps là ứng dụng bản đồ desktop cho Việt Nam, được xây dựng bằng Qt 5.15.13, MapLibre Native Qt v3.0.0 liên kết động, C++ và QML/Qt Quick trên Linux. Qt 5.13.2 chỉ được giữ để đối chiếu. Dự án tập trung vào khả năng chạy local/offline, tìm đường ô tô và trình bày chế độ dẫn đường theo phong cách quen thuộc của các ứng dụng bản đồ hiện đại.

## Yêu cầu dự án

- Hiển thị bản đồ số Việt Nam từ dữ liệu OpenStreetMap.
- Hỗ trợ các lớp raster tile, vector và orthography khi nguồn dữ liệu và giấy phép cho phép.
- Hoạt động được khi có hoặc không có Internet sau khi dữ liệu cần thiết đã được chuẩn bị.
- Tìm đường ô tô giữa hai điểm, ưu tiên các nhóm đường quốc lộ và tỉnh lộ phù hợp với dữ liệu OSM thực tế.
- Hiển thị tuyến đường, tổng quan hành trình và danh sách chỉ dẫn rẽ.
- Có chế độ dẫn đường mô phỏng với camera bám vị trí, xoay theo hướng di chuyển, banner chỉ dẫn và khả năng re-route.
- Đo FPS render, tổng RAM và tổng CPU của ứng dụng cùng các service con trong Release build.
- Duy trì tiến độ và báo cáo tuần qua GitHub Projects cùng tài liệu trong repository.

Các nội dung không thuộc phạm vi hiện tại gồm voice guidance, giao thông thời gian thực, mobile/web, tài khoản người dùng và dịch vụ hosted ở mức production.

## Mục tiêu demo đầu tiên

Deadline demo đầu tiên với quản lý là **03/11/2026**. Trong hai tuần chuẩn bị, dự án ưu tiên một luồng demo xuyên suốt, ổn định và có hình thức tốt:

1. Khởi động Release build và hiển thị bản đồ thật tại khu vực demo ở Việt Nam.
2. Giao diện map-first rõ ràng, có trạng thái online/offline, layer, route và performance.
3. Chọn hai điểm, vẽ tuyến ô tô, hiển thị thông tin tuyến và các bước chỉ dẫn.
4. Chuyển sang giao diện navigation, mô phỏng di chuyển và hiển thị chỉ dẫn tiếp theo.
5. Hiển thị FPS và cung cấp số đo CPU/RAM có phương pháp đo rõ ràng trên máy tham chiếu.
6. Bổ sung offline fallback, vector, orthography và tìm kiếm theo thứ tự ưu tiên khi luồng chính đã ổn định.

Phạm vi dữ liệu toàn quốc ở độ chi tiết cao, pipeline dữ liệu hoàn chỉnh, đóng gói installer, soak test dài và production hardening có thể được lùi sau demo nếu ảnh hưởng tới luồng chính. Tính năng dùng mô phỏng, fixture, Internet, phạm vi dữ liệu giới hạn hoặc chưa sẵn sàng phải được ghi nhãn trung thực khi trình diễn.

## Định hướng kỹ thuật

| Thành phần     | Định hướng hiện tại                                            |
| -------------- | -------------------------------------------------------------- |
| UI và runtime  | Qt 5.15.13, MapLibre Native Qt v3.0.0, C++, QML, Qt Quick      |
| Build          | CMake và Ninja với Qt 5 CMake APIs                             |
| Raster         | Fallback diagnostic localhost đã xác nhận; raster thật để sau  |
| Vector         | MapLibre Native Qt v3.0.0 đã duyệt, dùng MBTiles local hợp lệ  |
| Dữ liệu bản đồ | Package Hà Nội có version từ Geofabrik PBF/Planetiler đã duyệt |
| Routing        | Valhalla chạy local qua `QProcess` đang là phương án đề xuất   |
| Search offline | SQLite FTS5 đang là phương án đề xuất                          |
| GPS mô phỏng   | QtPositioning và route/NMEA replay                             |
| Hiệu năng      | FPS từ frame render; CPU/RAM tính cả application process tree  |

Các công nghệ mang trạng thái đề xuất không được xem là đã chốt cho đến khi có bằng chứng build/chạy trên Qt 5.15.13 và máy tham chiếu.

## Build và chạy

Workflow Release đã kiểm chứng chọn rõ Qt 5.15.13 của hệ thống và không đổi `PATH` toàn cục:

```bash
make requirements
make build
make test
```

MapLibre development install và các Qt QML/runtime plugin hiện là dependency local nằm ngoài Git. Lệnh smoke local đã chạy thành công dưới đây mở package Hà Nội có checksum trong 15 giây; evidence Unit 03 bọc lệnh này trong network namespace dùng cho kiểm chứng mất mạng:

```bash
LD_LIBRARY_PATH="$PWD/generated/unit-01b/maplibre-install/lib" \
QT_PLUGIN_PATH="$PWD/generated/unit-01b/maplibre-install/plugins:$PWD/generated/unit-01b/qml-runtime/usr/lib/x86_64-linux-gnu/qt5/plugins" \
QML2_IMPORT_PATH="$PWD/generated/unit-01b/maplibre-install/qml:$PWD/generated/unit-01b/qml-runtime/usr/lib/x86_64-linux-gnu/qt5/qml" \
QSG_RENDER_LOOP=threaded \
./build-qt515/qtmaps --unit03-local-map --navigation-benchmark \
  --map-package data/packages/current --exit-after 15 \
  --network-mode network-namespace-offline \
  --dataset-id hanoi-20261024-v1 \
  --dataset-coverage "Hanoi bounds 105.78,20.97,105.90,21.10; z12-z15"
```

Benchmark 120 giây đã ghi nhận dùng cùng lệnh và các biến runtime trên, thay `--exit-after 15` bằng:

```bash
--benchmark-output /tmp/qtmaps-unit03.csv \
--benchmark-warmup 10 --benchmark-duration 120 --benchmark-interval 1000 \
--scenario unit03-navigation
```

Chỉ tái tạo package có version nằm ngoài Git từ hai input local đã được duyệt:

```bash
make hanoi-package \
  HANOI_PBF=generated/unit-01b/source/vietnam-latest.osm.pbf \
  PLANETILER_JAR=generated/unit-01b/tools/planetiler.jar
```

Quy trình kiểm chứng mất mạng hoàn toàn và benchmark đầy đủ nằm trong [evidence Unit 03](docs/benchmarks/unit-03-local-map-data-pipeline.md).

Có thể chạy demo routing từ bất kỳ thư mục nào bằng:

```bash
/duong/dan/tuyet/doi/toi/QtMaps/tools/run-demo.sh
```

Script tự chuyển về thư mục gốc repo và truyền đường dẫn tuyệt đối cho map,
service và config. Nếu thiếu file Valhalla, UI hiện đúng đường dẫn bị thiếu và
không âm thầm vẽ tuyến fixture.

## Nguyên tắc triển khai

- Chỉ dùng API tương thích Qt 5.15.13; không dùng API chỉ có trên Qt 6.
- CMake phải dùng `find_package(Qt5 ...)`, `CMAKE_AUTOMOC`/`CMAKE_AUTORCC`, `qt5_add_resources` và `add_executable`.
- Không thực hiện I/O, routing, database hoặc chờ process theo kiểu blocking trên UI thread.
- Dữ liệu sinh ra, cache, routing graph, MBTiles và artifact bên thứ ba không được commit vào Git.
- Các số liệu trình bày với quản lý phải lấy từ Release build và ghi rõ phần process, dataset, thời lượng, kích thước cửa sổ và trạng thái mạng.
- Ưu tiên hoàn thành một vertical slice ổn định trước khi mở rộng số lượng tính năng.

## Phạm vi dữ liệu dự kiến

- Raster/vector toàn Việt Nam ở zoom 0–10.
- Hà Nội và khu vực Đồng bằng sông Hồng ở zoom 11–16; zoom 16 vẫn cần được chứng minh về dung lượng và hiệu năng.
- Routing ban đầu dùng một bản trích xuất Geofabrik Việt Nam chưa cắt vùng, sau đó đánh giá lại bằng thời gian build graph và peak RAM.
- Orthography chỉ áp dụng cho vùng demo và zoom được giới hạn, với nguồn dữ liệu có giấy phép phù hợp.

## Hiệu năng mục tiêu ban đầu

Các con số sau là mục tiêu lập kế hoạch, chưa phải kết quả đo:

- Trung bình ít nhất 30 FPS trong kịch bản tương tác.
- Idle CPU không quá 25% của một logical CPU.
- Interactive CPU không quá 150% theo cách báo cáo process-level trên Linux.
- Tổng resident memory của QtMaps và các child service không quá 2 GiB sau 30 phút.

Kết quả thực tế được ghi theo [mẫu benchmark](docs/benchmarks/benchmark-template.md).

## Trạng thái hiện tại

- Qt 5.15.13 hệ thống và MapLibre Native Qt v3.0.0 liên kết động là baseline đã duyệt; Qt 5.13.2 tại `/home/viet/Qt/5.13.2/gcc_64` được giữ để đối chiếu.
- CMake/Ninja/ccache Release build và CTest đã được xác nhận. Việc chọn Qt dùng `CMAKE_PREFIX_PATH` hoặc biến Makefile, không đổi `PATH` toàn cục.
- Package vector có checksum hiện chỉ bao phủ vùng Hà Nội giới hạn ở zoom 12–15; chưa phải dữ liệu toàn quốc.
- PR #20 đã merge. Tuyến 5,47 km và chuyển động 40 km/h của Unit 03 vẫn là fixture navigation mô phỏng xác định trước, chưa phải kết quả từ routing engine.
- Luồng lập tuyến Unit 04 hiện chạy Valhalla 3.9.1 local do ứng dụng quản lý, chỉ bind loopback, và trả về geometry, quãng đường, thời gian cùng chỉ dẫn tiếng Việt thật cho routing graph vùng đại Hà Nội giới hạn. Fixture ghi sẵn chỉ dùng trong chế độ fixture có nhãn rõ hoặc fallback khi request lỗi được hiển thị; thiếu file khởi động không kích hoạt fixture.
- Raster fallback chỉ là ảnh chẩn đoán local để kiểm tra phát hiện lỗi và chuyển nguồn, chưa phải raster Hà Nội thật.
- Navigation replay theo tuyến đã chọn đã có tốc độ 1x/2x/4x, tạm dừng/dừng,
  camera follow/recenter, xoay/nghiêng, banner maneuver có nhãn nguồn, quãng đường/thời
  gian còn lại và trạng thái đến đích. Có thể mô phỏng đi lệch trên 40 m để yêu
  cầu Valhalla tính lại tuyến; chưa có phát hiện lệch bằng GPS thật. Benchmark
  toàn cây tiến trình QtMaps + Valhalla, routing graph toàn quốc, search offline, orthography, UI sản
  phẩm hoàn chỉnh, installer và kiểm chứng ổn định dài hạn chưa được triển khai.

Trạng thái chi tiết và bước tiếp theo nằm trong [progress tracker](context/progress-tracker.md). Kiến trúc, UI, coding standards và workflow nằm trong thư mục [`context/`](context/).

## Cấu trúc repository

```text
app/                 entry point và lifecycle
qml/                 màn hình, control, overlay và theme
src/core/            domain types, cấu hình và lifecycle abstractions
src/map/             map engine, layer và tile access
src/routing/         routing client, route model và polyline decoder
src/search/          offline search và model
src/perf/            FPS, process metrics và CSV logger
tools/data-pipeline/ chuẩn bị dữ liệu có thể tái lập
tests/               Qt Test và Qt Quick Test
docs/                benchmark và báo cáo tuần
context/             nguồn sự thật về phạm vi và quyết định dự án
```

Cách build đối chiếu Qt 5.13.2 và cấu hình runtime MapLibre được ghi trong [code standards](context/code-standards.md) và [evidence Unit 01b](docs/benchmarks/unit-01b-vector-feasibility.md).

Attribution bên thứ ba và kế hoạch đóng gói dynamic link đã duyệt được ghi trong [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) và [hướng dẫn đóng gói Linux](docs/packaging/linux-dynamic-linking.md).
