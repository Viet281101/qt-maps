import QtQuick 2.13
import "../theme"

Item {
    id: toolsOverlay
    property bool navigationActive: false
    property bool panelCollapsed: false
    property real panelWidth: 330
    property bool layersOpen: false

    Rectangle {
        id: searchPlaceholder
        objectName: "searchPlaceholder"
        visible: !toolsOverlay.navigationActive
        x: toolsOverlay.panelCollapsed
           ? Math.max(Theme.spaceXl, (toolsOverlay.width - 360) / 2)
           : toolsOverlay.panelWidth + Theme.spaceXl * 2
        y: Theme.spaceMd
        width: Math.max(240, Math.min(360, toolsOverlay.width - x - 210))
        height: 44
        radius: Theme.radiusControl
        color: Theme.surfaceElevated
        border.color: Theme.border

        Text {
            anchors.left: parent.left
            anchors.leftMargin: Theme.spaceLg
            anchors.verticalCenter: parent.verticalCenter
            color: Theme.textSecondary
            text: qsTr("⌕  Tìm địa điểm — sắp có trong 07F")
            elide: Text.ElideRight
        }
    }

    KeyboardActionButton {
        id: layerButton
        objectName: "layerButton"
        visible: !toolsOverlay.navigationActive
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.rightMargin: Theme.spaceMd
        anchors.topMargin: 58
        width: 176
        text: toolsOverlay.layersOpen ? qsTr("Đóng lớp bản đồ") : qsTr("Lớp: Bản đồ vector")
        normalColor: Theme.surfaceElevated
        pressedColor: Theme.surfacePressed
        textColor: Theme.textPrimary
        onTriggered: toolsOverlay.layersOpen = !toolsOverlay.layersOpen
    }

    Rectangle {
        visible: toolsOverlay.layersOpen && !toolsOverlay.navigationActive
        anchors.right: layerButton.right
        anchors.top: layerButton.bottom
        anchors.topMargin: Theme.spaceXs
        width: 220
        height: layerColumn.implicitHeight + Theme.spaceLg * 2
        radius: Theme.radiusPanel
        color: Theme.surfaceElevated
        border.color: Theme.border

        Column {
            id: layerColumn
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: Theme.spaceLg
            spacing: Theme.spaceSm

            Text { color: Theme.textPrimary; font.bold: true; text: qsTr("Bản đồ vector · đang dùng") }
            Text { color: Theme.warning; text: qsTr("Raster offline · chờ duyệt nguồn") }
            Text { color: Theme.warning; text: qsTr("Lớp ảnh · chưa có dữ liệu") }
            Text {
                width: parent.width
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSizeCaption
                wrapMode: Text.WordWrap
                text: qsTr("Mọi tài nguyên bản đồ hiện tại chỉ dùng loopback.")
            }
        }
    }
}
