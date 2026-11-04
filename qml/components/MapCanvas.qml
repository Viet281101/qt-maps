import QtQuick 2.13
import QtLocation 5.13
import QtPositioning 5.13
import "../theme"

Item {
    id: canvas
    property bool endpointSelectionEnabled: true
    property bool panelCollapsed: false
    property real panelWidth: 330
    property string styleUrl: ""
    property bool routeOverlayEnabled: true
    property var travelledRoutePath: []
    readonly property bool rasterMode: mapSourceController.layerMode === "raster"
    readonly property var map: mapLoader.item
    readonly property bool guidanceActive: routeNavigationController.active
                                           || (routingController.loading
                                               && routingController.routeAvailable)
    readonly property real dataWest: 105.78
    readonly property real dataSouth: 20.97
    readonly property real dataEast: 105.90
    readonly property real dataNorth: 21.10
    property bool adjustingDataBounds: false
    property bool routeOverlayDirty: false
    property int routeOverlayPaintRequests: 0

    function scheduleRouteOverlayPaint() {
        if (!rasterRouteOverlay.visible) {
            routeOverlayDirty = false
            return
        }
        routeOverlayDirty = true
        if (!routeOverlayPaintTimer.running)
            routeOverlayPaintTimer.start()
    }

    function recenterNavigation() {
        if (!map || !routeNavigationController.active
                || !routeNavigationController.coordinate.isValid)
            return
        map.center = routeNavigationController.coordinate
        map.bearing = routeNavigationController.bearing
        map.tilt = 45
        if (map.zoomLevel < 16)
            map.zoomLevel = 16
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

    function clampMapToDataBounds() {
        if (!map || !map.mapReady || adjustingDataBounds
                || map.width < 1 || map.height < 1)
            return
        var corners = [
            map.toCoordinate(Qt.point(0, 0), false),
            map.toCoordinate(Qt.point(map.width, 0), false),
            map.toCoordinate(Qt.point(0, map.height), false),
            map.toCoordinate(Qt.point(map.width, map.height), false)
        ]
        var west = 180
        var east = -180
        var south = 90
        var north = -90
        for (var index = 0; index < corners.length; ++index) {
            if (!corners[index].isValid)
                return
            west = Math.min(west, corners[index].longitude)
            east = Math.max(east, corners[index].longitude)
            south = Math.min(south, corners[index].latitude)
            north = Math.max(north, corners[index].latitude)
        }
        if ((east - west > dataEast - dataWest
             || north - south > dataNorth - dataSouth)
                && map.zoomLevel < map.maximumZoomLevel) {
            adjustingDataBounds = true
            map.zoomLevel = Math.min(map.maximumZoomLevel, map.zoomLevel + 0.25)
            adjustingDataBounds = false
            dataBoundsTimer.restart()
            return
        }
        var longitudeShift = west < dataWest ? dataWest - west
                           : east > dataEast ? dataEast - east : 0
        var latitudeShift = south < dataSouth ? dataSouth - south
                          : north > dataNorth ? dataNorth - north : 0
        if (Math.abs(longitudeShift) < 0.000001
                && Math.abs(latitudeShift) < 0.000001)
            return
        adjustingDataBounds = true
        map.center = QtPositioning.coordinate(map.center.latitude + latitudeShift,
                                              map.center.longitude + longitudeShift)
        adjustingDataBounds = false
    }

    function visibleStreetLabels() {
        if (!map || map.zoomLevel < 15)
            return []
        var source = routingController.streetLabels
        var selected = []
        var minimumSpacing = map.zoomLevel < 16 ? 420
                           : map.zoomLevel < 17 ? 220 : 100
        var maximumLabels = map.zoomLevel < 16 ? 4 : 6
        for (var index = 0; index < source.length
             && selected.length < maximumLabels; ++index) {
            var coordinate = source[index].coordinate
            if (!coordinate || !coordinate.isValid)
                continue
            if (coordinate.distanceTo(routingController.origin) < 100
                    || coordinate.distanceTo(routingController.destination) < 100)
                continue
            var separated = true
            for (var selectedIndex = 0; selectedIndex < selected.length; ++selectedIndex) {
                if (coordinate.distanceTo(selected[selectedIndex].coordinate)
                        < minimumSpacing) {
                    separated = false
                    break
                }
            }
            if (separated)
                selected.push(source[index])
        }
        return selected
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
        minimumZoomLevel: canvas.rasterMode ? 12 : 0
        maximumZoomLevel: canvas.rasterMode ? 18 : 20
        zoomLevel: 14
        gesture.enabled: !routeNavigationController.running
        focus: true
        onCenterChanged: {
            canvas.scheduleRouteOverlayPaint()
            dataBoundsTimer.restart()
        }
        onZoomLevelChanged: {
            canvas.scheduleRouteOverlayPaint()
            dataBoundsTimer.restart()
        }
        onBearingChanged: {
            canvas.scheduleRouteOverlayPaint()
            dataBoundsTimer.restart()
        }
        onTiltChanged: {
            canvas.scheduleRouteOverlayPaint()
            dataBoundsTimer.restart()
        }

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
                routeRefreshTimer.restart()
                dataBoundsTimer.restart()
                benchmarkScenarioController.notifyMapReady()
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
            visible: false
            path: routingController.routePath
            line.width: 12
            line.color: Theme.routeOutline
            opacity: 0.92
            z: 5
        }

        MapPolyline {
            id: routeLine
            visible: false
            path: routingController.routePath
            line.width: 7
            line.color: Theme.route
            z: 6
        }

        MapPolyline {
            id: travelledRouteLine
            // MapLibre hides sibling polylines when an empty MapPolyline remains visible.
            visible: false
            path: canvas.travelledRoutePath
            line.width: 7
            line.color: Theme.routeTravelled
            z: 7
        }

        MapQuickItem {
            visible: false
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
            visible: false
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
            visible: false
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
            model: canvas.visibleStreetLabels()
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
        clip: true
        visible: canvas.routeOverlayEnabled && routingController.routeAvailable
        canvasSize: Qt.size(width, height)
        renderTarget: Canvas.FramebufferObject
        renderStrategy: Canvas.Threaded
        z: 2

        function drawPolyline(context, path, width, color, traceEnabled) {
            var metrics = {
                projectionMs: 0,
                pathMs: 0,
                strokeMs: 0,
                projectedVertices: 0,
                viewportVertices: 0,
                strokes: 0
            }
            if (!path || path.length < 2)
                return metrics
            var phaseStarted = traceEnabled
                    ? benchmarkScenarioController.traceMilliseconds() : 0
            var points = []
            for (var index = 0; index < path.length; ++index) {
                var point = map.fromCoordinate(path[index], false)
                if (!isFinite(point.x) || !isFinite(point.y))
                    continue
                points.push(point)
                if (point.x >= 0 && point.x <= width
                        && point.y >= 0 && point.y <= height)
                    ++metrics.viewportVertices
            }
            metrics.projectedVertices = points.length
            if (traceEnabled) {
                var phaseFinished = benchmarkScenarioController.traceMilliseconds()
                metrics.projectionMs = phaseFinished - phaseStarted
                phaseStarted = phaseFinished
            }
            context.beginPath()
            for (var pointIndex = 0; pointIndex < points.length; ++pointIndex) {
                point = points[pointIndex]
                if (pointIndex === 0) {
                    context.moveTo(point.x, point.y)
                } else {
                    context.lineTo(point.x, point.y)
                }
            }
            if (points.length < 2)
                return metrics
            if (traceEnabled) {
                phaseFinished = benchmarkScenarioController.traceMilliseconds()
                metrics.pathMs = phaseFinished - phaseStarted
                phaseStarted = phaseFinished
            }
            context.lineWidth = width
            context.strokeStyle = color
            context.lineCap = "round"
            context.lineJoin = "round"
            context.stroke()
            metrics.strokes = 1
            if (traceEnabled) {
                phaseFinished = benchmarkScenarioController.traceMilliseconds()
                metrics.strokeMs = phaseFinished - phaseStarted
            }
            return metrics
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
            if (!canvas.map || !routingController.routeAvailable)
                return
            var traceEnabled = typeof benchmarkRenderTrace !== "undefined"
                    && benchmarkRenderTrace
                    && typeof benchmarkScenarioController !== "undefined"
            var outlineMetrics = drawPolyline(context, routingController.routePath,
                                               12, Theme.routeOutline, traceEnabled)
            var routeMetrics = drawPolyline(context, routingController.routePath,
                                             7, Theme.route, traceEnabled)
            var travelledMetrics = drawPolyline(context, canvas.travelledRoutePath,
                                                 7, Theme.routeTravelled, traceEnabled)
            drawMarker(context, routingController.origin, 17, Theme.markerOrigin, "A", "#102018")
            drawMarker(context, routingController.destination, 17,
                       Theme.markerDestination, "B", "#301010")
            if (canvas.guidanceActive)
                drawMarker(context, routeNavigationController.coordinate, 14,
                           Theme.accent, "▲", "#10243a")
            if (traceEnabled) {
                var projectionMs = outlineMetrics.projectionMs
                        + routeMetrics.projectionMs + travelledMetrics.projectionMs
                var projectedVertices = outlineMetrics.projectedVertices
                        + routeMetrics.projectedVertices + travelledMetrics.projectedVertices
                benchmarkScenarioController.noteOverlayPaintTiming({
                    "paint_request": canvas.routeOverlayPaintRequests,
                    "route_vertices_total": routingController.routePath.length,
                    "route_vertices_viewport": routeMetrics.viewportVertices,
                    "travelled_vertices_total": canvas.travelledRoutePath.length,
                    "projection_vertex_operations": projectedVertices,
                    "projection_ms": projectionMs,
                    "projection_ms_per_vertex": projectedVertices > 0
                            ? projectionMs / projectedVertices : 0,
                    "path_ms": outlineMetrics.pathMs + routeMetrics.pathMs
                               + travelledMetrics.pathMs,
                    "stroke_ms": outlineMetrics.strokeMs + routeMetrics.strokeMs
                                 + travelledMetrics.strokeMs,
                    "stroke_count": outlineMetrics.strokes + routeMetrics.strokes
                                    + travelledMetrics.strokes
                })
            }
        }

        onVisibleChanged: {
            if (visible)
                canvas.scheduleRouteOverlayPaint()
            else
                canvas.routeOverlayDirty = false
        }
        onWidthChanged: canvas.scheduleRouteOverlayPaint()
        onHeightChanged: canvas.scheduleRouteOverlayPaint()
    }

    Rectangle {
        id: benchmarkRenderDriver
        width: 1
        height: 1
        x: phase
        y: 0
        color: "#ffffff"
        opacity: 0.001
        visible: typeof benchmarkRenderTrace !== "undefined" && benchmarkRenderTrace
                 && !canvas.routeOverlayEnabled
        property real phase: 0

        NumberAnimation on phase {
            from: 0
            to: 1
            duration: 16
            loops: Animation.Infinite
            running: benchmarkRenderDriver.visible
        }
    }

    Connections {
        target: benchmarkScenarioController
        function onCameraCommand(coordinate, zoomLevel, bearing) {
            if (!canvas.map)
                return
            map.center = coordinate
            map.zoomLevel = zoomLevel
            map.bearing = bearing
        }
    }

    Connections {
        target: routingController
        function onRouteChanged() {
            canvas.travelledRoutePath = []
            routeRefreshTimer.restart()
            canvas.scheduleRouteOverlayPaint()
        }
        function onStateChanged() {
            if (routingController.routeAvailable && !routingController.loading)
                routeRefreshTimer.restart()
        }
        function onFocusCoordinateRequested(coordinate, zoomLevel) {
            if (!canvas.map)
                return
            map.center = coordinate
            map.zoomLevel = zoomLevel
        }
    }

    Connections {
        target: routeNavigationController
        function onPositionChanged() {
            canvas.updateTravelledRoute()
            canvas.recenterNavigation()
            canvas.scheduleRouteOverlayPaint()
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
            if (canvas.map)
                canvas.map.refreshRoute()
            canvas.scheduleRouteOverlayPaint()
        }
    }

    Timer {
        id: routeOverlayPaintTimer
        objectName: "routeOverlayPaintTimer"
        interval: 16
        repeat: false
        onTriggered: {
            if (!canvas.routeOverlayDirty || !rasterRouteOverlay.visible) {
                canvas.routeOverlayDirty = false
                return
            }
            canvas.routeOverlayDirty = false
            if (typeof benchmarkRenderTrace !== "undefined" && benchmarkRenderTrace
                    && typeof benchmarkScenarioController !== "undefined") {
                ++canvas.routeOverlayPaintRequests
                benchmarkScenarioController.noteOverlayPaintRequest(
                            canvas.routeOverlayPaintRequests)
            }
            rasterRouteOverlay.requestPaint()
        }
    }

    Timer {
        id: dataBoundsTimer
        interval: 0
        repeat: false
        onTriggered: canvas.clampMapToDataBounds()
    }

    Timer {
        id: mapReloadTimer
        interval: 250
        repeat: false
        onTriggered: mapLoader.active = true
    }

    onTravelledRoutePathChanged: canvas.scheduleRouteOverlayPaint()
    onRasterModeChanged: {
        mapLoader.active = false
        mapReloadTimer.restart()
    }
}
