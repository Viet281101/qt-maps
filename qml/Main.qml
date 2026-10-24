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
    title: qsTr("QtMaps diagnostic raster with performance instrumentation")
    property bool performanceOverlayVisible: instrumentationEnabled
    property int automationStep: 0

    Plugin {
        id: osmPlugin
        name: "osm"
        allowExperimental: true

        PluginParameter {
            name: "osm.mapping.custom.host"
            value: rasterTileBaseUrl
        }
        PluginParameter {
            name: "osm.mapping.providersrepository.disabled"
            value: true
        }
        PluginParameter {
            name: "osm.mapping.prefetching_style"
            value: "NoPrefetching"
        }
        PluginParameter {
            name: "osm.useragent"
            value: "QtMaps-unit01-diagnostic/0.1 (localhost-only)"
        }
        PluginParameter {
            name: "osm.mapping.custom.mapcopyright"
            value: "Diagnostic fixture generated locally"
        }
        PluginParameter {
            name: "osm.mapping.custom.datacopyright"
            value: "NOT REAL GEOGRAPHIC MAP DATA"
        }
    }

    Map {
        id: map
        anchors.fill: parent
        plugin: osmPlugin
        center: QtPositioning.coordinate(21.0278, 105.8342)
        minimumZoomLevel: 10
        maximumZoomLevel: 14
        zoomLevel: 12
        gesture.enabled: true
        focus: true

        Keys.onPressed: {
            if (event.key === Qt.Key_F3 && instrumentationEnabled) {
                performanceOverlayVisible = !performanceOverlayVisible
                event.accepted = true
            } else if (event.key === Qt.Key_Left) {
                performanceMonitor.beginInteractionWindow(450)
                pan(128, 0)
                event.accepted = true
            } else if (event.key === Qt.Key_Right) {
                performanceMonitor.beginInteractionWindow(450)
                pan(-128, 0)
                event.accepted = true
            } else if (event.key === Qt.Key_Up) {
                performanceMonitor.beginInteractionWindow(450)
                pan(0, 128)
                event.accepted = true
            } else if (event.key === Qt.Key_Down) {
                performanceMonitor.beginInteractionWindow(450)
                pan(0, -128)
                event.accepted = true
            } else if (event.key === Qt.Key_Plus || event.key === Qt.Key_Equal) {
                performanceMonitor.beginInteractionWindow(450)
                zoomLevel = Math.min(maximumZoomLevel, zoomLevel + 1)
                event.accepted = true
            } else if (event.key === Qt.Key_Minus) {
                performanceMonitor.beginInteractionWindow(450)
                zoomLevel = Math.max(minimumZoomLevel, zoomLevel - 1)
                event.accepted = true
            }
        }

        function selectCustomMapType() {
            for (var index = 0; index < supportedMapTypes.length; ++index) {
                if (supportedMapTypes[index].name === "Custom URL Map") {
                    activeMapType = supportedMapTypes[index]
                    center = QtPositioning.coordinate(21.0278, 105.8342)
                    zoomLevel = 12
                    console.log("activeMapType=Custom URL Map")
                    return
                }
            }
            console.warn("Custom URL Map type is unavailable")
        }

        onMapReadyChanged: {
            console.log("mapReady=" + mapReady + " error=" + error + " errorString=" + errorString)
            if (mapReady)
                selectCustomMapType()
        }
        onZoomLevelChanged: console.log("zoomLevel=" + zoomLevel)
    }

    Timer {
        interval: 500
        running: benchmarkAutomation
        repeat: true
        onTriggered: {
            var phase = automationStep % 8
            if (phase === 0 || phase === 1) {
                performanceMonitor.beginInteractionWindow(450)
                map.pan(-64, 0)
            } else if (phase === 2 || phase === 3) {
                performanceMonitor.beginInteractionWindow(450)
                map.pan(64, 0)
            } else if (phase === 4) {
                performanceMonitor.beginInteractionWindow(450)
                map.zoomLevel = Math.min(map.maximumZoomLevel, map.zoomLevel + 1)
            } else if (phase === 6) {
                performanceMonitor.beginInteractionWindow(450)
                map.zoomLevel = Math.max(map.minimumZoomLevel, map.zoomLevel - 1)
            }
            automationStep += 1
        }
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: 40
        color: "#e6b800"

        Text {
            anchors.centerIn: parent
            color: "#111111"
            font.bold: true
            text: fallbackReason.length > 0
                  ? qsTr("DIAGNOSTIC RASTER FALLBACK — %1").arg(fallbackReason)
                  : qsTr("DIAGNOSTIC XYZ TILES — NOT A REAL GEOGRAPHIC MAP")
        }
    }

    Rectangle {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 28
        width: controlsLabel.implicitWidth + 24
        height: 32
        color: "#dd111111"

        Text {
            id: controlsLabel
            anchors.centerIn: parent
            color: "white"
            text: qsTr("Arrow keys: pan    + / −: zoom")
        }
    }

    Rectangle {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.topMargin: 48
        anchors.rightMargin: 8
        width: 82
        height: 28
        color: "#dd111111"
        visible: instrumentationEnabled
        z: 20

        Text {
            anchors.centerIn: parent
            color: "white"
            text: qsTr("Perf [F3]")
        }
        MouseArea {
            anchors.fill: parent
            onClicked: performanceOverlayVisible = !performanceOverlayVisible
        }
    }

    PerformanceOverlay {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.topMargin: 84
        anchors.rightMargin: 8
        monitor: performanceMonitor
        visible: instrumentationEnabled && performanceOverlayVisible
        z: 20
    }
}
