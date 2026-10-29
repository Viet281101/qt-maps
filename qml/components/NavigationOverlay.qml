import QtQuick 2.13
import "../theme"

Item {
    id: overlay
    property bool panelCollapsed: false
    property real panelX: Theme.spaceMd
    property real panelWidth: 330
    readonly property bool guidanceVisible: routeNavigationController.active
                                            || (routingController.loading
                                                && routeNavigationController.routeAvailable)
    readonly property real bannerHeight: navigationBanner.height

    function nextInstruction() {
        var nextIndex = routeNavigationController.currentManeuverIndex + 1
        var maneuvers = routingController.maneuvers
        return nextIndex >= 0 && nextIndex < maneuvers.length
                ? maneuvers[nextIndex].instruction : ""
    }

    NavigationBanner {
        id: navigationBanner
        objectName: "navigationBanner"
        visible: overlay.guidanceVisible
        x: overlay.panelCollapsed
           ? (overlay.width - width) / 2
           : overlay.panelX + overlay.panelWidth + Theme.spaceLg
             + Math.max(0, (overlay.width - overlay.panelX - overlay.panelWidth
                            - Theme.spaceLg - width) / 2)
        anchors.top: parent.top
        anchors.topMargin: Theme.spaceMd
        z: 30
        instruction: routeNavigationController.maneuverInstruction
        nextInstruction: overlay.nextInstruction()
        streetName: routeNavigationController.maneuverStreetName
        sourceLabel: routeNavigationController.sourceLabel
        distanceToManeuverMeters: routeNavigationController.distanceToManeuverMeters
        remainingDistanceMeters: routeNavigationController.remainingDistanceMeters
        remainingDurationSeconds: routeNavigationController.remainingDurationSeconds
        rerouting: routingController.loading && routeNavigationController.routeAvailable
        arrived: routeNavigationController.arrived
        onEndRequested: routeNavigationController.stop()
    }
}
