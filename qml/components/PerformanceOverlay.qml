import QtQuick 2.13
import "../theme"

Rectangle {
    id: root
    property var monitor
    property bool detailed: false
    width: detailed ? 282 : compactRow.implicitWidth + Theme.spaceLg * 2
    height: detailed ? content.height + Theme.spaceMd * 2 : 36
    radius: detailed ? Theme.radiusPanel : Theme.radiusPill
    color: Theme.surfaceElevated
    border.color: Theme.border
    border.width: 1

    Row {
        id: compactRow
        anchors.centerIn: parent
        visible: !root.detailed
        spacing: Theme.spaceSm
        Text {
            color: Theme.textPrimary
            font.bold: true
            text: qsTr("FPS %1").arg(root.monitor
                                      ? root.monitor.frameMetrics.currentFps.toFixed(0) : "0")
        }
        Rectangle { width: 1; height: 16; color: Theme.border }
        Text {
            color: Theme.textSecondary
            text: qsTr("RSS %1 MiB").arg(root.monitor ? root.monitor.totalRssMiB.toFixed(0) : "0")
        }
        Text { color: Theme.textSecondary; text: qsTr("F3 chi tiết") }
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
            color: Theme.textPrimary
            font.bold: true
            text: qsTr("Performance")
        }
        Text {
            color: Theme.textPrimary
            text: qsTr("FPS %1   avg %2")
                .arg(monitor ? monitor.frameMetrics.currentFps.toFixed(1) : "0.0")
                .arg(monitor ? monitor.frameMetrics.averageFps.toFixed(1) : "0.0")
        }
        Text {
            color: Theme.textSecondary
            text: qsTr("Active avg %1 FPS").arg(monitor ? monitor.frameMetrics.interactionAverageFps.toFixed(1) : "0.0")
        }
        Text {
            width: parent.width
            color: Theme.textSecondary
            wrapMode: Text.Wrap
            text: qsTr("Frame ms p50/p95/p99 %1/%2/%3")
                .arg(monitor ? monitor.frameMetrics.frameTimeP50Milliseconds.toFixed(1) : "0.0")
                .arg(monitor ? monitor.frameMetrics.frameTimeP95Milliseconds.toFixed(1) : "0.0")
                .arg(monitor ? monitor.frameMetrics.frameTimeP99Milliseconds.toFixed(1) : "0.0")
        }
        Text {
            color: Theme.textSecondary
            text: qsTr("Settling max %1 ms")
                .arg(monitor ? monitor.frameMetrics.settlingFrameMaximumMilliseconds.toFixed(1) : "0.0")
        }
        Text {
            color: Theme.textSecondary
            text: qsTr("CPU %1%   RSS %2 MiB")
                .arg(monitor ? monitor.totalCpuPercent.toFixed(1) : "0.0")
                .arg(monitor ? monitor.totalRssMiB.toFixed(1) : "0.0")
        }
        Text {
            color: Theme.textSecondary
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
