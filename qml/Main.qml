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
    maximumWidth: 1920
    maximumHeight: 1080
    visible: true
    title: qsTr("QtMaps — Lập tuyến Hà Nội")
    property bool performanceOverlayVisible: false
    property bool panelCollapsed: false
    property bool routeOverlayForBenchmark:
        typeof routeOverlayEnabled === "undefined" ? true : routeOverlayEnabled
    property string loadedMapStyleUrl: mapSourceController.activeStyleUrl
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
    function recenterNavigation() {
        if (mapCanvasLoader.item)
            mapCanvasLoader.item.recenterNavigation()
    }
    function fitRouteWithPanelPadding() {
        if (mapCanvasLoader.item)
            mapCanvasLoader.item.fitRouteWithPanelPadding()
    }

    Shortcut {
        sequence: "F3"
        context: Qt.ApplicationShortcut
        enabled: instrumentationEnabled
        onActivated: root.performanceOverlayVisible = !root.performanceOverlayVisible
    }

    onClosing: {
        close.accepted = false
        root.visible = false
        shutdownCoordinator.requestQuit()
    }

    Rectangle {
        anchors.fill: parent
        color: "#dce7f5"
        Text {
            anchors.centerIn: parent
            color: "#243447"
            text: mapSourceController.mapStatusText
        }
    }

    Loader {
        id: mapCanvasLoader
        anchors.fill: parent
        active: root.loadedMapStyleUrl.length > 0
        sourceComponent: Component {
            MapCanvas {
                styleUrl: root.loadedMapStyleUrl
                routeOverlayEnabled: root.routeOverlayForBenchmark
                endpointSelectionEnabled: root.endpointSelectionEnabled
                panelCollapsed: root.panelCollapsed
                panelWidth: routePanel.width
            }
        }
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
        navigationActive: navigationOverlay.guidanceVisible
        navigationBannerHeight: navigationOverlay.bannerHeight
        connectionLabel: routingController.connectionLabel
        z: 20
    }

    Connections {
        target: mapSourceController
        function onStateChanged() {
            if (root.loadedMapStyleUrl === mapSourceController.activeStyleUrl)
                return
            mapCanvasLoader.active = false
            root.loadedMapStyleUrl = mapSourceController.activeStyleUrl
            mapReloadTimer.restart()
        }
    }

    Timer {
        id: mapReloadTimer
        interval: 0
        repeat: false
        onTriggered: mapCanvasLoader.active = root.loadedMapStyleUrl.length > 0
    }

    Connections {
        target: routeNavigationController
        function onStateChanged() {
            if (routeNavigationController.active)
                root.panelCollapsed = true
        }
    }

    Connections {
        target: routingController
        function onStateChanged() {
            if (routingController.loading && routingController.routeAvailable)
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
