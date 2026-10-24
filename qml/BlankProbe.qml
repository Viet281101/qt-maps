import QtQuick 2.13
import QtQuick.Window 2.13
import "components"

Window {
    id: root
    width: 960
    height: 640
    visible: true
    color: "#20252b"
    title: qsTr("QtMaps blank performance probe")
    property bool performanceOverlayVisible: instrumentationEnabled

    Text {
        anchors.centerIn: parent
        color: "#d8dee7"
        text: qsTr("QtMaps blank Release probe")
    }

    Rectangle {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 8
        width: 82
        height: 28
        color: "#dd111111"
        visible: instrumentationEnabled

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
        anchors.topMargin: 44
        anchors.rightMargin: 8
        monitor: performanceMonitor
        visible: instrumentationEnabled && performanceOverlayVisible
    }

    Item {
        anchors.fill: parent
        focus: true
        Keys.onPressed: {
            if (event.key === Qt.Key_F3 && instrumentationEnabled) {
                performanceOverlayVisible = !performanceOverlayVisible
                event.accepted = true
            }
        }
    }
}
