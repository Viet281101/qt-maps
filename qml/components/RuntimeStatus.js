.pragma library

// Controller messages are kept in Vietnamese for existing API consumers.
// Translate them at the UI boundary so changing language updates visible text.
function text(message, language) {
    if (!message || language !== "en")
        return message || ""

    switch (message) {
    case "Bản đồ vector · dữ liệu cục bộ": return qsTr("Bản đồ vector · dữ liệu cục bộ")
    case "Raster chẩn đoán · không phải dữ liệu sản phẩm": return qsTr("Raster chẩn đoán · không phải dữ liệu sản phẩm")
    case "Bản đồ chưa sẵn sàng": return qsTr("Bản đồ chưa sẵn sàng")
    case "Đang mở raster dự phòng…": return qsTr("Đang mở raster dự phòng…")
    case "Đang mở raster ngoại tuyến…": return qsTr("Đang mở raster ngoại tuyến…")
    case "Chưa cấu hình gói raster ngoại tuyến": return qsTr("Chưa cấu hình gói raster ngoại tuyến")
    case "Raster ngoại tuyến · tự động dự phòng": return qsTr("Raster ngoại tuyến · tự động dự phòng")
    case "Raster ngoại tuyến · chọn thủ công": return qsTr("Raster ngoại tuyến · chọn thủ công")
    case "Không mở được raster ngoại tuyến": return qsTr("Không mở được raster ngoại tuyến")
    case "Đang mở ảnh trực giao cục bộ…": return qsTr("Đang mở ảnh trực giao cục bộ…")
    case "Chưa có gói ảnh trực giao được cấp phép": return qsTr("Chưa có gói ảnh trực giao được cấp phép")
    case "Ảnh trực giao 10 m · cục bộ": return qsTr("Ảnh trực giao 10 m · cục bộ")
    case "Gói ảnh trực giao không hợp lệ hoặc không sẵn sàng": return qsTr("Gói ảnh trực giao không hợp lệ hoặc không sẵn sàng")
    case "Bản đồ vector không sẵn sàng": return qsTr("Bản đồ vector không sẵn sàng")

    case "Tìm trong dữ liệu demo cục bộ": return qsTr("Tìm trong dữ liệu demo cục bộ")
    case "Đang tìm trong dữ liệu demo cục bộ…": return qsTr("Đang tìm trong dữ liệu demo cục bộ…")
    case "Không có địa điểm demo phù hợp": return qsTr("Không có địa điểm demo phù hợp")
    case "Kết quả từ dữ liệu demo cục bộ": return qsTr("Kết quả từ dữ liệu demo cục bộ")
    case "Gói địa điểm Hà Nội · OpenStreetMap (ODbL)": return qsTr("Gói địa điểm Hà Nội · OpenStreetMap (ODbL)")
    case "Dữ liệu demo cục bộ · gói ngoại tuyến chưa sẵn sàng": return qsTr("Dữ liệu demo cục bộ · gói ngoại tuyến chưa sẵn sàng")
    case "Dữ liệu demo cục bộ · gói ngoại tuyến lỗi": return qsTr("Dữ liệu demo cục bộ · gói ngoại tuyến lỗi")
    case "Không có địa điểm ngoại tuyến phù hợp": return qsTr("Không có địa điểm ngoại tuyến phù hợp")
    case "Kết quả từ gói địa điểm Hà Nội ngoại tuyến": return qsTr("Kết quả từ gói địa điểm Hà Nội ngoại tuyến")
    case "Tìm trong gói địa điểm Hà Nội ngoại tuyến": return qsTr("Tìm trong gói địa điểm Hà Nội ngoại tuyến")

    case "Chọn hai điểm rồi bấm Tính đường": return qsTr("Chọn hai điểm rồi bấm Tính đường")
    case "Đã tính tuyến bằng Valhalla local": return qsTr("Đã tính tuyến bằng Valhalla local")
    case "Đã tính lại tuyến bằng Valhalla local": return qsTr("Đã tính lại tuyến bằng Valhalla local")
    case "Điểm đi không hợp lệ; dùng tên mẫu hoặc vĩ độ, kinh độ": return qsTr("Điểm đi không hợp lệ; dùng tên mẫu hoặc vĩ độ, kinh độ")
    case "Điểm đến không hợp lệ; dùng tên mẫu hoặc vĩ độ, kinh độ": return qsTr("Điểm đến không hợp lệ; dùng tên mẫu hoặc vĩ độ, kinh độ")
    case "Đã cập nhật điểm đi": return qsTr("Đã cập nhật điểm đi")
    case "Đã cập nhật điểm đến": return qsTr("Đã cập nhật điểm đến")
    case "Không thể chọn tọa độ bản đồ này": return qsTr("Không thể chọn tọa độ bản đồ này")
    case "Đang tính tuyến bằng Valhalla local…": return qsTr("Đang tính tuyến bằng Valhalla local…")
    case "Đang tính lại tuyến bằng Valhalla local…": return qsTr("Đang tính lại tuyến bằng Valhalla local…")
    case "Không thể tính lại tuyến: vị trí lệch không hợp lệ": return qsTr("Không thể tính lại tuyến: vị trí lệch không hợp lệ")
    case "Không thể tính lại tuyến: Valhalla chưa sẵn sàng": return qsTr("Không thể tính lại tuyến: Valhalla chưa sẵn sàng")
    case "Cần đủ điểm đi và điểm đến hợp lệ": return qsTr("Cần đủ điểm đi và điểm đến hợp lệ")
    case "Không thể đọc route fixture đã đóng gói": return qsTr("Không thể đọc route fixture đã đóng gói")
    case "Đã nạp tuyến fixture offline": return qsTr("Đã nạp tuyến fixture offline")
    case "Bước chỉ đường không hợp lệ": return qsTr("Bước chỉ đường không hợp lệ")

    case "Chưa có tuyến để chỉ đường": return qsTr("Chưa có tuyến để chỉ đường")
    case "Không thể phát lại hình học tuyến": return qsTr("Không thể phát lại hình học tuyến")
    case "Tuyến thiếu quãng đường hoặc thời gian phát lại": return qsTr("Tuyến thiếu quãng đường hoặc thời gian phát lại")
    case "Tuyến có maneuver ngoài hình học": return qsTr("Tuyến có maneuver ngoài hình học")
    case "Không thể bắt đầu: chưa có tuyến hợp lệ": return qsTr("Không thể bắt đầu: chưa có tuyến hợp lệ")
    case "Đã tạm dừng chỉ đường": return qsTr("Đã tạm dừng chỉ đường")
    case "Đã dừng chỉ đường": return qsTr("Đã dừng chỉ đường")
    case "Tốc độ phát lại chỉ hỗ trợ 1x, 2x hoặc 4x": return qsTr("Tốc độ phát lại chỉ hỗ trợ 1x, 2x hoặc 4x")
    case "Đã phát hiện lệch tuyến; đang tính lại tuyến": return qsTr("Đã phát hiện lệch tuyến; đang tính lại tuyến")
    case "Không thể mô phỏng điểm lệch quá 40 m": return qsTr("Không thể mô phỏng điểm lệch quá 40 m")
    case "Đã đến đích": return qsTr("Đã đến đích")
    case "đang khởi động Valhalla local": return qsTr("đang khởi động Valhalla local")
    case "đang dừng Valhalla local": return qsTr("đang dừng Valhalla local")
    case "dịch vụ chưa sẵn sàng": return qsTr("dịch vụ chưa sẵn sàng")
    case "Không tìm thấy gói tìm kiếm ngoại tuyến": return qsTr("Không tìm thấy gói tìm kiếm ngoại tuyến")
    case "Gói tìm kiếm ngoại tuyến không hợp lệ": return qsTr("Gói tìm kiếm ngoại tuyến không hợp lệ")
    case "Gói tìm kiếm ngoại tuyến không sẵn sàng": return qsTr("Gói tìm kiếm ngoại tuyến không sẵn sàng")
    }

    if (message.indexOf("Sẵn sàng chỉ đường • ") === 0)
        return qsTr("Ready to navigate • %1").arg(source(message.substring("Sẵn sàng chỉ đường • ".length)))
    if (message.indexOf("Đang chỉ đường ") === 0) {
        var match = /^Đang chỉ đường (\d+x) • (.+)$/.exec(message)
        if (match)
            return qsTr("Navigating at %1 • %2").arg(match[1]).arg(source(match[2]))
    }
    if (message.indexOf("Đã chọn trên bản đồ; tiếp theo chọn ") === 0)
        return qsTr("Selected on map; next choose %1").arg(
                    message.indexOf("điểm đi") >= 0 ? qsTr("origin") : qsTr("destination"))
    if (message.indexOf("Tìm trong dữ liệu demo cục bộ; ") === 0)
        return qsTr("Search local demo data; %1").arg(
                    text(message.substring("Tìm trong dữ liệu demo cục bộ; ".length), language))
    if (message.indexOf("Valhalla chưa chạy: ") === 0)
        return qsTr("Valhalla is not running: %1").arg(
                    text(message.substring("Valhalla chưa chạy: ".length), language))
    if (message.indexOf("Không thể tính lại tuyến: ") === 0)
        return qsTr("Cannot recalculate route: %1").arg(message.substring("Không thể tính lại tuyến: ".length))
    return message
}

function source(value) {
    return value === "fixture/mô phỏng" ? qsTr("simulated fixture") : value
}
