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
    title: qsTr("QtMaps unit 01b local-vector feasibility probe")
    property bool performanceOverlayVisible: instrumentationEnabled
    property int automationStep: 0

    Plugin {
        id: mapboxGlPlugin
        name: "mapboxgl"
        allowExperimental: true

        PluginParameter {
            name: "mapboxgl.mapping.additional_style_urls"
            value: vectorStyleUrl
        }
        PluginParameter {
            name: "mapboxgl.mapping.cache.memory"
            value: true
        }
    }

    Map {
        id: map
        anchors.fill: parent
        plugin: mapboxGlPlugin
        center: QtPositioning.coordinate(21.0278, 105.8342)
        zoomLevel: 14
        bearing: cameraProbeController.requestedBearing
        tilt: cameraProbeController.requestedTilt
        gesture.enabled: true
        focus: true

        Keys.onPressed: {
            if (event.key === Qt.Key_F3 && instrumentationEnabled) {
                performanceOverlayVisible = !performanceOverlayVisible
                event.accepted = true
            } else if (event.key === Qt.Key_C) {
                cameraProbeController.advance(minimumTilt, maximumTilt)
                event.accepted = true
            } else if (event.key === Qt.Key_N) {
                cameraProbeController.reset()
                event.accepted = true
            }
        }

        function selectLocalStyle() {
            if (supportedMapTypes.length === 0) {
                console.warn("mapboxgl supportedMapTypes is empty")
                return
            }
            activeMapType = supportedMapTypes[0]
            console.log("mapboxgl activeMapType=" + activeMapType.name
                        + " description=" + activeMapType.description)
        }

        onMapReadyChanged: {
            console.log("mapboxgl mapReady=" + mapReady
                        + " error=" + error + " errorString=" + errorString)
            if (mapReady)
                selectLocalStyle()
        }
        onBearingChanged: console.log("camera observed bearing=" + bearing
                                      + " requested=" + cameraProbeController.requestedBearing)
        onTiltChanged: console.log("camera observed tilt=" + tilt
                                   + " requested=" + cameraProbeController.requestedTilt
                                   + " limits=" + minimumTilt + ".." + maximumTilt)
    }

    Timer {
        interval: 500
        running: benchmarkAutomation
        repeat: true
        onTriggered: {
            var phase = automationStep % 12
            if (phase === 0) {
                cameraProbeController.advance(map.minimumTilt, map.maximumTilt)
            } else if (phase === 2 || phase === 3) {
                map.pan(-64, 0)
            } else if (phase === 4 || phase === 5) {
                map.pan(64, 0)
            } else if (phase === 6) {
                map.zoomLevel = Math.min(map.maximumZoomLevel, map.zoomLevel + 1)
            } else if (phase === 9) {
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
        color: vectorDataReal ? "#166534" : "#d97706"

        Text {
            anchors.centerIn: parent
            color: vectorDataReal ? "white" : "#111111"
            font.bold: true
            text: vectorDataReal
                  ? qsTr("LOCAL VECTOR SPIKE — REAL OSM DATA — © OpenStreetMap contributors, ODbL 1.0")
                  : qsTr("LOCAL VECTOR SPIKE — SYNTHETIC FIXTURE, NOT REAL MAP DATA")
        }
    }

    Rectangle {
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.leftMargin: 8
        anchors.bottomMargin: 8
        width: cameraText.implicitWidth + 20
        height: 50
        color: "#dd111111"
        z: 20

        Text {
            id: cameraText
            anchors.centerIn: parent
            color: "white"
            text: qsTr("Dataset: %1\nCamera: %2 — C: next, N: north")
                      .arg(datasetLabel).arg(cameraProbeController.stepName)
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
