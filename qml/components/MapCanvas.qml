import QtQuick 2.13
import QtLocation 5.13
import QtPositioning 5.13
import "../theme"

Item {
    id: canvas
    property bool endpointSelectionEnabled: true
    property bool panelCollapsed: false
    property real panelWidth: 330
    property bool cameraGestureActive: false
    property string styleUrl: ""
    property var travelledRoutePath: []
    readonly property bool rasterMode: mapSourceController.layerMode === "raster"
    readonly property var map: mapLoader.item
    readonly property bool guidanceActive: routeNavigationController.active
                                           || (routingController.loading
                                               && routingController.routeAvailable)
    signal togglePerformanceOverlayRequested()

    function applyCamera(state) {
        if (!map || !state.valid)
            return
        map.center = QtPositioning.coordinate(state.latitude, state.longitude)
        map.zoomLevel = state.zoom
        map.bearing = state.bearing
        map.tilt = state.tilt
    }

    function applyOverviewCamera() {
        applyCamera(boundedCameraController.overviewCamera(map.width, map.height))
    }

    function settleUserCamera() {
        cameraGestureActive = false
        if (!map)
            return
        var state = boundedCameraController.normalizedCamera(
                    map.center.latitude, map.center.longitude, map.zoomLevel,
                    map.bearing, map.tilt, map.width, map.height)
        if (Math.abs(state.latitude - map.center.latitude) > 0.0000001
                || Math.abs(state.longitude - map.center.longitude) > 0.0000001
                || Math.abs(state.zoom - map.zoomLevel) > 0.0001
                || Math.abs(state.bearing - map.bearing) > 0.0001
                || Math.abs(state.tilt - map.tilt) > 0.0001)
            applyCamera(state)
    }

    function recenterNavigation() {
        if (!map || !routeNavigationController.active
                || !routeNavigationController.coordinate.isValid)
            return
        applyCamera(boundedCameraController.navigationCamera(
                        routeNavigationController.coordinate.latitude,
                        routeNavigationController.coordinate.longitude,
                        routeNavigationController.bearing, map.zoomLevel, map.width, map.height))
    }

    function updateTravelledRoute() {
        var routePath = routingController.routePath
        var position = routeNavigationController.coordinate
        if (!routeNavigationController.active || !position.isValid
                || routePath.length < 2) {
            travelledRoutePath = []
            return
        }
        var nearestIndex = 0
        var nearestDistance = Number.MAX_VALUE
        for (var index = 0; index < routePath.length; ++index) {
            var distance = position.distanceTo(routePath[index])
            if (distance < nearestDistance) {
                nearestDistance = distance
                nearestIndex = index
            }
        }
        var completedPath = []
        for (var completedIndex = 0; completedIndex <= nearestIndex; ++completedIndex)
            completedPath.push(routePath[completedIndex])
        completedPath.push(position)
        travelledRoutePath = completedPath
    }

    function settleNavigationState() {
        if (!map)
            return
        if (guidanceActive)
            return
        travelledRoutePath = []
        map.bearing = 0
        map.tilt = 0
        routeRefreshTimer.restart()
    }

    function fitRouteWithPanelPadding() {
        if (!map || !routingController.routeAvailable || routingController.routePath.length < 2)
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
        var leftPadding = panelCollapsed ? Theme.spaceLg
                                         : panelWidth + Theme.spaceLg * 2
        var rightPadding = Theme.spaceLg * 3
        var verticalPadding = Theme.spaceLg * 2
        applyCamera(boundedCameraController.routeFitCamera(
                        minLatitude, maxLatitude, minLongitude, maxLongitude,
                        map.width, map.height, leftPadding, rightPadding,
                        verticalPadding, verticalPadding, map.bearing, map.tilt))
    }

    Loader {
        id: mapLoader
        anchors.fill: parent
        sourceComponent: routingMapComponent
    }

    Component {
      id: routingMapComponent

      Map {
        id: mapSurface
        function refreshRoute() {
            var currentPath = routingController.routePath
            routeOutline.path = []
            routeLine.path = []
            routeOutline.path = currentPath
            routeLine.path = currentPath
            canvas.fitRouteWithPanelPadding()
        }
        objectName: "routingMap"
        anchors.fill: parent
        plugin: Plugin {
            name: canvas.rasterMode ? "osm" : "maplibre"
            allowExperimental: true
            PluginParameter { name: "maplibre.map.styles"; value: canvas.styleUrl }
            PluginParameter {
                name: "osm.mapping.custom.host"
                value: mapSourceController.rasterBaseUrl + "tiles/"
            }
            PluginParameter { name: "osm.mapping.providersrepository.disabled"; value: true }
            PluginParameter { name: "osm.mapping.prefetching_style"; value: "NoPrefetching" }
            PluginParameter { name: "osm.mapping.cache.disk.size"; value: 0 }
            PluginParameter { name: "osm.mapping.cache.memory.cost_strategy"; value: "bytesize" }
            PluginParameter { name: "osm.mapping.cache.memory.size"; value: 8 * 1024 * 1024 }
            PluginParameter { name: "osm.mapping.cache.texture.cost_strategy"; value: "bytesize" }
            PluginParameter { name: "osm.mapping.cache.texture.size"; value: 16 * 1024 * 1024 }
            PluginParameter { name: "osm.useragent"; value: "QtMaps-local-raster/0.2" }
            PluginParameter {
                name: "osm.mapping.custom.mapcopyright"
                value: "© OpenStreetMap contributors, ODbL"
            }
            PluginParameter {
                name: "osm.mapping.custom.datacopyright"
                value: "© OpenStreetMap contributors, ODbL"
            }
        }
        center: QtPositioning.coordinate(21.0278, 105.8342)
        // Local source tiles stop at z15/z16, but MapLibre deliberately overzooms them to
        // the camera contract's z18 display ceiling. Keep the provider inside that contract
        // even while a gesture is still in progress.
        minimumZoomLevel: 12
        maximumZoomLevel: 18
        zoomLevel: 14
        gesture.enabled: !routeNavigationController.running
        gesture.onPanStarted: canvas.cameraGestureActive = true
        gesture.onPanFinished: canvas.settleUserCamera()
        gesture.onFlickStarted: canvas.cameraGestureActive = true
        gesture.onFlickFinished: canvas.settleUserCamera()
        gesture.onPinchStarted: canvas.cameraGestureActive = true
        gesture.onPinchFinished: canvas.settleUserCamera()
        gesture.onRotationStarted: canvas.cameraGestureActive = true
        gesture.onRotationFinished: canvas.settleUserCamera()
        gesture.onTiltStarted: canvas.cameraGestureActive = true
        gesture.onTiltFinished: canvas.settleUserCamera()
        focus: true
        onCenterChanged: rasterRouteOverlay.requestPaint()
        onZoomLevelChanged: rasterRouteOverlay.requestPaint()
        onBearingChanged: rasterRouteOverlay.requestPaint()
        onTiltChanged: rasterRouteOverlay.requestPaint()

        onMapReadyChanged: {
            console.log("routing mapReady=" + mapReady + " error=" + error
                        + " errorString=" + errorString)
            if (mapReady && supportedMapTypes.length > 0) {
                if (canvas.rasterMode) {
                    for (var index = 0; index < supportedMapTypes.length; ++index) {
                        if (supportedMapTypes[index].name === "Custom URL Map") {
                            activeMapType = supportedMapTypes[index]
                            break
                        }
                    }
                } else {
                    activeMapType = supportedMapTypes[0]
                }
                canvas.applyOverviewCamera()
                routeRefreshTimer.restart()
                benchmarkScenarioController.notifyMapReady()
            }
        }

        Keys.onPressed: {
            if (event.key === Qt.Key_F3 && instrumentationEnabled) {
                canvas.togglePerformanceOverlayRequested()
                event.accepted = true
            }
        }

        TapHandler {
            objectName: "routeMapTapHandler"
            enabled: canvas.endpointSelectionEnabled
            acceptedButtons: Qt.LeftButton
            onTapped: routingController.selectMapPoint(map.toCoordinate(point.position))
        }

        MapPolyline {
            id: routeOutline
            visible: !canvas.rasterMode
            path: routingController.routePath
            line.width: 12
            line.color: Theme.routeOutline
            opacity: 0.92
            z: 5
        }

        MapPolyline {
            id: routeLine
            visible: !canvas.rasterMode
            path: routingController.routePath
            line.width: 7
            line.color: Theme.route
            z: 6
        }

        MapPolyline {
            id: travelledRouteLine
            // MapLibre hides sibling polylines when an empty MapPolyline remains visible.
            visible: !canvas.rasterMode && canvas.travelledRoutePath.length > 1
            path: canvas.travelledRoutePath
            line.width: 7
            line.color: Theme.routeTravelled
            z: 7
        }

        MapQuickItem {
            visible: !canvas.rasterMode
            coordinate: routingController.origin
            anchorPoint.x: originMarker.width / 2
            anchorPoint.y: originMarker.height / 2
            z: 10
            sourceItem: Rectangle {
                id: originMarker
                width: 34; height: 34; radius: 17
                color: Theme.markerOrigin
                border.color: Theme.surfaceElevated
                border.width: 5
                Text { anchors.centerIn: parent; text: "A"; font.bold: true; color: "#102018" }
            }
        }

        MapQuickItem {
            visible: !canvas.rasterMode && canvas.guidanceActive
            coordinate: routeNavigationController.coordinate
            anchorPoint.x: navigationMarker.width / 2
            anchorPoint.y: navigationMarker.height / 2
            z: 12
            sourceItem: Rectangle {
                id: navigationMarker
                width: 28; height: 28; radius: 14
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
            visible: !canvas.rasterMode
            coordinate: routingController.destination
            anchorPoint.x: destinationMarker.width / 2
            anchorPoint.y: destinationMarker.height / 2
            z: 10
            sourceItem: Rectangle {
                id: destinationMarker
                width: 34; height: 34; radius: 17
                color: Theme.markerDestination
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
    }

    Rectangle {
        id: overzoomWarning
        objectName: "rasterOverzoomWarning"
        visible: canvas.rasterMode && canvas.map && canvas.map.zoomLevel >= 16
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 54
        width: overzoomText.implicitWidth + Theme.spaceLg * 2
        height: 34
        radius: Theme.radiusPill
        color: Theme.surfaceElevated
        border.color: Theme.warning
        z: 18

        Text {
            id: overzoomText
            anchors.centerIn: parent
            color: Theme.warning
            font.pixelSize: Theme.fontSizeCaption
            font.bold: true
            text: qsTr("Nguồn chi tiết đến z15 · z16+ có độ phân giải thấp hơn")
        }
    }

    Canvas {
        id: rasterRouteOverlay
        objectName: "rasterRouteOverlay"
        anchors.fill: parent
        visible: canvas.rasterMode
        z: 2

        function drawPolyline(context, path, width, color) {
            if (!path || path.length < 2)
                return
            context.beginPath()
            var started = false
            for (var index = 0; index < path.length; ++index) {
                var point = map.fromCoordinate(path[index], false)
                if (!isFinite(point.x) || !isFinite(point.y))
                    continue
                if (!started) {
                    context.moveTo(point.x, point.y)
                    started = true
                } else {
                    context.lineTo(point.x, point.y)
                }
            }
            if (!started)
                return
            context.lineWidth = width
            context.strokeStyle = color
            context.lineCap = "round"
            context.lineJoin = "round"
            context.stroke()
        }

        function drawMarker(context, coordinate, radius, fill, label, textColor) {
            if (!coordinate || !coordinate.isValid)
                return
            var point = map.fromCoordinate(coordinate, false)
            context.beginPath()
            context.arc(point.x, point.y, radius, 0, Math.PI * 2)
            context.fillStyle = fill
            context.fill()
            context.lineWidth = 5
            context.strokeStyle = Theme.surfaceElevated
            context.stroke()
            context.fillStyle = textColor
            context.font = "bold 14px sans-serif"
            context.textAlign = "center"
            context.textBaseline = "middle"
            context.fillText(label, point.x, point.y)
        }

        onPaint: {
            var context = getContext("2d")
            context.clearRect(0, 0, width, height)
            if (!canvas.map)
                return
            drawPolyline(context, routingController.routePath, 12, Theme.routeOutline)
            drawPolyline(context, routingController.routePath, 7, Theme.route)
            drawPolyline(context, canvas.travelledRoutePath, 7, Theme.routeTravelled)
            drawMarker(context, routingController.origin, 17, Theme.markerOrigin, "A", "#102018")
            drawMarker(context, routingController.destination, 17,
                       Theme.markerDestination, "B", "#301010")
            if (canvas.guidanceActive)
                drawMarker(context, routeNavigationController.coordinate, 14,
                           Theme.accent, "▲", "#10243a")
        }

        onVisibleChanged: requestPaint()
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
    }

    Connections {
        target: benchmarkScenarioController
        function onCameraCommand(coordinate, zoomLevel, bearing) {
            if (!canvas.map)
                return
            canvas.applyCamera(boundedCameraController.normalizedCamera(
                                 coordinate.latitude, coordinate.longitude,
                                 zoomLevel, bearing, map.tilt, map.width, map.height))
        }
    }

    Connections {
        target: routingController
        function onRouteChanged() {
            canvas.travelledRoutePath = []
            routeRefreshTimer.restart()
            rasterRouteOverlay.requestPaint()
        }
        function onStateChanged() {
            if (routingController.routeAvailable && !routingController.loading)
                routeRefreshTimer.restart()
        }
        function onFocusCoordinateRequested(coordinate, zoomLevel) {
            if (!canvas.map)
                return
            canvas.applyCamera(boundedCameraController.normalizedCamera(
                                     coordinate.latitude, coordinate.longitude,
                                     zoomLevel, map.bearing, map.tilt, map.width, map.height))
        }
    }

    Connections {
        target: routeNavigationController
        function onPositionChanged() {
            canvas.updateTravelledRoute()
            canvas.recenterNavigation()
            rasterRouteOverlay.requestPaint()
        }
        function onStateChanged() {
            // Deviation emits navigation state before RoutingController enters loading.
            // Defer the stop decision so re-routing keeps the last route, puck and camera.
            Qt.callLater(canvas.settleNavigationState)
        }
    }

    Timer {
        id: routeRefreshTimer
        interval: 100
        repeat: false
        onTriggered: {
            if (canvas.cameraGestureActive) {
                restart()
                return
            }
            if (canvas.map)
                canvas.map.refreshRoute()
            rasterRouteOverlay.requestPaint()
        }
    }

    Timer {
        id: mapReloadTimer
        interval: 250
        repeat: false
        onTriggered: mapLoader.active = true
    }

    onTravelledRoutePathChanged: rasterRouteOverlay.requestPaint()
    onRasterModeChanged: {
        mapLoader.active = false
        mapReloadTimer.restart()
    }
}
