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
        width: Math.max(240, Math.min(360, toolsOverlay.width - x
                                     - (toolsOverlay.width < 1100 ? 280 : 210)))
        height: 44
        radius: Theme.radiusControl
        color: Theme.surfaceElevated
        border.color: Theme.border

        Text {
            anchors.left: parent.left
            anchors.leftMargin: Theme.spaceLg
            anchors.verticalCenter: parent.verticalCenter
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSizeBody
            text: qsTr("⌕  Tìm địa điểm (sắp có)")
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
        text: toolsOverlay.layersOpen
              ? qsTr("Đóng lớp bản đồ")
              : mapSourceController.layerMode === "raster"
                ? qsTr("Lớp: Raster ngoại tuyến")
                : qsTr("Lớp: Bản đồ vector")
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

            KeyboardActionButton {
                objectName: "vectorLayerButton"
                width: parent.width
                text: mapSourceController.layerMode === "vector"
                      ? qsTr("✓ Bản đồ vector") : qsTr("Bản đồ vector")
                enabled: !mapSourceController.loading
                         && mapSourceController.layerMode !== "vector"
                onTriggered: mapSourceController.activateVector()
            }
            KeyboardActionButton {
                objectName: "rasterLayerButton"
                width: parent.width
                text: mapSourceController.layerMode === "raster"
                      ? qsTr("✓ Raster ngoại tuyến") : qsTr("Raster ngoại tuyến")
                enabled: mapSourceController.rasterConfigured
                         && !mapSourceController.loading
                         && mapSourceController.layerMode !== "raster"
                onTriggered: mapSourceController.activateRaster()
            }
            Text {
                width: parent.width
                color: mapSourceController.layerMode === "error" ? Theme.error
                                                                  : Theme.textSecondary
                font.pixelSize: Theme.fontSizeBody
                wrapMode: Text.WordWrap
                text: mapSourceController.mapStatusText
            }
            Text {
                visible: mapSourceController.layerMode === "raster"
                width: parent.width
                color: Theme.warning
                font.pixelSize: Theme.fontSizeCaption
                wrapMode: Text.WordWrap
                text: qsTr("Nguồn chi tiết đến z15; từ z16, độ phân giải thấp hơn.")
            }
            Text {
                color: Theme.warning
                font.pixelSize: Theme.fontSizeBody
                text: qsTr("Lớp ảnh · chưa có dữ liệu")
            }
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
