import QtQuick 2.13
import "../theme"

Rectangle {
    id: root
    property int maneuverIndex: -1
    property var maneuver
    property bool selected: false
    signal activated(int maneuverIndex)

    function directionSymbol(type) {
        if (type === 4 || type === 5)
            return "●"
        if (type === 15 || type === 16 || type === 17 || type === 18)
            return "↱"
        if (type === 8 || type === 9 || type === 11 || type === 12)
            return "↰"
        if (type === 23 || type === 24 || type === 25 || type === 26)
            return "⟳"
        return "↑"
    }

    height: Math.max(60, instructionColumn.implicitHeight + Theme.spaceLg)
    radius: Theme.radiusControl
    color: selected ? Theme.accent
                    : maneuverMouse.containsMouse ? "#e6334352" : "#b8222d38"
    border.color: selected ? Theme.routeOutline : Theme.border
    border.width: selected ? 2 : 1

    Rectangle {
        id: directionIcon
        objectName: "maneuverDirectionIcon"
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: Theme.spaceSm
        width: 36; height: 36; radius: 18
        color: root.selected ? "#e8f3ff" : Theme.surfacePressed
        Text {
            objectName: "maneuverDirectionSymbol"
            anchors.centerIn: parent
            color: root.selected ? "#12334a" : Theme.accent
            font.pixelSize: 22
            font.bold: true
            text: root.directionSymbol(root.maneuver ? root.maneuver.type : 0)
        }
    }

    Column {
        id: instructionColumn
        anchors.left: directionIcon.right
        anchors.right: distanceText.left
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: Theme.spaceSm
        anchors.rightMargin: Theme.spaceSm
        spacing: 2
        Text {
            width: parent.width
            color: root.selected ? "#08243a" : Theme.textPrimary
            font.bold: true
            elide: Text.ElideRight
            text: root.maneuver && root.maneuver.streetName
                  && root.maneuver.streetName.length > 0
                  ? root.maneuver.streetName
                  : (root.maneuver ? root.maneuver.instruction : "")
        }
        Text {
            width: parent.width
            visible: root.maneuver && root.maneuver.streetName
                     && root.maneuver.streetName.length > 0
            color: root.selected ? "#12334a" : Theme.textSecondary
            font.pixelSize: Theme.fontSizeCaption
            elide: Text.ElideRight
            text: root.maneuver ? root.maneuver.instruction : ""
        }
    }
    Text {
        id: distanceText
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.rightMargin: Theme.spaceSm
        width: 58
        horizontalAlignment: Text.AlignRight
        color: root.selected ? "#12334a" : Theme.textSecondary
        text: !root.maneuver ? ""
              : root.maneuver.distanceMeters >= 1000
                  ? (root.maneuver.distanceMeters / 1000).toFixed(1) + " km"
                  : Math.round(root.maneuver.distanceMeters) + " m"
    }
    MouseArea {
        id: maneuverMouse
        anchors.fill: parent
        hoverEnabled: true
        onClicked: root.activated(root.maneuverIndex)
    }
}
