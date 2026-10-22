import QtQuick 2.13
import QtQuick.Window 2.13
import QtLocation 5.13
import QtPositioning 5.13

Window {
    width: 960
    height: 640
    visible: true
    title: qsTr("QtMaps unit 01 Mapbox GL style-only probe")

    Plugin {
        id: mapboxGlPlugin
        name: "mapboxgl"
        allowExperimental: true

        PluginParameter {
            name: "mapboxgl.mapping.additional_style_urls"
            value: "http://127.0.0.1:8766/style.json"
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
        zoomLevel: 12

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
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: 40
        color: "#d97706"

        Text {
            anchors.centerIn: parent
            color: "#111111"
            font.bold: true
            text: qsTr("MAPBOX GL LOCAL STYLE ONLY — NO VECTOR TILES")
        }
    }
}
