import QtQuick 2.13
import "../theme"

Item {
    id: toolsOverlay
    property bool navigationActive: false
    property bool panelCollapsed: false
    property real panelWidth: 330
    property bool layersOpen: false
    readonly property real statusSafeTop: layersOpen
                                       ? layerMenu.y + layerMenu.height + Theme.spaceMd
                                       : layerButton.y + layerButton.height + Theme.spaceMd

    Rectangle {
        id: searchPlaceholder
        objectName: "searchPlaceholder"
        visible: !toolsOverlay.navigationActive
        x: toolsOverlay.panelCollapsed
           ? Math.max(Theme.spaceXl, (toolsOverlay.width - 360) / 2)
           : toolsOverlay.panelWidth + Theme.spaceXl * 2
        y: Theme.spaceMd
        width: Math.max(0, Math.min(360, toolsOverlay.width - x - Theme.spaceMd))
        height: 40
        radius: Theme.radiusControl
        color: Theme.surfaceElevated
        border.color: Theme.border
        clip: true

        Item {
            id: searchIcon
            objectName: "searchIcon"
            anchors.left: parent.left
            anchors.leftMargin: Theme.spaceLg
            anchors.verticalCenter: parent.verticalCenter
            width: 24
            height: 24

            Rectangle {
                width: 13
                height: 13
                radius: 7
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.leftMargin: 2
                anchors.topMargin: 2
                color: "transparent"
                border.color: Theme.accent
                border.width: 2
            }
            Rectangle {
                width: 10
                height: 2
                radius: 1
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.rightMargin: 2
                anchors.bottomMargin: 4
                rotation: 45
                color: Theme.accent
            }
        }

        Text {
            objectName: "searchPlaceholderLabel"
            anchors.left: searchIcon.right
            anchors.right: parent.right
            anchors.leftMargin: Theme.spaceSm
            anchors.rightMargin: Theme.spaceLg
            anchors.verticalCenter: parent.verticalCenter
            color: Theme.textSecondary
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeBody
            text: qsTr("Tìm kiếm · sắp có")
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
        id: layerMenu
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
