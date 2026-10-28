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
    property bool panelCollapsed: false
    property bool navigationActive: routeNavigationController.active
    readonly property bool endpointSelectionEnabled:
        !routeNavigationController.active
        && !(routingController.loading && routeNavigationController.routeAvailable)
    property string compactSource: routingController.sourceLabel === "Valhalla local"
                                   ? qsTr("Valhalla") : qsTr("Fixture")
    property color routeSummaryColor: routingController.loading ? Theme.warning
                                      : !routingController.routeAvailable ? Theme.error
                                      : routingController.sourceLabel === "Valhalla local"
                                        && routingController.connectionLabel.indexOf("loopback") >= 0
                                        ? Theme.success : Theme.warning

    onClosing: {
        close.accepted = false
        root.visible = false
        shutdownCoordinator.requestQuit()
    }

    function applyEndpoints() {
        var requestedOrigin = originField.text
        var requestedDestination = destinationField.text
        if (routingController.setOriginInput(requestedOrigin))
            routingController.setDestinationInput(requestedDestination)
    }

    function calculateRequestedRoute() {
        var requestedOrigin = originField.text
        var requestedDestination = destinationField.text
        if (routingController.setOriginInput(requestedOrigin)
                && routingController.setDestinationInput(requestedDestination)) {
            if (routingFixtureOnly)
                routingController.calculateFixtureRoute()
            else
                routingController.calculateRoute()
        }
    }

    function recenterNavigation() {
        if (!routeNavigationController.active
                || !routeNavigationController.coordinate.isValid)
            return
        map.center = routeNavigationController.coordinate
        map.bearing = routeNavigationController.bearing
        map.tilt = 45
        if (map.zoomLevel < 16)
            map.zoomLevel = 16
    }

    function fitRouteWithPanelPadding() {
        if (!routingController.routeAvailable || routingController.routePath.length < 2)
            return
        var path = routingController.routePath
        var minLatitude = path[0].latitude
        var maxLatitude = path[0].latitude
        var minLongitude = path[0].longitude
        var maxLongitude = path[0].longitude
        for (var index = 1; index < path.length; ++index) {
            minLatitude = Math.min(minLatitude, path[index].latitude)
            maxLatitude = Math.max(maxLatitude, path[index].latitude)
            minLongitude = Math.min(minLongitude, path[index].longitude)
            maxLongitude = Math.max(maxLongitude, path[index].longitude)
        }
        var leftPadding = root.panelCollapsed ? Theme.spaceLg
                                              : routePanel.width + Theme.spaceLg * 2
        var rightPadding = Theme.spaceLg * 3
        var verticalPadding = Theme.spaceLg * 2
        var usableWidth = Math.max(240, map.width - leftPadding - rightPadding)
        var usableHeight = Math.max(240, map.height - verticalPadding * 2)
        var longitudeSpan = Math.max(0.000001, maxLongitude - minLongitude)
        function mercatorY(latitude) {
            var radians = latitude * Math.PI / 180
            return (1 - Math.log(Math.tan(radians) + 1 / Math.cos(radians))
                    / Math.PI) / 2
        }
        function latitudeFromMercator(y) {
            return Math.atan(Math.sinh(Math.PI * (1 - 2 * y))) * 180 / Math.PI
        }
        var northY = mercatorY(maxLatitude)
        var southY = mercatorY(minLatitude)
        var latitudeSpan = Math.max(0.000001, southY - northY)
        var zoomX = Math.log(usableWidth * 360 / (256 * longitudeSpan)) / Math.LN2
        var zoomY = Math.log(usableHeight / (256 * latitudeSpan)) / Math.LN2
        var fittedZoom = Math.min(zoomX, zoomY, map.maximumZoomLevel)
        var worldPixels = 256 * Math.pow(2, fittedZoom)
        var routeCenterLongitude = (minLongitude + maxLongitude) / 2
        var routeCenterLatitude = latitudeFromMercator((northY + southY) / 2)
        var routeTargetX = leftPadding + usableWidth / 2
        var centerShiftPixels = routeTargetX - map.width / 2
        var mapCenterLongitude = routeCenterLongitude
                - centerShiftPixels * 360 / worldPixels
        map.zoomLevel = Math.max(map.minimumZoomLevel, fittedZoom)
        map.center = QtPositioning.coordinate(routeCenterLatitude, mapCenterLongitude)
    }

    Plugin {
        id: mapPlugin
        name: "maplibre"
        allowExperimental: true
        PluginParameter { name: "maplibre.map.styles"; value: vectorStyleUrl }
        PluginParameter { name: "mapboxgl.mapping.cache.memory"; value: true }
    }

    Map {
        id: map
        objectName: "routingMap"
        anchors.fill: parent
        plugin: mapPlugin
        center: QtPositioning.coordinate(21.0278, 105.8342)
        zoomLevel: 14
        gesture.enabled: !routeNavigationController.running
        focus: true

        onMapReadyChanged: {
            console.log("routing mapReady=" + mapReady + " error=" + error
                        + " errorString=" + errorString)
            if (mapReady && supportedMapTypes.length > 0) {
                activeMapType = supportedMapTypes[0]
                routeRefreshTimer.restart()
                benchmarkScenarioController.notifyMapReady()
            }
        }

        Keys.onPressed: {
            if (event.key === Qt.Key_F3 && instrumentationEnabled) {
                performanceOverlayVisible = !performanceOverlayVisible
                event.accepted = true
            }
        }

        TapHandler {
            objectName: "routeMapTapHandler"
            enabled: root.endpointSelectionEnabled
            acceptedButtons: Qt.LeftButton
            onTapped: routingController.selectMapPoint(map.toCoordinate(point.position))
        }

        MapPolyline {
            id: routeOutline
            path: routingController.routePath
            line.width: 12
            line.color: Theme.routeOutline
            opacity: 0.92
            z: 5
        }

        MapPolyline {
            id: routeLine
            path: routingController.routePath
            line.width: 7
            line.color: Theme.route
            z: 6
        }

        MapQuickItem {
            coordinate: routingController.origin
            anchorPoint.x: originMarker.width / 2
            anchorPoint.y: originMarker.height / 2
            z: 10
            sourceItem: Rectangle {
                id: originMarker
                width: 30
                height: 30
                radius: 15
                color: Theme.success
                border.color: Theme.surfaceElevated
                border.width: 5
                Text { anchors.centerIn: parent; text: "A"; font.bold: true; color: "#102018" }
            }
        }

        MapQuickItem {
            visible: routeNavigationController.active
            coordinate: routeNavigationController.coordinate
            anchorPoint.x: navigationMarker.width / 2
            anchorPoint.y: navigationMarker.height / 2
            z: 12
            sourceItem: Rectangle {
                id: navigationMarker
                width: 28
                height: 28
                radius: 14
                color: Theme.accent
                border.color: Theme.routeOutline
                border.width: 4
                Text {
                    anchors.centerIn: parent
                    text: "▲"
                    color: "#10243a"
                    font.bold: true
                    font.pixelSize: 13
                }
            }
        }

        MapQuickItem {
            coordinate: routingController.destination
            anchorPoint.x: destinationMarker.width / 2
            anchorPoint.y: destinationMarker.height / 2
            z: 10
            sourceItem: Rectangle {
                id: destinationMarker
                width: 30
                height: 30
                radius: 15
                color: Theme.error
                border.color: Theme.surfaceElevated
                border.width: 5
                Text { anchors.centerIn: parent; text: "B"; font.bold: true; color: "#301010" }
            }
        }

        MapItemView {
            model: map.zoomLevel >= 15 ? routingController.streetLabels : []
            delegate: MapQuickItem {
                coordinate: modelData.coordinate
                anchorPoint.x: streetLabel.width / 2
                anchorPoint.y: streetLabel.height + 8
                z: 9
                sourceItem: Rectangle {
                    id: streetLabel
                    width: streetName.implicitWidth + 12
                    height: streetName.implicitHeight + 6
                    radius: Theme.radiusPanel
                    color: "#eeffffff"
                    border.color: Theme.route
                    Text {
                        id: streetName
                        anchors.centerIn: parent
                        text: modelData.streetName
                        color: "#10243a"
                        font.pixelSize: Theme.fontSizeCaption
                        font.bold: true
                    }
                }
            }
        }
    }

    NavigationBanner {
        id: navigationBanner
        objectName: "navigationBanner"
        visible: routeNavigationController.active
        x: root.panelCollapsed
           ? (root.width - width) / 2
           : routePanel.x + routePanel.width + Theme.spaceLg
             + Math.max(0, (root.width - routePanel.x - routePanel.width
                            - Theme.spaceLg - width) / 2)
        anchors.top: parent.top
        anchors.topMargin: Theme.spaceMd
        z: 30
        instruction: routeNavigationController.maneuverInstruction
        streetName: routeNavigationController.maneuverStreetName
        sourceLabel: routeNavigationController.sourceLabel
        distanceToManeuverMeters: routeNavigationController.distanceToManeuverMeters
        remainingDistanceMeters: routeNavigationController.remainingDistanceMeters
        remainingDurationSeconds: routeNavigationController.remainingDurationSeconds
        arrived: routeNavigationController.arrived
    }

    Rectangle {
        id: routePanel
        readonly property real maneuverListMinimumHeight: 120
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.margins: Theme.spaceMd
        width: 330
        visible: !root.panelCollapsed
        radius: Theme.radiusPanel
        color: Theme.surfaceElevated
        border.color: Theme.border
        z: 20

        Flickable {
            id: routeControlsScroll
            objectName: "routeControlsScroll"
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: Theme.spaceLg
            height: Math.max(0, Math.min(routeControls.implicitHeight,
                                         routePanel.height
                                         - routePanel.maneuverListMinimumHeight
                                         - Theme.spaceLg * 2
                                         - Theme.spaceSm))
            contentWidth: width
            contentHeight: routeControls.implicitHeight
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            flickableDirection: Flickable.VerticalFlick

            Column {
                id: routeControls
                width: routeControlsScroll.width
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
                    objectName: "originField"
                    anchors.fill: parent
                    anchors.margins: Theme.spaceSm
                    color: "#17212b"
                    verticalAlignment: TextInput.AlignVCenter
                    selectByMouse: true
                    activeFocusOnTab: true
                    KeyNavigation.tab: destinationField
                    KeyNavigation.backtab: startNavigationButton
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
                    objectName: "destinationField"
                    anchors.fill: parent
                    anchors.margins: Theme.spaceSm
                    color: "#17212b"
                    verticalAlignment: TextInput.AlignVCenter
                    selectByMouse: true
                    activeFocusOnTab: true
                    KeyNavigation.tab: applyButton
                    KeyNavigation.backtab: originField
                    text: routingController.destinationInput
                    onAccepted: routingController.setDestinationInput(text)
                }
            }

            Row {
                width: parent.width
                spacing: Theme.spaceSm
                KeyboardActionButton {
                    id: applyButton
                    width: (parent.width - parent.spacing) / 2
                    text: qsTr("Áp dụng điểm")
                    normalColor: "#334352"
                    pressedColor: "#455869"
                    textColor: Theme.textPrimary
                    KeyNavigation.tab: routeButton
                    KeyNavigation.backtab: destinationField
                    onTriggered: root.applyEndpoints()
                }
                KeyboardActionButton {
                    id: routeButton
                    width: (parent.width - parent.spacing) / 2
                    enabled: !routingController.loading
                    text: routingController.loading ? qsTr("Đang tính…") : qsTr("Tính đường")
                    KeyNavigation.tab: startNavigationButton
                    KeyNavigation.backtab: applyButton
                    onTriggered: root.calculateRequestedRoute()
                }
            }

            Rectangle {
                width: parent.width
                height: 40
                radius: Theme.radiusPanel
                color: "#cc1b2530"
                border.color: Theme.border

                Text {
                    anchors.fill: parent
                    anchors.margins: Theme.spaceSm
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                    color: root.routeSummaryColor
                    font.bold: true
                    text: routingController.routeAvailable
                          ? qsTr("%1 • %2 km • %3 phút")
                                .arg(root.compactSource)
                                .arg((routingController.distanceMeters / 1000).toFixed(2))
                                .arg(Math.ceil(routingController.durationSeconds / 60))
                          : routingController.loading ? qsTr("Đang tính tuyến…")
                                                      : qsTr("Không có tuyến")
                }
            }

            KeyboardActionButton {
                width: parent.width
                visible: routeNavigationController.active
                text: qsTr("Theo vị trí hiện tại")
                normalColor: "#334352"
                pressedColor: "#455869"
                textColor: Theme.textPrimary
                onTriggered: root.recenterNavigation()
            }

            KeyboardActionButton {
                id: startNavigationButton
                width: parent.width
                visible: routingController.routeAvailable
                         && !routeNavigationController.active
                text: qsTr("Bắt đầu chỉ đường")
                KeyNavigation.tab: originField
                KeyNavigation.backtab: routeButton
                normalColor: Theme.success
                pressedColor: "#4fab70"
                textColor: "#102018"
                onTriggered: routeNavigationController.start()
            }

            Row {
                width: parent.width
                visible: routingController.routeAvailable
                spacing: Theme.spaceXs
                Repeater {
                    model: [1, 2, 4]
                    delegate: KeyboardActionButton {
                        width: (routeControls.width - Theme.spaceXs * 2) / 3
                        text: modelData + "x"
                        normalColor: routeNavigationController.speedMultiplier === modelData
                                     ? Theme.accent : "#334352"
                        pressedColor: "#455869"
                        textColor: routeNavigationController.speedMultiplier === modelData
                                   ? "#10243a" : Theme.textPrimary
                        onTriggered: routeNavigationController.setSpeedMultiplier(modelData)
                    }
                }
            }

            Row {
                width: parent.width
                visible: routeNavigationController.active
                spacing: Theme.spaceSm
                KeyboardActionButton {
                    id: navigationPrimaryAction
                    objectName: "navigationPrimaryAction"
                    width: (parent.width - parent.spacing) / 2
                    text: routeNavigationController.arrived ? qsTr("Đi lại từ đầu")
                          : routeNavigationController.paused ? qsTr("Tiếp tục")
                                                             : qsTr("Tạm dừng")
                    normalColor: Theme.warning
                    pressedColor: "#d89b19"
                    textColor: "#2d2308"
                    onTriggered: {
                        if (routeNavigationController.arrived
                                || routeNavigationController.paused)
                            routeNavigationController.start()
                        else
                            routeNavigationController.pause()
                    }
                }
                KeyboardActionButton {
                    width: (parent.width - parent.spacing) / 2
                    text: qsTr("Dừng")
                    normalColor: Theme.error
                    pressedColor: "#d95f5f"
                    textColor: "#301010"
                    onTriggered: routeNavigationController.stop()
                }
            }

            KeyboardActionButton {
                width: parent.width
                visible: routeNavigationController.running || routeNavigationController.paused
                text: qsTr("Mô phỏng đi lệch (>40 m)")
                normalColor: "#334352"
                pressedColor: "#455869"
                textColor: Theme.textPrimary
                onTriggered: routeNavigationController.simulateDeviation()
            }

            Text {
                width: parent.width
                visible: routingController.routeAvailable
                text: routeNavigationController.statusMessage
                color: routeNavigationController.arrived ? Theme.success
                                                          : Theme.textSecondary
                wrapMode: Text.WordWrap
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
        }

        ListView {
            id: maneuverList
            objectName: "maneuverList"
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: routeControlsScroll.bottom
            anchors.bottom: parent.bottom
            anchors.leftMargin: Theme.spaceLg
            anchors.rightMargin: Theme.spaceLg
            anchors.topMargin: Theme.spaceSm
            anchors.bottomMargin: Theme.spaceLg
            clip: true
            spacing: Theme.spaceXs
            model: routingController.maneuvers
            currentIndex: routeNavigationController.active
                          ? routeNavigationController.currentManeuverIndex
                          : routingController.currentManeuverIndex
            activeFocusOnTab: true

            Keys.onDownPressed: {
                routingController.focusManeuver(
                            Math.min(count - 1,
                                     routingController.currentManeuverIndex + 1))
                event.accepted = true
            }
            Keys.onUpPressed: {
                routingController.focusManeuver(
                            Math.max(0, routingController.currentManeuverIndex - 1))
                event.accepted = true
            }

            delegate: ManeuverRow {
                width: maneuverList.width
                maneuverIndex: index
                maneuver: modelData
                selected: index === (routeNavigationController.active
                                     ? routeNavigationController.currentManeuverIndex
                                     : routingController.currentManeuverIndex)
                onActivated: {
                    routingController.focusManeuver(maneuverIndex)
                    maneuverList.forceActiveFocus()
                }
            }
        }
    }

    KeyboardActionButton {
        id: panelToggle
        x: root.panelCollapsed ? Theme.spaceMd
                               : routePanel.x + routePanel.width + Theme.spaceSm
        y: Theme.spaceMd
        width: 38
        z: 24
        text: root.panelCollapsed ? "›" : "‹"
        normalColor: "#dd1b2530"
        pressedColor: "#334352"
        textColor: Theme.textPrimary
        onTriggered: {
            root.panelCollapsed = !root.panelCollapsed
            Qt.callLater(root.fitRouteWithPanelPadding)
        }
    }

    Rectangle {
        id: connectionStatusPill
        objectName: "connectionStatusPill"
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.rightMargin: Theme.spaceMd
        anchors.topMargin: routeNavigationController.active
                           ? navigationBanner.height + Theme.spaceMd * 2
                           : Theme.spaceMd
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
                color: root.routeSummaryColor
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
        target: benchmarkScenarioController
        function onCameraCommand(coordinate, zoomLevel, bearing) {
            map.center = coordinate
            map.zoomLevel = zoomLevel
            map.bearing = bearing
        }
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

    Connections {
        target: routeNavigationController
        function onPositionChanged() {
            if (!routeNavigationController.active)
                return
            map.center = routeNavigationController.coordinate
            map.bearing = routeNavigationController.bearing
            map.tilt = 45
            if (map.zoomLevel < 16)
                map.zoomLevel = 16
        }
        function onStateChanged() {
            if (!routeNavigationController.active) {
                map.bearing = 0
                map.tilt = 0
                routeRefreshTimer.restart()
            }
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
            root.fitRouteWithPanelPadding()
        }
    }

    Component.onCompleted: {
        if (routingFixtureOnly && !routingController.routeAvailable
                && !routingController.loading)
            routingController.calculateFixtureRoute()
    }
}
