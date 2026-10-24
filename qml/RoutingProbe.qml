import QtQuick 2.13
import QtQuick.Window 2.13
import QtLocation 5.13
import QtPositioning 5.13
import "components"
import "theme"

Window {
    id: root
    width: 1120
    height: 700
    minimumWidth: 900
    minimumHeight: 580
    visible: true
    title: qsTr("QtMaps — Lập tuyến Hà Nội")
    property bool performanceOverlayVisible: instrumentationEnabled

    Plugin {
        id: mapPlugin
        name: "maplibre"
        allowExperimental: true
        PluginParameter { name: "maplibre.map.styles"; value: vectorStyleUrl }
        PluginParameter { name: "mapboxgl.mapping.cache.memory"; value: true }
    }

    Map {
        id: map
        anchors.fill: parent
        plugin: mapPlugin
        center: QtPositioning.coordinate(21.0278, 105.8342)
        zoomLevel: 14
        gesture.enabled: true
        focus: true

        onMapReadyChanged: {
            console.log("routing mapReady=" + mapReady + " error=" + error
                        + " errorString=" + errorString)
            if (mapReady && supportedMapTypes.length > 0) {
                activeMapType = supportedMapTypes[0]
                routeRefreshTimer.restart()
            }
        }

        Keys.onPressed: {
            if (event.key === Qt.Key_F3 && instrumentationEnabled) {
                performanceOverlayVisible = !performanceOverlayVisible
                event.accepted = true
            }
        }

        TapHandler {
            acceptedButtons: Qt.LeftButton
            onTapped: routingController.selectMapPoint(map.toCoordinate(point.position))
        }

        MapPolyline {
            id: routeOutline
            path: routingController.routePath
            line.width: 9
            line.color: Theme.routeOutline
            opacity: 0.92
            z: 5
        }

        MapPolyline {
            id: routeLine
            path: routingController.routePath
            line.width: 5
            line.color: Theme.route
            z: 6
        }

        MapQuickItem {
            coordinate: routingController.origin
            anchorPoint.x: originMarker.width / 2
            anchorPoint.y: originMarker.height / 2
            z: 8
            sourceItem: Rectangle {
                id: originMarker
                width: 22
                height: 22
                radius: 11
                color: Theme.success
                border.color: "white"
                border.width: 3
                Text { anchors.centerIn: parent; text: "A"; font.bold: true; color: "#102018" }
            }
        }

        MapQuickItem {
            coordinate: routingController.destination
            anchorPoint.x: destinationMarker.width / 2
            anchorPoint.y: destinationMarker.height / 2
            z: 8
            sourceItem: Rectangle {
                id: destinationMarker
                width: 22
                height: 22
                radius: 11
                color: Theme.error
                border.color: "white"
                border.width: 3
                Text { anchors.centerIn: parent; text: "B"; font.bold: true; color: "#301010" }
            }
        }
    }

    Rectangle {
        id: routePanel
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.margins: Theme.spaceMd
        width: 330
        radius: Theme.radiusPanel
        color: Theme.surfaceElevated
        border.color: Theme.border
        z: 20

        Column {
            id: routeControls
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: Theme.spaceLg
            spacing: Theme.spaceSm

            Text {
                text: qsTr("Lập tuyến Hà Nội")
                color: Theme.textPrimary
                font.pixelSize: Theme.fontSizeTitle
                font.bold: true
            }

            Text {
                width: parent.width
                text: qsTr("Bấm bản đồ để chọn %1, hoặc nhập tên mẫu / tọa độ")
                    .arg(routingController.nextMapSelection)
                color: Theme.textSecondary
                wrapMode: Text.WordWrap
            }

            Rectangle {
                width: parent.width
                height: 38
                radius: Theme.radiusPanel
                color: "#f5f7fa"
                border.color: originField.activeFocus ? Theme.accent : Theme.border
                TextInput {
                    id: originField
                    anchors.fill: parent
                    anchors.margins: Theme.spaceSm
                    color: "#17212b"
                    verticalAlignment: TextInput.AlignVCenter
                    selectByMouse: true
                    text: routingController.originInput
                    onAccepted: routingController.setOriginInput(text)
                }
            }

            Rectangle {
                width: parent.width
                height: 38
                radius: Theme.radiusPanel
                color: "#f5f7fa"
                border.color: destinationField.activeFocus ? Theme.accent : Theme.border
                TextInput {
                    id: destinationField
                    anchors.fill: parent
                    anchors.margins: Theme.spaceSm
                    color: "#17212b"
                    verticalAlignment: TextInput.AlignVCenter
                    selectByMouse: true
                    text: routingController.destinationInput
                    onAccepted: routingController.setDestinationInput(text)
                }
            }

            Row {
                width: parent.width
                spacing: Theme.spaceSm
                Rectangle {
                    width: (parent.width - parent.spacing) / 2
                    height: 38
                    radius: Theme.radiusPanel
                    color: applyMouse.pressed ? "#455869" : "#334352"
                    border.color: Theme.border
                    Text { anchors.centerIn: parent; color: Theme.textPrimary; text: qsTr("Áp dụng điểm") }
                    MouseArea {
                        id: applyMouse
                        anchors.fill: parent
                        onClicked: {
                        if (routingController.setOriginInput(originField.text))
                            routingController.setDestinationInput(destinationField.text)
                        }
                    }
                }
                Rectangle {
                    width: (parent.width - parent.spacing) / 2
                    height: 38
                    radius: Theme.radiusPanel
                    color: routeMouse.pressed ? "#2266aa" : Theme.accent
                    opacity: routingController.loading ? 0.55 : 1
                    Text {
                        anchors.centerIn: parent
                        color: "#08243a"
                        font.bold: true
                        text: routingController.loading ? qsTr("Đang tính…") : qsTr("Tính đường")
                    }
                    MouseArea {
                        id: routeMouse
                        anchors.fill: parent
                        enabled: !routingController.loading
                        onClicked: {
                            if (routingController.setOriginInput(originField.text)
                                    && routingController.setDestinationInput(destinationField.text))
                                if (routingFixtureOnly)
                                    routingController.calculateFixtureRoute()
                                else
                                    routingController.calculateRoute()
                        }
                    }
                }
            }

            Rectangle {
                width: parent.width
                height: 64
                radius: Theme.radiusPanel
                color: "#cc1b2530"
                border.color: Theme.border

                Column {
                    anchors.fill: parent
                    anchors.margins: Theme.spaceSm
                    spacing: Theme.spaceXs
                    Text {
                        color: Theme.textPrimary
                        font.bold: true
                        text: routingController.routeAvailable
                              ? qsTr("%1 km  •  %2 phút")
                                    .arg((routingController.distanceMeters / 1000).toFixed(2))
                                    .arg(Math.ceil(routingController.durationSeconds / 60))
                              : qsTr("Chưa có tuyến")
                    }
                    Text {
                        color: Theme.warning
                        text: qsTr("Nguồn: %1  •  %2")
                            .arg(routingController.sourceLabel)
                            .arg(routingController.connectionLabel)
                    }
                }
            }

            Text {
                width: parent.width
                text: routingController.statusMessage
                color: routingController.routeAvailable ? Theme.success : Theme.textSecondary
                wrapMode: Text.WordWrap
            }

            Text {
                text: qsTr("Các bước chỉ đường")
                color: Theme.textPrimary
                font.bold: true
                visible: routingController.routeAvailable
            }

        }

        ListView {
            id: maneuverList
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: routeControls.bottom
            anchors.bottom: parent.bottom
            anchors.leftMargin: Theme.spaceLg
            anchors.rightMargin: Theme.spaceLg
            anchors.topMargin: Theme.spaceSm
            anchors.bottomMargin: Theme.spaceLg
            clip: true
            spacing: Theme.spaceXs
            model: routingController.maneuvers

            delegate: Rectangle {
                width: maneuverList.width
                height: Math.max(54, instructionText.implicitHeight + 22)
                radius: Theme.radiusPanel
                color: maneuverMouse.containsMouse ? "#e6334352" : "#b8222d38"
                border.color: Theme.border

                Text {
                    id: instructionText
                    anchors.left: parent.left
                    anchors.right: distanceText.left
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.margins: Theme.spaceSm
                    color: Theme.textPrimary
                    wrapMode: Text.WordWrap
                    text: (index + 1) + ". " + modelData.instruction
                }
                Text {
                    id: distanceText
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.rightMargin: Theme.spaceSm
                    width: 58
                    horizontalAlignment: Text.AlignRight
                    color: Theme.textSecondary
                    text: modelData.distanceMeters >= 1000
                          ? (modelData.distanceMeters / 1000).toFixed(1) + " km"
                          : Math.round(modelData.distanceMeters) + " m"
                }
                MouseArea {
                    id: maneuverMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: routingController.focusManeuver(index)
                }
            }
        }
    }

    Rectangle {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: Theme.spaceMd
        width: statusRow.implicitWidth + 20
        height: 34
        radius: 17
        color: "#dd111820"
        border.color: Theme.border
        z: 20
        Row {
            id: statusRow
            anchors.centerIn: parent
            spacing: Theme.spaceSm
            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width: 10; height: 10; radius: 5
                color: Theme.warning
            }
            Text { color: Theme.textPrimary; text: routingController.connectionLabel }
        }
    }

    Rectangle {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: Theme.spaceMd
        width: attribution.implicitWidth + 16
        height: 28
        color: "#cc111820"
        z: 20
        Text {
            id: attribution
            anchors.centerIn: parent
            color: Theme.textPrimary
            text: "© OpenStreetMap contributors, ODbL 1.0"
        }
    }

    PerformanceOverlay {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.topMargin: 54
        anchors.rightMargin: Theme.spaceMd
        monitor: performanceMonitor
        visible: instrumentationEnabled && performanceOverlayVisible
        z: 20
    }

    Connections {
        target: routingController
        function onRouteChanged() {
            routeRefreshTimer.restart()
        }
        function onStateChanged() {
            if (routingController.routeAvailable)
                routeRefreshTimer.restart()
        }
        function onFocusCoordinateRequested(coordinate, zoomLevel) {
            map.center = coordinate
            map.zoomLevel = zoomLevel
        }
    }

    Timer {
        id: routeRefreshTimer
        interval: 100
        repeat: false
        onTriggered: {
            var currentPath = routingController.routePath
            routeOutline.path = []
            routeLine.path = []
            routeOutline.path = currentPath
            routeLine.path = currentPath
        }
    }

    Component.onCompleted: {
        if (!routingController.routeAvailable && !routingController.loading) {
            if (routingFixtureOnly)
                routingController.calculateFixtureRoute()
            else
                routingController.calculateRoute()
        }
    }
}
