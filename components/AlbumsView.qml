pragma ComponentBehavior: Bound
import QtQuick
import Evernight

Flickable {
    id: flick
    readonly property real avail: width - 2 * Theme.pageMargin
    signal openRequested(string kind, string name)
    signal dragLock(bool locked)
    function toUrl(s) {
        if (!s)
            return "";
        const v = String(s);
        return v.indexOf("://") >= 0 || v.indexOf("qrc:") === 0 ? v : "file://" + v;
    }
    onDraggingVerticallyChanged: dragLock(draggingVertically)
    clip: true
    contentWidth: width
    contentHeight: grid.height + Theme.spaceXl
    flickableDirection: Flickable.VerticalFlick
    boundsBehavior: Flickable.StopAtBounds
    Grid {
        id: grid
        x: Theme.pageMargin
        spacing: 0
        columns: Math.max(2, Math.floor(flick.avail / 172))
        readonly property real cellW: Math.floor(flick.avail / columns)
        readonly property real cellH: cellW + 56
        width: cellW * columns
        Repeater {
            model: Controller.albums
            Item {
                id: cell
                required property var modelData
                required property int index
                width: grid.cellW
                height: grid.cellH
                Rectangle {
                    x: 4
                    y: 4
                    width: grid.cellW - 8
                    height: grid.cellW - 8
                    color: Theme.secondaryContainer
                    clip: true
                    Text {
                        anchors.centerIn: parent
                        text: "\u266B"
                        color: Theme.onSurfaceColor
                        font.pixelSize: parent.width * 0.35
                        visible: art.status !== Image.Ready
                    }
                    Image {
                        id: art
                        anchors.fill: parent
                        source: flick.toUrl(cell.modelData.thumb)
                        fillMode: Image.PreserveAspectCrop
                        asynchronous: true
                        sourceSize.width: 400
                        sourceSize.height: 400
                    }
                }
                Column {
                    x: 16
                    y: grid.cellW + 4
                    width: parent.width - 32
                    spacing: 2
                    Text {
                        width: parent.width
                        text: cell.modelData.name
                        elide: Text.ElideRight
                        color: Theme.onSurfaceColor
                        font.pixelSize: Theme.fsBodyLarge
                        font.weight: Font.Medium
                    }
                    Text {
                        width: parent.width
                        text: cell.modelData.artist
                        elide: Text.ElideRight
                        color: Theme.onSurfaceVariantColor
                        font.pixelSize: 13
                    }
                }
                Rectangle {
                    anchors.fill: parent
                    anchors.margins: 4
                    color: Theme.onSurfaceColor
                    opacity: area.pressed ? 0.08 : 0
                }
                Rectangle {
                    anchors.fill: parent
                    anchors.margins: 4
                    color: "transparent"
                    border.width: 1
                    border.color: Theme.outlineVariant
                }
                MouseArea {
                    id: area
                    anchors.fill: parent
                    onClicked: flick.openRequested("album", cell.modelData.name)
                }
            }
        }
    }
}
