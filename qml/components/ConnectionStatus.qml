import QtQuick 2.13
import "../theme"
import "RuntimeStatus.js" as RuntimeStatus

Item {
    id: statusOverlay
    property color statusColor: Theme.warning
    property string connectionLabel: ""
    property string attributionText: "© OpenStreetMap contributors, ODbL 1.0"
    property bool layerControlVisible: true
    property bool layerMenuOpen: false
    property real layerMenuHeight: 0
    readonly property string statusText: friendlyConnectionLabel(connectionLabel)
    readonly property bool requiresMapLevelStatus: connectionLabel !== "Valhalla local — loopback"
    readonly property real attributionHeight: attributionPill.height + Theme.spaceSm
    readonly property real statusBottomMargin: attributionHeight + Theme.spaceXs
                                               + (layerControlVisible ? 36 + Theme.spaceSm : 0)
                                               + (layerMenuOpen ? layerMenuHeight + Theme.spaceXs : 0)

    function friendlyConnectionLabel(label) {
        if (label === "Valhalla local — loopback")
            return qsTr("Valhalla (cục bộ)")
        if (label === "offline — fixture fallback")
            return qsTr("Ngoại tuyến · tuyến dự phòng")
        if (label === "offline — fixture")
            return qsTr("Ngoại tuyến · tuyến mô phỏng")
        return RuntimeStatus.text(label, appPreferences ? appPreferences.language : "vi")
                            .replace("Valhalla local", qsTr("Valhalla cục bộ"))
    }

    Rectangle {
        id: connectionStatusPill
        objectName: "connectionStatusPill"
        visible: statusOverlay.requiresMapLevelStatus
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.rightMargin: Theme.spaceMd
        anchors.bottomMargin: statusOverlay.statusBottomMargin
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
            text: statusOverlay.attributionText
        }
    }
}
