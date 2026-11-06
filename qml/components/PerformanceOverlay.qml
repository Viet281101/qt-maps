import QtQuick 2.13
import "../theme"

Rectangle {
    id: root
    objectName: "performanceOverlay"
    property var monitor
    property bool detailed: false
    readonly property int maximumWidth: Theme.spaceXl * 12
    readonly property bool processSampleAvailable: monitor && monitor.totalRssMiB > 0
    readonly property string compactRssText: processSampleAvailable
                                                   ? qsTr("RSS %1 MiB").arg(
                                                         monitor.totalRssMiB.toFixed(0))
                                                   : qsTr("RSS —")
    width: detailed ? maximumWidth : Math.min(maximumWidth, compactRow.implicitWidth
                                               + Theme.spaceLg * 2)
    height: detailed ? content.height + Theme.spaceMd * 2
                     : Theme.spaceLg * 2 + Theme.spaceXs
    radius: detailed ? Theme.radiusPanel : Theme.radiusPill
    color: Theme.surfaceElevated
    border.color: Theme.border
    border.width: 1

    Row {
        id: compactRow
        objectName: "performanceCompactBadge"
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: Theme.spaceLg
        anchors.rightMargin: Theme.spaceLg
        clip: true
        visible: !root.detailed
        spacing: Theme.spaceSm
        Text {
            id: compactFps
            color: Theme.textPrimary
            font.bold: true
            font.pixelSize: Theme.fontSizeBody
            elide: Text.ElideRight
            text: qsTr("FPS %1").arg(root.monitor
                                      ? root.monitor.frameMetrics.currentFps.toFixed(0) : "0")
        }
        Rectangle { id: compactSeparator; width: 1; height: Theme.spaceLg; color: Theme.border }
        Text {
            id: compactRss
            width: Math.max(0, parent.width - compactFps.implicitWidth
                            - compactSeparator.width - compactHint.implicitWidth
                            - parent.spacing * 2)
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSizeBody
            elide: Text.ElideRight
            text: root.compactRssText
        }
        Text {
            id: compactHint
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSizeBody
            elide: Text.ElideRight
            text: qsTr("F3 chi tiết")
        }
    }

    Column {
        id: content
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: Theme.spaceMd
        spacing: Theme.spaceXs
        visible: root.detailed

        Text {
            width: parent.width
            color: Theme.textPrimary
            font.bold: true
            font.pixelSize: Theme.fontSizeBody
            elide: Text.ElideRight
            text: qsTr("Performance")
        }
        Text {
            width: parent.width
            color: Theme.textPrimary
            font.pixelSize: Theme.fontSizeBody
            elide: Text.ElideRight
            text: qsTr("FPS %1   avg %2")
                .arg(monitor ? monitor.frameMetrics.currentFps.toFixed(1) : "0.0")
                .arg(monitor ? monitor.frameMetrics.averageFps.toFixed(1) : "0.0")
        }
        Text {
            width: parent.width
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSizeBody
            elide: Text.ElideRight
            text: qsTr("Active avg %1 FPS").arg(monitor ? monitor.frameMetrics.interactionAverageFps.toFixed(1) : "0.0")
        }
        Text {
            width: parent.width
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSizeBody
            wrapMode: Text.Wrap
            text: qsTr("Frame ms p50/p95/p99 %1/%2/%3")
                .arg(monitor ? monitor.frameMetrics.frameTimeP50Milliseconds.toFixed(1) : "0.0")
                .arg(monitor ? monitor.frameMetrics.frameTimeP95Milliseconds.toFixed(1) : "0.0")
                .arg(monitor ? monitor.frameMetrics.frameTimeP99Milliseconds.toFixed(1) : "0.0")
        }
        Text {
            width: parent.width
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSizeBody
            elide: Text.ElideRight
            text: qsTr("Settling max %1 ms")
                .arg(monitor ? monitor.frameMetrics.settlingFrameMaximumMilliseconds.toFixed(1) : "0.0")
        }
        Text {
            width: parent.width
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSizeBody
            elide: Text.ElideRight
            text: root.processSampleAvailable
                  ? qsTr("CPU %1%   RSS %2 MiB")
                        .arg(monitor.totalCpuPercent.toFixed(1))
                        .arg(monitor.totalRssMiB.toFixed(1))
                  : qsTr("CPU —   RSS —")
        }
        Text {
            width: parent.width
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSizeBody
            elide: Text.ElideRight
            text: qsTr("Child processes: %1").arg(monitor ? monitor.childCount : 0)
        }
        Rectangle {
            width: parent.width
            height: 36
            radius: Theme.radiusControl
            color: monitor && monitor.recording ? Theme.error : Theme.accent

            Text {
                anchors.centerIn: parent
                color: "#111111"
                font.bold: true
                font.pixelSize: Theme.fontSizeBody
                elide: Text.ElideRight
                text: monitor && monitor.recording ? qsTr("Stop recording") : qsTr("Start benchmark")
            }
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    if (monitor)
                        monitor.recording ? monitor.stopBenchmark() : monitor.startBenchmark("")
                }
            }
        }
        Text {
            width: parent.width
            visible: monitor && monitor.errorMessage.length > 0
            wrapMode: Text.Wrap
            color: Theme.error
            font.pixelSize: Theme.fontSizeCaption
            text: monitor ? monitor.errorMessage : ""
        }
    }
}
