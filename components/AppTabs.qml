import QtQuick
import Evernight.Extraa
import Evernight

MaterialTabs {
    labels: ["Home", "Playlists", "Library", "Settings"]
    type: MaterialTabs.Primary
    springDamping: 1.0
    activeColor: Theme.onSurfaceColor
    inactiveColor: Theme.onSurfaceVariantColor
    indicatorColor: Theme.onSurfaceColor
    dividerColor: Theme.outlineVariant
}
