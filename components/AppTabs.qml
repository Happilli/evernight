import QtQuick
import Evernight.Extraa
import Evernight

MaterialTabs {
    labels: ["Songs", "Playlists", "Settings"]
    type: MaterialTabs.Primary
    springDamping: 1.0
    activeColor: Theme.onSurfaceColor
    inactiveColor: Theme.onSurfaceVariantColor
    indicatorColor: Theme.onSurfaceColor
    dividerColor: Theme.outlineVariant
}
