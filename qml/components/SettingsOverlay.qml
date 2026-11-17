import QtQuick 2.13
import "../theme"

Item {
    id: settings
    property bool open: false
    signal toggled()
    function packageName(role) {
        if (role === "vector") return qsTr("Bản đồ vector")
        if (role === "raster") return qsTr("Raster ngoại tuyến")
        if (role === "search") return qsTr("Tìm kiếm ngoại tuyến")
        if (role === "routing") return qsTr("Định tuyến ngoại tuyến")
        return qsTr("Ảnh trực giao")
    }
    function recoveryText(item) {
        if (item.available) return ""
        if (item.required)
            return qsTr("Cần khôi phục gói bản đồ vector để khởi động bản đồ cục bộ.")
        if (item.role === "search")
            return qsTr("Tìm kiếm đang dùng dữ liệu demo cục bộ; kiểm tra hoặc cài lại gói tìm kiếm ngoại tuyến.")
        if (item.role === "orthography")
            return qsTr("Ảnh trực giao chưa khả dụng; cần gói dữ liệu được cấp phép và hợp lệ.")
        if (item.role === "routing")
            return qsTr("Chưa có graph định tuyến hợp lệ; tuyến thật dùng Valhalla cục bộ chưa sẵn sàng.")
        if (item.reason === "missing")
            return qsTr("Chưa cài gói raster ngoại tuyến; bản đồ vector vẫn khả dụng.")
        return qsTr("Gói không hợp lệ; kiểm tra lại dữ liệu cục bộ.")
    }
    KeyboardActionButton {
        id: button
        objectName: "settingsButton"
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: Theme.spaceMd
        width: 40; height: 40
        labelPixelSize: 22
        text: "⚙"
        Accessible.name: qsTr("Cài đặt ứng dụng")
        normalColor: Theme.surfaceElevated
        pressedColor: Theme.surfacePressed
        textColor: Theme.textPrimary
        onTriggered: {
            settings.open = !settings.open
            if (settings.open)
                panel.forceActiveFocus()
            else
                button.forceActiveFocus()
        }
    }
    Rectangle {
        visible: button.hovered && !settings.open
        anchors.right: button.left
        anchors.rightMargin: Theme.spaceXs
        anchors.verticalCenter: button.verticalCenter
        width: settingsHint.implicitWidth + Theme.spaceMd * 2
        height: 28
        radius: Theme.radiusPanel
        color: Theme.surfaceOpaque
        border.color: Theme.border
        z: 1
        Text {
            id: settingsHint
            anchors.centerIn: parent
            color: Theme.textPrimary
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeCaption
            text: qsTr("Cài đặt ứng dụng")
        }
    }
    Rectangle {
        id: panel
        objectName: "settingsPanel"
        visible: settings.open
        anchors.right: button.right
        anchors.top: button.bottom
        anchors.topMargin: Theme.spaceXs
        width: 230; height: column.implicitHeight + Theme.spaceLg * 2
        radius: Theme.radiusPanel
        color: Theme.surfaceOpaque
        border.color: Theme.border
        Keys.onEscapePressed: {
            settings.open = false
            button.forceActiveFocus()
        }
        focus: visible
        Column {
            id: column
            anchors.fill: parent; anchors.margins: Theme.spaceLg
            spacing: Theme.spaceSm
            Text { color: Theme.textPrimary; font.family: Theme.fontFamily; font.bold: true; text: qsTr("Ngôn ngữ") }
            KeyboardActionButton {
                width: parent.width
                text: appPreferences && appPreferences.language === "vi"
                      ? qsTr("✓ Tiếng Việt") : qsTr("Tiếng Việt")
                onTriggered: appPreferences.setLanguage("vi")
            }
            KeyboardActionButton {
                width: parent.width
                text: appPreferences && appPreferences.language === "en"
                      ? qsTr("✓ English") : qsTr("English")
                onTriggered: appPreferences.setLanguage("en")
            }
            Text { width: parent.width; color: Theme.textSecondary; font.family: Theme.fontFamily; text: qsTr("Dữ liệu cục bộ") }
            Text {
                width: parent.width
                color: Theme.textSecondary
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeCaption
                visible: packageReadiness && (packageReadiness.state === "loading" || packageReadiness.state === "timed-out")
                text: packageReadiness && packageReadiness.state === "timed-out" ? qsTr("Kiểm tra dữ liệu đã quá thời gian") : qsTr("Đang kiểm tra dữ liệu cục bộ…")
            }
            Repeater {
                model: packageReadiness ? packageReadiness.items : []
                delegate: Column {
                    width: parent.width
                    Text {
                        width: parent.width
                        color: modelData.available ? Theme.success : (modelData.required ? Theme.error : Theme.warning)
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontSizeCaption
                        text: (modelData.available ? "● " : "○ ") + settings.packageName(modelData.role)
                              + (modelData.available ? "" : (modelData.required ? qsTr(" — cần thiết") : qsTr(" — không bắt buộc")))
                    }
                    Text {
                        width: parent.width
                        visible: !modelData.available
                        wrapMode: Text.WordWrap
                        color: Theme.textSecondary
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontSizeCaption
                        text: settings.recoveryText(modelData)
                    }
                }
            }
        }
    }
}
