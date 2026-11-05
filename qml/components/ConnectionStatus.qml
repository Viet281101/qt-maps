import QtQuick 2.13
import "../theme"

Item {
    id: statusOverlay
    property color statusColor: Theme.warning
    property bool navigationActive: false
    property real navigationBannerHeight: 0
    property string connectionLabel: ""
    readonly property string statusText: friendlyConnectionLabel(connectionLabel)

    function friendlyConnectionLabel(label) {
        if (label === "Valhalla local — loopback")
            return qsTr("Valhalla (cục bộ)")
        if (label === "offline — fixture fallback")
            return qsTr("Ngoại tuyến · tuyến dự phòng")
        if (label === "offline — fixture")
            return qsTr("Ngoại tuyến · tuyến mô phỏng")
        return label.replace("Valhalla local", qsTr("Valhalla cục bộ"))
    }

    Rectangle {
        id: connectionStatusPill
        objectName: "connectionStatusPill"
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.rightMargin: Theme.spaceMd
        anchors.topMargin: statusOverlay.navigationActive
                           ? statusOverlay.navigationBannerHeight + Theme.spaceMd * 2
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
                color: statusOverlay.statusColor
            }
            Text {
                color: Theme.textPrimary
                font.pixelSize: Theme.fontSizeBody
                text: statusOverlay.statusText
            }
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
            font.pixelSize: Theme.fontSizeBody
            text: "© OpenStreetMap contributors, ODbL 1.0"
        }
    }
}
