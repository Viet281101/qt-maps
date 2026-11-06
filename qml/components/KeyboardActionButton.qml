import QtQuick 2.13
import "../theme"

Rectangle {
    id: root
    property string text
    property color normalColor: Theme.accent
    property color pressedColor: "#2266aa"
    property color textColor: "#08243a"
    readonly property real labelImplicitWidth: label.implicitWidth
    readonly property real labelMaximumWidth: width - 2 * Theme.spaceSm
    signal triggered()

    height: 38
    radius: Theme.radiusPanel
    color: buttonMouse.pressed ? pressedColor : normalColor
    opacity: enabled ? 1 : 0.55
    border.color: activeFocus ? Theme.routeOutline : Theme.border
    border.width: activeFocus ? 2 : 1
    activeFocusOnTab: enabled

    Text {
        id: label
        objectName: "buttonLabel"
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        width: Math.max(0, root.labelMaximumWidth)
        color: root.textColor
        font.pixelSize: Theme.fontSizeBody
        font.bold: true
        text: root.text
        elide: Text.ElideRight
        horizontalAlignment: Text.AlignHCenter
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
