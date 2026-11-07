import QtQuick 2.13
import "../theme"

Rectangle {
    id: routePanel
    width: 330
    radius: Theme.radiusPanel
    color: Theme.surfaceElevated
    border.color: Theme.border
    property color routeSummaryColor: Theme.warning
    property string compactSource: qsTr("Fixture")
    readonly property real maneuverListMinimumHeight: 120
    signal recenterRequested()

    function applyEndpoints() {
        var requestedOrigin = originField.text
        var requestedDestination = destinationField.text
        if (routingController.setOriginInput(requestedOrigin))
            routingController.setDestinationInput(requestedDestination)
    }

    function calculateRequestedRoute() {
        var requestedOrigin = originField.text
        var requestedDestination = destinationField.text
        if (routingController.setOriginInput(requestedOrigin)
                && routingController.setDestinationInput(requestedDestination)) {
            if (routingFixtureOnly)
                routingController.calculateFixtureRoute()
            else
                routingController.calculateRoute()
        }
    }

    Flickable {
        id: routeControlsScroll
        objectName: "routeControlsScroll"
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: Theme.spaceLg
        height: Math.max(0, Math.min(routeControls.implicitHeight,
                                     routePanel.height
                                     - routePanel.maneuverListMinimumHeight
                                     - Theme.spaceLg * 2
                                     - Theme.spaceSm))
        contentWidth: width
        contentHeight: routeControls.implicitHeight
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        flickableDirection: Flickable.VerticalFlick

        Column {
            id: routeControls
            width: routeControlsScroll.width
            spacing: Theme.spaceSm

            Text {
                text: qsTr("Lập tuyến Hà Nội")
                color: Theme.textPrimary
                font.pixelSize: Theme.fontSizeTitle
                font.bold: true
            }

            Text {
                width: parent.width
                text: qsTr("Bấm bản đồ để chọn %1, hoặc nhập tên mẫu / tọa độ")
                    .arg(routingController.nextMapSelection)
                color: Theme.textSecondary
                wrapMode: Text.WordWrap
            }

            Rectangle {
                width: parent.width
                height: 38
                radius: Theme.radiusPanel
                color: "#f5f7fa"
                border.color: originField.activeFocus ? Theme.accent : Theme.border
                TextInput {
                    id: originField
                    objectName: "originField"
                    anchors.fill: parent
                    anchors.margins: Theme.spaceSm
                    color: "#17212b"
                    verticalAlignment: TextInput.AlignVCenter
                    selectByMouse: true
                    activeFocusOnTab: true
                    KeyNavigation.tab: destinationField
                    KeyNavigation.backtab: startNavigationButton
                    text: routingController.originInput
                    onAccepted: routingController.setOriginInput(text)
                }
            }

            Rectangle {
                width: parent.width
                height: 38
                radius: Theme.radiusPanel
                color: "#f5f7fa"
                border.color: destinationField.activeFocus ? Theme.accent : Theme.border
                TextInput {
                    id: destinationField
                    objectName: "destinationField"
                    anchors.fill: parent
                    anchors.margins: Theme.spaceSm
                    color: "#17212b"
                    verticalAlignment: TextInput.AlignVCenter
                    selectByMouse: true
                    activeFocusOnTab: true
                    KeyNavigation.tab: applyButton
                    KeyNavigation.backtab: originField
                    text: routingController.destinationInput
                    onAccepted: routingController.setDestinationInput(text)
                }
            }

            Row {
                width: parent.width
                spacing: Theme.spaceSm
                KeyboardActionButton {
                    id: applyButton
                    width: (parent.width - parent.spacing) / 2
                    text: qsTr("Áp dụng điểm")
                    normalColor: "#334352"
                    pressedColor: "#455869"
                    textColor: Theme.textPrimary
                    KeyNavigation.tab: routeButton
                    KeyNavigation.backtab: destinationField
                    onTriggered: routePanel.applyEndpoints()
                }
                KeyboardActionButton {
                    id: routeButton
                    width: (parent.width - parent.spacing) / 2
                    enabled: !routingController.loading
                    text: routingController.loading ? qsTr("Đang tính…") : qsTr("Tính đường")
                    KeyNavigation.tab: startNavigationButton
                    KeyNavigation.backtab: applyButton
                    onTriggered: routePanel.calculateRequestedRoute()
                }
            }

            Rectangle {
                width: parent.width
                height: 40
                radius: Theme.radiusPanel
                color: "#cc1b2530"
                border.color: Theme.border
                Text {
                    anchors.fill: parent
                    anchors.margins: Theme.spaceSm
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                    color: routePanel.routeSummaryColor
                    font.bold: true
                    text: routingController.routeAvailable
                          ? qsTr("%1 • %2 km • %3 phút")
                                .arg(routePanel.compactSource)
                                .arg((routingController.distanceMeters / 1000).toFixed(2))
                                .arg(Math.ceil(routingController.durationSeconds / 60))
                          : routingController.loading ? qsTr("Đang tính tuyến…")
                                                      : qsTr("Không có tuyến")
                }
            }

            KeyboardActionButton {
                width: parent.width
                visible: routeNavigationController.active
                text: qsTr("Theo vị trí hiện tại")
                normalColor: "#334352"
                pressedColor: "#455869"
                textColor: Theme.textPrimary
                onTriggered: routePanel.recenterRequested()
            }

            KeyboardActionButton {
                id: startNavigationButton
                width: parent.width
                visible: routingController.routeAvailable
                         && !routeNavigationController.active
                text: qsTr("Bắt đầu chỉ đường")
                KeyNavigation.tab: originField
                KeyNavigation.backtab: routeButton
                normalColor: Theme.success
                pressedColor: "#4fab70"
                textColor: "#102018"
                onTriggered: routeNavigationController.start()
            }

            Row {
                width: parent.width
                visible: routingController.routeAvailable
                spacing: Theme.spaceXs
                Repeater {
                    model: [1, 2, 4]
                    delegate: KeyboardActionButton {
                        width: (routeControls.width - Theme.spaceXs * 2) / 3
                        text: modelData + "x"
                        normalColor: routeNavigationController.speedMultiplier === modelData
                                     ? Theme.accent : "#334352"
                        pressedColor: "#455869"
                        textColor: routeNavigationController.speedMultiplier === modelData
                                   ? "#10243a" : Theme.textPrimary
                        onTriggered: routeNavigationController.setSpeedMultiplier(modelData)
                    }
                }
            }

            Row {
                width: parent.width
                visible: routeNavigationController.active
                spacing: Theme.spaceSm
                KeyboardActionButton {
                    id: navigationPrimaryAction
                    objectName: "navigationPrimaryAction"
                    width: (parent.width - parent.spacing) / 2
                    text: routeNavigationController.arrived ? qsTr("Đi lại từ đầu")
                          : routeNavigationController.paused ? qsTr("Tiếp tục")
                                                             : qsTr("Tạm dừng")
                    normalColor: Theme.warning
                    pressedColor: "#d89b19"
                    textColor: "#2d2308"
                    onTriggered: {
                        if (routeNavigationController.arrived
                                || routeNavigationController.paused)
                            routeNavigationController.start()
                        else
                            routeNavigationController.pause()
                    }
                }
                KeyboardActionButton {
                    width: (parent.width - parent.spacing) / 2
                    text: qsTr("Dừng")
                    normalColor: Theme.error
                    pressedColor: "#d95f5f"
                    textColor: "#301010"
                    onTriggered: routeNavigationController.stop()
                }
            }

            KeyboardActionButton {
                width: parent.width
                visible: routeNavigationController.running || routeNavigationController.paused
                text: qsTr("Mô phỏng đi lệch (>40 m)")
                normalColor: "#334352"
                pressedColor: "#455869"
                textColor: Theme.textPrimary
                onTriggered: routeNavigationController.simulateDeviation()
            }

            Text {
                width: parent.width
                visible: routingController.routeAvailable
                text: routeNavigationController.statusMessage
                color: routeNavigationController.arrived ? Theme.success
                                                          : Theme.textSecondary
                wrapMode: Text.WordWrap
            }

            Text {
                width: parent.width
                text: routingController.statusMessage
                color: routingController.routeAvailable ? Theme.success : Theme.textSecondary
                wrapMode: Text.WordWrap
            }

            Text {
                text: qsTr("Các bước chỉ đường")
                color: Theme.textPrimary
                font.bold: true
                visible: routingController.routeAvailable
            }
        }
    }

    ListView {
        id: maneuverList
        objectName: "maneuverList"
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: routeControlsScroll.bottom
        anchors.bottom: parent.bottom
        anchors.leftMargin: Theme.spaceLg
        anchors.rightMargin: Theme.spaceLg
        anchors.topMargin: Theme.spaceSm
        anchors.bottomMargin: Theme.spaceLg
        clip: true
        spacing: Theme.spaceXs
        model: routingController.maneuvers
        currentIndex: routeNavigationController.active
                      ? routeNavigationController.currentManeuverIndex
                      : routingController.currentManeuverIndex
        activeFocusOnTab: true

        Keys.onDownPressed: {
            routingController.focusManeuver(
                        Math.min(count - 1, routingController.currentManeuverIndex + 1))
            event.accepted = true
        }
        Keys.onUpPressed: {
            routingController.focusManeuver(
                        Math.max(0, routingController.currentManeuverIndex - 1))
            event.accepted = true
        }

        delegate: ManeuverRow {
            width: maneuverList.width
            maneuverIndex: index
            maneuver: modelData
            selected: index === (routeNavigationController.active
                                 ? routeNavigationController.currentManeuverIndex
                                 : routingController.currentManeuverIndex)
            onActivated: {
                routingController.focusManeuver(maneuverIndex)
                maneuverList.forceActiveFocus()
            }
        }
    }
}
