import QtQuick 2.13
import "../theme"

Rectangle {
    id: root

    property string instruction: ""
    property string nextInstruction: ""
    property string streetName: ""
    property string sourceLabel: ""
    property real distanceToManeuverMeters: 0
    property real remainingDistanceMeters: 0
    property real remainingDurationSeconds: 0
    property bool rerouting: false
    property bool arrived: false
    signal endRequested()

    readonly property string primaryText: arrived
                                                ? qsTr("Đã đến nơi")
                                                : rerouting
                                                  ? qsTr("Đang tìm đường mới…")
                                                  : instruction.length > 0
                                                    ? instruction
                                                    : qsTr("Tiếp tục theo tuyến")
    readonly property string maneuverDistanceText: arrived
                                                         ? qsTr("0 m")
                                                         : formatDistance(distanceToManeuverMeters)
    readonly property string remainingText: qsTr("%1 · %2 phút")
                                               .arg(formatDistance(remainingDistanceMeters))
                                               .arg(Math.max(0, Math.ceil(remainingDurationSeconds / 60)))
    readonly property string etaText: qsTr("Đến %1").arg(
                                          Qt.formatTime(new Date(Date.now()
                                                                 + remainingDurationSeconds * 1000),
                                                        "HH:mm"))
    readonly property string sourceText: qsTr("Nguồn: %1").arg(sourceLabel)

    function directionSymbol(text) {
        var normalized = text.toLowerCase()
        if (arrived)
            return "●"
        if (normalized.indexOf("vòng xuyến") >= 0)
            return "⟳"
        if (normalized.indexOf("trái") >= 0)
            return "↰"
        if (normalized.indexOf("phải") >= 0)
            return "↱"
        return "↑"
    }

    function formatDistance(meters) {
        if (!isFinite(meters) || meters <= 0)
            return qsTr("0 m")
        if (meters < 1000)
            return qsTr("%1 m").arg(Math.round(meters / 10) * 10)
        return qsTr("%1 km").arg((meters / 1000).toFixed(1).replace(".", ","))
    }

    width: Math.min(600, parent ? parent.width - Theme.spaceXl * 2 : 600)
    height: 158
    radius: Theme.radiusPanel
    color: Theme.surfaceElevated
    border.color: arrived ? Theme.success : rerouting ? Theme.warning : Theme.accent
    border.width: 2

    Text {
        id: directionIcon
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.leftMargin: Theme.spaceLg
        anchors.topMargin: Theme.spaceMd
        width: 52
        horizontalAlignment: Text.AlignHCenter
        text: root.directionSymbol(root.instruction)
        color: root.arrived ? Theme.success : root.rerouting ? Theme.warning : Theme.accent
        font.family: Theme.fontFamily
        font.pixelSize: 42
        font.bold: true
    }

    Column {
        anchors.left: directionIcon.right
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.leftMargin: Theme.spaceMd
        anchors.rightMargin: Theme.spaceLg
        anchors.topMargin: Theme.spaceSm
        spacing: 1

        Text {
            width: parent.width
            text: root.maneuverDistanceText
            color: root.arrived ? Theme.success : Theme.textPrimary
            font.family: Theme.fontFamily
            font.pixelSize: 30
            font.bold: true
        }
        Text {
            width: parent.width
            text: root.arrived || root.rerouting
                  ? root.primaryText
                  : root.streetName.length > 0 ? root.streetName : root.primaryText
            color: Theme.textPrimary
            font.family: Theme.fontFamily
            font.pixelSize: 18
            font.bold: true
            elide: Text.ElideRight
        }
        Text {
            objectName: "nextInstructionText"
            width: parent.width
            text: root.arrived
                  ? qsTr("Bạn đã hoàn thành tuyến")
                  : root.rerouting
                    ? qsTr("Giữ nguyên tuyến gần nhất trong lúc tính lại")
                    : root.nextInstruction.length > 0
                    ? qsTr("Tiếp theo: %1").arg(root.nextInstruction)
                    : root.primaryText
            color: root.rerouting ? Theme.warning : Theme.textSecondary
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeBody
            elide: Text.ElideRight
        }
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 44
        color: Theme.surfaceInput
        radius: Theme.radiusControl

        Row {
            anchors.left: parent.left
            anchors.right: endNavigationButton.left
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: Theme.spaceLg
            anchors.rightMargin: Theme.spaceSm
            spacing: Theme.spaceLg
            Text {
                width: Math.max(0, parent.width * 0.42)
                color: Theme.textPrimary
                font.family: Theme.fontFamily
                font.bold: true
                elide: Text.ElideRight
                text: root.remainingText
            }
            Text {
                width: Math.max(0, parent.width * 0.30)
                color: Theme.textSecondary
                font.family: Theme.fontFamily
                elide: Text.ElideRight
                text: root.etaText
            }
            Text {
                width: Math.max(0, parent.width * 0.28 - Theme.spaceLg * 2)
                color: Theme.warning
                font.family: Theme.fontFamily
                elide: Text.ElideRight
                text: root.sourceText
            }
        }

        KeyboardActionButton {
            id: endNavigationButton
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.rightMargin: Theme.spaceXs
            width: 92
            height: 36
            text: qsTr("Kết thúc")
            normalColor: Theme.error
            pressedColor: Theme.surfacePressed
            textColor: "#ffffff"
            onTriggered: root.endRequested()
        }
    }
}
