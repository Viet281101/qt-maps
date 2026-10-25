import QtQuick 2.13
import "../theme"

Rectangle {
    id: root
    property string text
    property color normalColor: Theme.accent
    property color pressedColor: "#2266aa"
    property color textColor: "#08243a"
    signal triggered()

    height: 38
    radius: Theme.radiusPanel
    color: buttonMouse.pressed ? pressedColor : normalColor
    opacity: enabled ? 1 : 0.55
    border.color: activeFocus ? Theme.routeOutline : Theme.border
    border.width: activeFocus ? 2 : 1
    activeFocusOnTab: enabled

    Text {
        anchors.centerIn: parent
        color: root.textColor
        font.bold: true
        text: root.text
    }
    MouseArea {
        id: buttonMouse
        anchors.fill: parent
        enabled: root.enabled
        onClicked: {
            root.forceActiveFocus()
            root.triggered()
        }
    }
    Keys.onReturnPressed: triggered()
    Keys.onEnterPressed: triggered()
    Keys.onSpacePressed: triggered()
}
