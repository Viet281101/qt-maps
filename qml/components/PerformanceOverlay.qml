import QtQuick 2.13
import "../theme"

Rectangle {
    id: root
    property var monitor
    width: 224
    height: content.height + Theme.spaceMd * 2
    radius: Theme.radiusPanel
    color: Theme.surfaceElevated
    border.color: Theme.border
    border.width: 1

    Column {
        id: content
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: Theme.spaceMd
        spacing: Theme.spaceXs

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
            color: Theme.textSecondary
            text: qsTr("Frame ms p50/p95/p99 %1/%2/%3")
                .arg(monitor ? monitor.frameMetrics.frameTimeP50Milliseconds.toFixed(1) : "0.0")
                .arg(monitor ? monitor.frameMetrics.frameTimeP95Milliseconds.toFixed(1) : "0.0")
                .arg(monitor ? monitor.frameMetrics.frameTimeP99Milliseconds.toFixed(1) : "0.0")
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
            height: 30
            radius: 4
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
