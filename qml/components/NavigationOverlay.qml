import QtQuick 2.13
import "../theme"

Item {
    id: overlay
    property bool panelCollapsed: false
    property real panelX: Theme.spaceMd
    property real panelWidth: 330
    readonly property real bannerHeight: navigationBanner.height

    NavigationBanner {
        id: navigationBanner
        objectName: "navigationBanner"
        visible: routeNavigationController.active
        x: overlay.panelCollapsed
           ? (overlay.width - width) / 2
           : overlay.panelX + overlay.panelWidth + Theme.spaceLg
             + Math.max(0, (overlay.width - overlay.panelX - overlay.panelWidth
                            - Theme.spaceLg - width) / 2)
        anchors.top: parent.top
        anchors.topMargin: Theme.spaceMd
        z: 30
        instruction: routeNavigationController.maneuverInstruction
        streetName: routeNavigationController.maneuverStreetName
        sourceLabel: routeNavigationController.sourceLabel
        distanceToManeuverMeters: routeNavigationController.distanceToManeuverMeters
        remainingDistanceMeters: routeNavigationController.remainingDistanceMeters
        remainingDurationSeconds: routeNavigationController.remainingDurationSeconds
        arrived: routeNavigationController.arrived
    }
}
