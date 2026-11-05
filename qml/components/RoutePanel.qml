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
    property string originError: ""
    property string destinationError: ""
    property bool maneuversCollapsed: false
    readonly property real maneuverListMinimumHeight: 120
    signal recenterRequested()

    function applyEndpoints() {
        var requestedOrigin = originField.text
        var requestedDestination = destinationField.text
        var originValid = routingController.setOriginInput(requestedOrigin)
        var destinationValid = routingController.setDestinationInput(requestedDestination)
        originError = originValid ? "" : qsTr("Điểm đi không hợp lệ. Dùng tên mẫu hoặc vĩ độ, kinh độ.")
        destinationError = destinationValid ? "" : qsTr("Điểm đến không hợp lệ. Dùng tên mẫu hoặc vĩ độ, kinh độ.")
        return originValid && destinationValid
    }

    function calculateRequestedRoute() {
        if (applyEndpoints()) {
            if (routingFixtureOnly)
                routingController.calculateFixtureRoute()
            else
                routingController.calculateRoute()
        }
    }

    function swapEndpoints() {
        var oldOrigin = originField.text
        originField.text = destinationField.text
        destinationField.text = oldOrigin
        originError = ""
        destinationError = ""
    }

    function clearEndpointFields() {
        originField.text = ""
        destinationField.text = ""
        originError = qsTr("Nhập điểm đi để tìm đường.")
        destinationError = qsTr("Nhập điểm đến để tìm đường.")
        originField.forceActiveFocus()
    }

    function formatRouteDistance(meters) {
        return (meters / 1000).toFixed(1).replace(".", ",")
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
                font.pixelSize: Theme.fontSizeBody
                wrapMode: Text.WordWrap
            }

            Rectangle {
                width: parent.width
                height: 42
                radius: Theme.radiusControl
                color: Theme.surfaceInput
                border.color: originField.activeFocus ? Theme.accent : Theme.border
                Rectangle {
                    id: originDot
                    anchors.left: parent.left
                    anchors.leftMargin: Theme.spaceSm
                    anchors.verticalCenter: parent.verticalCenter
                    width: 24; height: 24; radius: 12
                    color: Theme.markerOrigin
                    border.color: Theme.routeOutline
                    border.width: 2
                    Text {
                        anchors.centerIn: parent
                        color: "#102018"
                        font.bold: true
                        font.pixelSize: Theme.fontSizeBody
                        text: "A"
                    }
                }
                TextInput {
                    id: originField
                    objectName: "originField"
                    anchors.left: originDot.right
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.leftMargin: Theme.spaceSm
                    anchors.rightMargin: Theme.spaceSm
                    color: Theme.textPrimary
                    verticalAlignment: TextInput.AlignVCenter
                    selectByMouse: true
                    activeFocusOnTab: true
                    KeyNavigation.tab: destinationField
                    KeyNavigation.backtab: routeButton
                    text: routingController.originInput
                    onAccepted: routePanel.calculateRequestedRoute()
                }
            }

            Text {
                width: parent.width
                visible: routePanel.originError.length > 0
                color: Theme.error
                wrapMode: Text.WordWrap
                font.pixelSize: Theme.fontSizeCaption
                text: routePanel.originError
            }

            Rectangle {
                width: parent.width
                height: 42
                radius: Theme.radiusControl
                color: Theme.surfaceInput
                border.color: destinationField.activeFocus ? Theme.accent : Theme.border
                Rectangle {
                    id: destinationDot
                    anchors.left: parent.left
                    anchors.leftMargin: Theme.spaceSm
                    anchors.verticalCenter: parent.verticalCenter
                    width: 24; height: 24; radius: 12
                    color: Theme.markerDestination
                    border.color: Theme.routeOutline
                    border.width: 2
                    Text {
                        anchors.centerIn: parent
                        color: "#301010"
                        font.bold: true
                        font.pixelSize: Theme.fontSizeBody
                        text: "B"
                    }
                }
                TextInput {
                    id: destinationField
                    objectName: "destinationField"
                    anchors.left: destinationDot.right
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.leftMargin: Theme.spaceSm
                    anchors.rightMargin: Theme.spaceSm
                    color: Theme.textPrimary
                    verticalAlignment: TextInput.AlignVCenter
                    selectByMouse: true
                    activeFocusOnTab: true
                    KeyNavigation.tab: swapButton
                    KeyNavigation.backtab: originField
                    text: routingController.destinationInput
                    onAccepted: routePanel.calculateRequestedRoute()
                }
            }

            Text {
                width: parent.width
                visible: routePanel.destinationError.length > 0
                color: Theme.error
                wrapMode: Text.WordWrap
                font.pixelSize: Theme.fontSizeCaption
                text: routePanel.destinationError
            }

            Row {
                width: parent.width
                spacing: Theme.spaceSm
                KeyboardActionButton {
                    id: swapButton
                    objectName: "swapButton"
                    width: (parent.width - parent.spacing) / 2
                    text: qsTr("⇅  Đổi chiều")
                    normalColor: Theme.surfaceInput
                    pressedColor: Theme.surfacePressed
                    textColor: Theme.textPrimary
                    KeyNavigation.tab: clearButton
                    KeyNavigation.backtab: destinationField
                    onTriggered: routePanel.swapEndpoints()
                }
                KeyboardActionButton {
                    id: clearButton
                    objectName: "clearButton"
                    width: (parent.width - parent.spacing) / 2
                    text: qsTr("×  Xóa")
                    normalColor: Theme.surfaceInput
                    pressedColor: Theme.surfacePressed
                    textColor: Theme.textPrimary
                    KeyNavigation.tab: routeButton
                    KeyNavigation.backtab: swapButton
                    onTriggered: routePanel.clearEndpointFields()
                }
            }


            KeyboardActionButton {
                id: routeButton
                width: parent.width
                enabled: !routingController.loading
                text: routingController.loading ? qsTr("Đang tìm đường…") : qsTr("Tìm đường")
                KeyNavigation.tab: startNavigationButton
                KeyNavigation.backtab: clearButton
                onTriggered: routePanel.calculateRequestedRoute()
            }

            Rectangle {
                width: parent.width
                height: routingController.routeAvailable ? 68 : 44
                radius: Theme.radiusControl
                color: Theme.surfaceInput
                border.color: Theme.border
                Text {
                    anchors.fill: parent
                    anchors.margins: Theme.spaceSm
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                    color: routePanel.routeSummaryColor
                    font.bold: true
                    font.pixelSize: routingController.routeAvailable
                                    ? Theme.fontSizeDisplay : Theme.fontSizeBody
                    text: routingController.routeAvailable
                          ? qsTr("%1 km · %2 phút")
                                .arg(routePanel.formatRouteDistance(routingController.distanceMeters))
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
                objectName: "startNavigationButton"
                width: parent.width
                visible: routingController.routeAvailable
                         && !routeNavigationController.active
                         && !routingController.loading
                enabled: visible
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
                Text {
                    width: parent.width
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeCaption
                    text: qsTr("Tốc độ mô phỏng")
                }
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
                                     ? Theme.accent : Theme.surfaceInput
                        pressedColor: Theme.surfacePressed
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
                text: routeNavigationController.active
                      ? routeNavigationController.statusMessage
                      : routingController.statusMessage + qsTr(" · Nguồn: ")
                        + routingController.sourceLabel
                color: routeNavigationController.arrived || routingController.routeAvailable
                       ? Theme.success : Theme.textSecondary
                font.pixelSize: Theme.fontSizeBody
                wrapMode: Text.WordWrap
            }

            KeyboardActionButton {
                width: parent.width
                visible: routingController.routeAvailable
                text: routePanel.maneuversCollapsed
                      ? qsTr("Hiện các bước chỉ đường")
                      : qsTr("Thu gọn các bước chỉ đường")
                normalColor: Theme.surfaceInput
                pressedColor: Theme.surfacePressed
                textColor: Theme.textPrimary
                onTriggered: routePanel.maneuversCollapsed = !routePanel.maneuversCollapsed
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
        visible: !routePanel.maneuversCollapsed
        spacing: Theme.spaceXs
        model: routingController.maneuvers
        currentIndex: routeNavigationController.active
                      ? routeNavigationController.currentManeuverIndex
                      : routingController.currentManeuverIndex
        activeFocusOnTab: true
        onCurrentIndexChanged: {
            if (currentIndex >= 0)
                positionViewAtIndex(currentIndex, ListView.Contain)
        }

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
