import QtQuick 2.13
import "../theme"

Rectangle {
    id: root
    property int maneuverIndex: -1
    property var maneuver
    property bool selected: false
    signal activated(int maneuverIndex)

    height: Math.max(54, instructionText.implicitHeight + 22)
    radius: Theme.radiusPanel
    color: selected ? Theme.accent
                    : maneuverMouse.containsMouse ? "#e6334352" : "#b8222d38"
    border.color: selected ? Theme.routeOutline : Theme.border
    border.width: selected ? 2 : 1

    Text {
        id: instructionText
        anchors.left: parent.left
        anchors.right: distanceText.left
        anchors.verticalCenter: parent.verticalCenter
        anchors.margins: Theme.spaceSm
        color: root.selected ? "#08243a" : Theme.textPrimary
        wrapMode: Text.WordWrap
        text: (root.maneuverIndex + 1) + ". "
              + (root.maneuver ? root.maneuver.instruction : "")
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
