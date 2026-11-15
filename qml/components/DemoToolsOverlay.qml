import QtQuick 2.13
import "../theme"

Item {
    id: toolsOverlay
    objectName: "demoToolsOverlay"
    property bool navigationActive: false
    property bool panelCollapsed: false
    property real panelWidth: 330
    property bool layersOpen: false
    property var searchController: null
    readonly property real layerControlsBottom: layersOpen
                                               ? layerMenu.y + layerMenu.height
                                               : layerButton.y + layerButton.height

    function selectSearchResult(coordinate, setOrigin) {
        var accepted = setOrigin
                       ? routingController.setOriginInput(coordinate)
                       : routingController.setDestinationInput(coordinate)
        if (accepted)
            searchInput.text = ""
    }

    Rectangle {
        id: placeSearchControl
        objectName: "placeSearchControl"
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

        TextInput {
            id: searchInput
            objectName: "placeSearchInput"
            anchors.left: searchIcon.right
            anchors.right: parent.right
            anchors.leftMargin: Theme.spaceSm
            anchors.rightMargin: Theme.spaceLg
            anchors.verticalCenter: parent.verticalCenter
            color: text.length > 0 ? Theme.textPrimary : Theme.textSecondary
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeBody
            clip: true
            selectByMouse: true
            activeFocusOnTab: true
            text: ""
            onTextChanged: {
                if (toolsOverlay.searchController)
                    toolsOverlay.searchController.setQuery(text)
            }
            Text {
                anchors.fill: parent
                verticalAlignment: Text.AlignVCenter
                color: Theme.textSecondary
                font: parent.font
                text: qsTr("Tìm địa điểm Hà Nội")
                visible: !parent.text && !parent.activeFocus
            }
        }
    }

    Rectangle {
        id: searchResults
        objectName: "placeSearchResults"
        visible: !toolsOverlay.navigationActive && toolsOverlay.searchController
                 && toolsOverlay.searchController.state !== "idle"
        x: placeSearchControl.x
        y: placeSearchControl.y + placeSearchControl.height + Theme.spaceXs
        width: placeSearchControl.width
        height: Math.min(searchScroller.contentHeight + Theme.spaceMd * 2,
                         Math.min(360, Math.max(0, toolsOverlay.height - y - Theme.spaceMd)))
        radius: Theme.radiusControl
        color: Theme.surfaceElevated
        border.color: Theme.border
        z: 2

        Flickable {
            id: searchScroller
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.margins: Theme.spaceMd
            contentWidth: width
            contentHeight: searchColumn.implicitHeight
            clip: true

            Column {
                id: searchColumn
                width: searchScroller.width
            spacing: Theme.spaceXs

            Text {
                width: parent.width
                color: Theme.warning
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeCaption
                text: toolsOverlay.searchController ? toolsOverlay.searchController.sourceLabel : ""
                wrapMode: Text.WordWrap
            }
            Text {
                width: parent.width
                color: Theme.textSecondary
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeBody
                text: toolsOverlay.searchController ? toolsOverlay.searchController.message : ""
                wrapMode: Text.WordWrap
            }
            Repeater {
                model: toolsOverlay.searchController && toolsOverlay.searchController.state === "ready"
                       ? toolsOverlay.searchController.results : []
                delegate: Rectangle {
                    width: parent.width
                    height: 58
                    radius: Theme.radiusControl
                    color: Theme.surfaceInput
                    border.color: Theme.border
                    Row {
                        anchors.fill: parent
                        anchors.margins: Theme.spaceSm
                        spacing: Theme.spaceSm
                        Column {
                            width: parent.width - originSelect.width - destinationSelect.width
                                   - parent.spacing * 2
                            anchors.verticalCenter: parent.verticalCenter
                            Text {
                                color: Theme.textPrimary
                                font.family: Theme.fontFamily
                                font.pixelSize: Theme.fontSizeBody
                                font.bold: true
                                text: modelData.name
                                elide: Text.ElideRight
                                width: parent.width
                            }
                            Text {
                                color: Theme.textSecondary
                                font.family: Theme.fontFamily
                                font.pixelSize: Theme.fontSizeCaption
                                text: modelData.detail
                                elide: Text.ElideRight
                                width: parent.width
                            }
                        }
                        KeyboardActionButton {
                            id: originSelect
                            objectName: "searchResultOriginButton"
                            width: 36
                            text: "A"
                            normalColor: Theme.markerOrigin
                            pressedColor: "#4fab70"
                            textColor: "#102018"
                            onTriggered: toolsOverlay.selectSearchResult(modelData.coordinate, true)
                        }
                        KeyboardActionButton {
                            id: destinationSelect
                            objectName: "searchResultDestinationButton"
                            width: 36
                            text: "B"
                            normalColor: Theme.markerDestination
                            pressedColor: "#d95f5f"
                            textColor: "#301010"
                            onTriggered: toolsOverlay.selectSearchResult(modelData.coordinate, false)
                        }
                    }
                }
            }
            }
        }
    }

    readonly property real statusSafeTop: Math.max(
                                        layerControlsBottom,
                                        searchResults.visible
                                        ? searchResults.y + searchResults.height : 0)
                                        + Theme.spaceMd

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
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeBody
                wrapMode: Text.WordWrap
                text: mapSourceController.mapStatusText
            }
            Text {
                visible: mapSourceController.layerMode === "raster"
                width: parent.width
                color: Theme.warning
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeCaption
                wrapMode: Text.WordWrap
                text: qsTr("Nguồn chi tiết đến z15; từ z16, độ phân giải thấp hơn.")
            }
            Text {
                color: Theme.warning
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeBody
                text: qsTr("Lớp ảnh · chưa có dữ liệu")
            }
            Text {
                width: parent.width
                color: Theme.textSecondary
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeCaption
                wrapMode: Text.WordWrap
                text: qsTr("Mọi tài nguyên bản đồ hiện tại chỉ dùng loopback.")
            }
        }
    }
}
