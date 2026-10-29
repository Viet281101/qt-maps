import QtQuick 2.13
import QtQuick.Window 2.13
import "components"
import "theme"

Window {
    id: root
    width: 1120
    height: 700
    minimumWidth: 900
    minimumHeight: 580
    visible: true
    title: qsTr("QtMaps — Lập tuyến Hà Nội")
    property bool performanceOverlayVisible: false
    property bool panelCollapsed: false
    readonly property bool endpointSelectionEnabled:
        !routeNavigationController.active
        && !(routingController.loading && routeNavigationController.routeAvailable)
    readonly property string compactSource: routingController.sourceLabel === "Valhalla local"
                                            ? qsTr("Valhalla") : qsTr("Fixture")
    readonly property color routeSummaryColor: routingController.loading ? Theme.warning
                                               : !routingController.routeAvailable ? Theme.error
                                               : routingController.sourceLabel === "Valhalla local"
                                                 && routingController.connectionLabel.indexOf("loopback") >= 0
                                                 ? Theme.success : Theme.warning

    function applyEndpoints() { routePanel.applyEndpoints() }
    function calculateRequestedRoute() { routePanel.calculateRequestedRoute() }
    function recenterNavigation() { mapCanvas.recenterNavigation() }
    function fitRouteWithPanelPadding() { mapCanvas.fitRouteWithPanelPadding() }

    onClosing: {
        close.accepted = false
        root.visible = false
        shutdownCoordinator.requestQuit()
    }

    MapCanvas {
        id: mapCanvas
        anchors.fill: parent
        endpointSelectionEnabled: root.endpointSelectionEnabled
        panelCollapsed: root.panelCollapsed
        panelWidth: routePanel.width
        onTogglePerformanceOverlayRequested:
            root.performanceOverlayVisible = !root.performanceOverlayVisible
    }

    RoutePanel {
        id: routePanel
        objectName: "routePanel"
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.margins: Theme.spaceMd
        visible: !root.panelCollapsed
        routeSummaryColor: root.routeSummaryColor
        compactSource: root.compactSource
        z: 20
        onRecenterRequested: root.recenterNavigation()
    }

    NavigationOverlay {
        id: navigationOverlay
        anchors.fill: parent
        panelCollapsed: root.panelCollapsed
        panelX: routePanel.x
        panelWidth: routePanel.width
        z: 30
    }

    DemoToolsOverlay {
        anchors.fill: parent
        navigationActive: navigationOverlay.guidanceVisible
        panelCollapsed: root.panelCollapsed
        panelWidth: routePanel.x + routePanel.width
        z: 22
    }

    KeyboardActionButton {
        id: panelToggle
        objectName: "panelToggle"
        x: root.panelCollapsed ? Theme.spaceMd
                               : routePanel.x + routePanel.width + Theme.spaceSm
        y: Theme.spaceMd
        width: 38
        z: 34
        text: root.panelCollapsed ? "›" : "‹"
        normalColor: "#dd1b2530"
        pressedColor: "#334352"
        textColor: Theme.textPrimary
        onTriggered: {
            root.panelCollapsed = !root.panelCollapsed
            Qt.callLater(root.fitRouteWithPanelPadding)
        }
    }

    ConnectionStatus {
        anchors.fill: parent
        statusColor: root.routeSummaryColor
        navigationActive: routeNavigationController.active
        navigationBannerHeight: navigationOverlay.bannerHeight
        z: 20
    }

    Connections {
        target: routeNavigationController
        function onStateChanged() {
            if (routeNavigationController.active)
                root.panelCollapsed = true
        }
    }

    PerformanceOverlay {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 50
        anchors.rightMargin: Theme.spaceMd
        monitor: performanceMonitor
        detailed: root.performanceOverlayVisible
        visible: instrumentationEnabled
        z: 20
    }

    Component.onCompleted: {
        if (routingFixtureOnly && !routingController.routeAvailable
                && !routingController.loading)
            routingController.calculateFixtureRoute()
    }
}
