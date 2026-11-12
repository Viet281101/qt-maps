# QtMaps

**Bản demo bản đồ và dẫn đường desktop, ưu tiên chạy local cho Hà Nội.**

QtMaps kết hợp Qt/QML, dữ liệu bản đồ local, tìm đường ô tô và dẫn đường mô
phỏng. Dữ liệu local, fixture và phần chưa hoàn chỉnh đều được ghi rõ.

[English](README.md) · [Project board](https://github.com/users/Viet281101/projects/6) · [Lộ trình](context/project-overview.md)

## Có gì hoạt động

| Khả năng | Trạng thái hiện tại |
| --- | --- |
| Bản đồ vector Hà Nội | Dữ liệu local giới hạn, có nhãn tiếng Việt |
| Raster fallback | Fallback local để kiểm tra phục hồi nguồn |
| Tìm đường ô tô | Valhalla local cho graph Hà Nội |
| Dẫn đường | Replay xác định trước, chỉ dẫn và re-route |
| Hiệu năng | FPS, CPU và RAM từ Release build |

Dữ liệu chỉ phủ **một vùng Hà Nội giới hạn**, không phải toàn quốc. GPS thật,
giao thông trực tuyến, voice guidance, mobile/web, dịch vụ hosted, tìm kiếm
POI và soak test dài chưa phải tính năng sản phẩm.

## Bắt đầu nhanh

QtMaps dùng **Qt 5.15.18**, CMake, Ninja, C++, QML/Qt Quick và MapLibre Native
Qt 3.0.0 trên Linux. Qt 5.15.13 chỉ dùng để đối chiếu.

```bash
make requirements
make test
make run
```

Dùng `make check` khi phát triển và `make review` trước PR với worktree sạch.
`make requirements-apt` chỉ in lệnh cài đặt đã rà soát, không tự chạy lệnh đó.

## Ảnh chụp trạng thái

- Hanoi v3 có water, landuse, phân cấp đường và Noto Sans local cho nhãn Việt.
- Raster fallback tương ứng có 847 tile local và từ chối contract không khớp.
- Release matrix đã chạy 30 lượt GPU trực tiếp ở năm scenario và hai mode;
  mọi ngưỡng đã khóa đều pass.

Đây là evidence trên máy tham chiếu, không phải cam kết cho mọi máy. Xem
[benchmark guide](docs/benchmark/README.md) để biết phương pháp.

## Tìm tài liệu chi tiết

| Bạn cần | Xem tại |
| --- | --- |
| Build, kiến trúc, UI và quy ước | [`context/`](context/) |
| Tiến độ delivery | [Progress tracker](context/progress-tracker.md) |
| Contract bản đồ Hà Nội | [`docs/map/`](docs/map/) |
| Benchmark | [`docs/benchmark/`](docs/benchmark/) |
| Ghi chú bên thứ ba | [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) |

## Giấy phép và attribution

Dữ liệu bản đồ dùng attribution OpenStreetMap. Xem [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
