# QtMaps

[English](README.md) · [Bảng tiến độ dự án](https://github.com/users/Viet281101/projects/6) · [Milestone demo đầu tiên](https://github.com/Viet281101/qt-maps/milestone/1)

QtMaps là ứng dụng bản đồ desktop cho Việt Nam, được xây dựng bằng Qt 5.13.2, C++ và QML/Qt Quick 2.13 trên Linux. Dự án tập trung vào khả năng chạy local/offline, tìm đường ô tô và trình bày chế độ dẫn đường theo phong cách quen thuộc của các ứng dụng bản đồ hiện đại.

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
| UI và runtime  | Qt 5.13.2, C++, QML, Qt Quick 2.13                             |
| Build          | CMake và Ninja với Qt 5 CMake APIs                             |
| Raster         | QtLocation `osm`, cần xác nhận bằng spike chạy thật            |
| Vector         | MapLibre Native/Qt integration, chỉ chốt sau spike tương thích |
| Dữ liệu bản đồ | Geofabrik Vietnam OSM PBF; MBTiles cho dữ liệu tile local      |
| Routing        | Valhalla chạy local qua `QProcess` đang là phương án đề xuất   |
| Search offline | SQLite FTS5 đang là phương án đề xuất                          |
| GPS mô phỏng   | QtPositioning và route/NMEA replay                             |
| Hiệu năng      | FPS từ frame render; CPU/RAM tính cả application process tree  |

Các công nghệ mang trạng thái đề xuất không được xem là đã chốt cho đến khi có bằng chứng build/chạy trên Qt 5.13.2 và máy tham chiếu.

## Nguyên tắc triển khai

- Chỉ dùng API tương thích Qt 5.13; không dùng Qt 5.15 hoặc Qt 6 APIs.
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

- Qt 5.13.2 `gcc_64` đã được cài tại `/home/viet/Qt/5.13.2/gcc_64`.
- Qt Quick, QtLocation, QtPositioning và các plugin `osm`, `mapboxgl`, `serialnmea` đã có trên disk nhưng vẫn cần probe load/render thực tế.
- CMake 3.28.3 và GCC/G++ 13.3.0 đã có; Ninja và ccache chưa có trên `PATH` và phải do owner cài bằng lệnh đã duyệt.
- Qt 5.13.2 đang có cảnh báo tương thích OpenSSL 3; đường HTTPS thực tế chưa được xác nhận.
- Application code và build command chính thức chưa được tạo/xác nhận.

Trạng thái chi tiết và bước tiếp theo nằm trong [progress tracker](context/progress-tracker.md). Kiến trúc, UI, coding standards và workflow nằm trong thư mục [`context/`](context/).

## Cấu trúc dự kiến

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

Build và run command sẽ được bổ sung sau khi Unit 01 hoàn thành CMake/Ninja validation; repository không công bố một lệnh chưa được kiểm chứng.
