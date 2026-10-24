import QtQuick 2.13
import QtQuick.Window 2.13
import QtLocation 5.13
import QtPositioning 5.13
import "components"

Window {
    id: root
    width: 960
    height: 640
    visible: true
    title: qsTr("QtMaps Unit 03 continuous navigation benchmark")

    Plugin {
        id: mapPlugin
        name: "maplibre"
        allowExperimental: true
        PluginParameter { name: "maplibre.map.styles"; value: vectorStyleUrl }
    }

    Map {
        id: map
        anchors.fill: parent
        plugin: mapPlugin
        center: navigationController.coordinate
        zoomLevel: 15
        tilt: 45
        bearing: navigationController.bearing
        gesture.enabled: false

        onMapReadyChanged: {
            console.log("unit03 navigation mapReady=" + mapReady
                        + " error=" + error + " errorString=" + errorString)
            if (mapReady && supportedMapTypes.length > 0)
                activeMapType = supportedMapTypes[0]
        }

        Connections {
            target: navigationController
            function onPositionChanged() {
                map.center = navigationController.coordinate
                map.bearing = navigationController.bearing
            }
        }

        MapQuickItem {
            coordinate: navigationController.coordinate
            anchorPoint.x: marker.width / 2
            anchorPoint.y: marker.height / 2
            z: 10

            sourceItem: Rectangle {
                id: marker
                width: 18
                height: 18
                radius: 9
                color: "#1976d2"
                border.color: "white"
                border.width: 3
            }
        }
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: 42
        color: "#dd166534"
        z: 20
        Text {
            anchors.centerIn: parent
            color: "white"
            font.bold: true
            text: qsTr("SIMULATED 40 KM/H — REAL HANOI MAP DATA — © OpenStreetMap contributors, ODbL 1.0")
        }
    }

    Rectangle {
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.margins: 8
        width: statusText.implicitWidth + 20
        height: 50
        color: "#dd111111"
        z: 20
        Text {
            id: statusText
            anchors.centerIn: parent
            color: "white"
            text: qsTr("Route %1 km; travelled %2 km\nContinuous camera follow")
                .arg((navigationController.routeLengthMeters / 1000.0).toFixed(2))
                .arg((navigationController.travelledMeters / 1000.0).toFixed(2))
        }
    }

    PerformanceOverlay {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.topMargin: 50
        anchors.rightMargin: 8
        monitor: performanceMonitor
        visible: instrumentationEnabled
        z: 20
    }

    Component.onCompleted: {
        performanceMonitor.beginInteractionWindow(navigationInteractionMilliseconds)
        navigationController.start(40.0)
    }
}
