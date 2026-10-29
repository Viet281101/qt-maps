import QtQuick 2.13
import QtLocation 5.13
import QtPositioning 5.13
import "../theme"

Item {
    id: canvas
    property bool endpointSelectionEnabled: true
    property bool panelCollapsed: false
    property real panelWidth: 330
    signal togglePerformanceOverlayRequested()

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
                width: 30; height: 30; radius: 15
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
            coordinate: routingController.destination
            anchorPoint.x: destinationMarker.width / 2
            anchorPoint.y: destinationMarker.height / 2
            z: 10
            sourceItem: Rectangle {
                id: destinationMarker
                width: 30; height: 30; radius: 15
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
        function onRouteChanged() { routeRefreshTimer.restart() }
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
        function onPositionChanged() { canvas.recenterNavigation() }
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
            canvas.fitRouteWithPanelPadding()
        }
    }
}
