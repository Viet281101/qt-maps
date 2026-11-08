import QtQuick 2.13
import QtQuick.Window 2.13
import QtLocation 5.13
import QtPositioning 5.13

Window {
    id: root
    width: 256
    height: 256
    visible: true
    color: "#f2efe9"
    flags: Qt.FramelessWindowHint

    Plugin {
        id: mapPlugin
        name: "maplibre"
        allowExperimental: true
        PluginParameter { name: "maplibre.map.styles"; value: vectorStyleUrl }
    }

    Map {
        id: renderMap
        objectName: "rasterRenderMap"
        anchors.fill: parent
        plugin: mapPlugin
        center: QtPositioning.coordinate(21.0278, 105.8342)
        zoomLevel: 12
        gesture.enabled: false
        copyrightsVisible: false

        onMapReadyChanged: {
            if (mapReady)
                rasterRenderController.mapReady(renderMap)
        }
    }
}
