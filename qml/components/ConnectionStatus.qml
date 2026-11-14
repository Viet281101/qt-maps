import QtQuick 2.13
import "../theme"

Item {
    id: statusOverlay
    property color statusColor: Theme.warning
    property bool navigationActive: false
    property real navigationBannerHeight: 0
    property real normalTopMargin: Theme.spaceMd
    property string connectionLabel: ""
    readonly property string statusText: friendlyConnectionLabel(connectionLabel)
    readonly property real attributionHeight: attributionPill.height + Theme.spaceSm

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
                           : statusOverlay.normalTopMargin
        width: Math.min(264, parent.width - Theme.spaceMd * 2)
        height: 32
        radius: 17
        color: "#dd111820"
        border.color: Theme.border
        z: 20
        Row {
            id: statusRow
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: Theme.spaceSm + 2
            anchors.rightMargin: Theme.spaceSm + 2
            spacing: Theme.spaceSm
            Rectangle {
                id: statusDot
                anchors.verticalCenter: parent.verticalCenter
                width: 10; height: 10; radius: 5
                color: statusOverlay.statusColor
            }
            Text {
                width: Math.max(0, parent.width - statusDot.width - parent.spacing)
                color: Theme.textPrimary
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeBody
                elide: Text.ElideRight
                text: statusOverlay.statusText
            }
        }
    }

    Rectangle {
        id: attributionPill
        objectName: "attributionPill"
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: Theme.spaceSm
        width: attribution.implicitWidth + Theme.spaceMd
        height: 24
        color: "#cc111820"
        z: 20
        Text {
            id: attribution
            anchors.centerIn: parent
            color: Theme.textPrimary
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeCaption
            text: "© OpenStreetMap contributors, ODbL 1.0"
        }
    }
}
