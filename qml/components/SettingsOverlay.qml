import QtQuick 2.13
import "../theme"

Item {
    id: settings
    property bool open: false
    signal toggled()
    KeyboardActionButton {
        id: button
        objectName: "settingsButton"
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: Theme.spaceMd
        width: 40; height: 40
        labelPixelSize: 22
        text: "⚙"
        normalColor: Theme.surfaceElevated
        pressedColor: Theme.surfacePressed
        textColor: Theme.textPrimary
        onTriggered: settings.open = !settings.open
    }
    Rectangle {
        visible: settings.open
        anchors.right: button.right
        anchors.top: button.bottom
        anchors.topMargin: Theme.spaceXs
        width: 210; height: column.implicitHeight + Theme.spaceLg * 2
        radius: Theme.radiusPanel
        color: Theme.surfaceElevated
        border.color: Theme.border
        Column {
            id: column
            anchors.fill: parent; anchors.margins: Theme.spaceLg
            spacing: Theme.spaceSm
            Text { color: Theme.textPrimary; font.family: Theme.fontFamily; font.bold: true; text: qsTr("Ngôn ngữ") }
            KeyboardActionButton { width: parent.width; text: appPreferences.language === "vi" ? qsTr("✓ Tiếng Việt") : qsTr("Tiếng Việt"); onTriggered: appPreferences.setLanguage("vi") }
            KeyboardActionButton { width: parent.width; text: appPreferences.language === "en" ? qsTr("✓ English") : qsTr("English"); onTriggered: appPreferences.setLanguage("en") }
        }
    }
}
