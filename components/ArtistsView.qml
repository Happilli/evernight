import QtQuick
import Evernight.Extraa
import Evernight

Flickable {
    id: flick
    readonly property var artistList: Controller.artists
    readonly property var artistModel: artistList.map(a => ({
                headline: a.name,
                supporting: a.count + (a.count === 1 ? " song" : " songs"),
                image: a.thumb,
                avatarText: "\u266A"
            }))
    signal openRequested(string kind, string name)
    signal dragLock(bool locked)
    onDraggingVerticallyChanged: dragLock(draggingVertically)
    clip: true
    contentWidth: width
    contentHeight: col.height + Theme.spaceXl
    flickableDirection: Flickable.VerticalFlick
    boundsBehavior: Flickable.StopAtBounds
    Column {
        id: col
        x: Theme.pageMargin
        width: flick.width - 2 * Theme.pageMargin
        spacing: 0
        MaterialList {
            width: parent.width
            variant: MaterialList.Standard
            selectionMode: MaterialList.NoSelection
            selectedColor: Theme.secondaryContainer
            textColor: Theme.onSurfaceColor
            selectedTextColor: Theme.onSurfaceColor
            supportingColor: Theme.onSurfaceVariantColor
            avatarColor: Theme.secondaryContainer
            avatarTextColor: Theme.onSurfaceColor
            dividerColor: Theme.outlineVariant
            model: flick.artistModel
            onClicked: (index, item) => flick.openRequested("artist", flick.artistList[index].name)
        }
    }
}
