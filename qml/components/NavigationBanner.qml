import QtQuick 2.13
import "../theme"

Rectangle {
    id: root

    property string instruction: ""
    property string streetName: ""
    property string sourceLabel: ""
    property real distanceToManeuverMeters: 0
    property real remainingDistanceMeters: 0
    property real remainingDurationSeconds: 0
    property bool arrived: false

    readonly property string primaryText: arrived
                                                ? qsTr("Đã đến đích")
                                                : (instruction.length > 0
                                                   ? instruction
                                                   : qsTr("Tiếp tục theo tuyến"))
    readonly property string maneuverDistanceText: arrived
                                                         ? qsTr("0 m")
                                                         : formatDistance(distanceToManeuverMeters)
    readonly property string remainingText: qsTr("Còn %1 • %2 phút")
                                               .arg(formatDistance(remainingDistanceMeters))
                                               .arg(Math.max(0, Math.ceil(remainingDurationSeconds / 60)))
    readonly property string sourceText: qsTr("Nguồn: %1").arg(sourceLabel)

    function formatDistance(meters) {
        if (!isFinite(meters) || meters <= 0)
            return qsTr("0 m")
        if (meters < 1000)
            return qsTr("%1 m").arg(Math.round(meters))
        return qsTr("%1 km").arg((meters / 1000).toFixed(1))
    }

    width: 480
    height: 118
    radius: Theme.radiusPanel
    color: "#f2111720"
    border.color: arrived ? Theme.success : Theme.accent
    border.width: 2

    Text {
        id: distanceText
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: Theme.spaceLg
        text: root.maneuverDistanceText
        color: root.arrived ? Theme.success : Theme.accent
        font.pixelSize: 26
        font.bold: true
    }

    Text {
        anchors.left: distanceText.right
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.leftMargin: Theme.spaceLg
        anchors.rightMargin: Theme.spaceLg
        anchors.topMargin: Theme.spaceMd
        text: root.primaryText
        color: Theme.textPrimary
        font.pixelSize: Theme.fontSizeTitle
        font.bold: true
        elide: Text.ElideRight
    }

    Text {
        anchors.left: distanceText.right
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.leftMargin: Theme.spaceLg
        anchors.rightMargin: Theme.spaceLg
        anchors.topMargin: 42
        text: root.streetName
        color: Theme.textSecondary
        font.pixelSize: Theme.fontSizeBody
        elide: Text.ElideRight
    }

    Text {
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.margins: Theme.spaceLg
        text: root.remainingText
        color: Theme.textPrimary
        font.pixelSize: Theme.fontSizeCaption
    }

    Text {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: Theme.spaceLg
        text: root.sourceText
        color: Theme.warning
        font.pixelSize: Theme.fontSizeCaption
    }
}
