pragma ComponentBehavior: Bound
import QtQuick
import Evernight.Extraa
import Evernight

FadePage {
    id: page

    signal openRequested(string kind, string name)

    property int mode: Number(Store.get("browseMode", 0))
    readonly property var albumList: Controller.albums
    readonly property var artistList: Controller.artists
    readonly property var artistModel: artistList.map(a => ({
                headline: a.name,
                supporting: a.count + (a.count === 1 ? " song" : " songs"),
                image: a.thumb,
                avatarText: "\u266A"
            }))

    function handleBack() {
        if (filterMenu.open) {
            filterMenu.close();
            return true;
        }
        return false;
    }

    function toUrl(s) {
        if (!s)
            return "";
        const v = String(s);
        return v.indexOf("://") >= 0 || v.indexOf("qrc:") === 0 ? v : "file://" + v;
    }

    Item {
        id: header
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 56

        Text {
            anchors.left: parent.left
            anchors.leftMargin: 20
            anchors.verticalCenter: parent.verticalCenter
            text: page.mode === 0 ? "Albums" : "Artists"
            color: Theme.onSurfaceColor
            font.pixelSize: 20
            font.weight: Font.Medium
        }

        MaterialIconButton {
            id: filterBtn
            anchors.right: parent.right
            anchors.rightMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            type: MaterialIconButton.Standard
            sizeStyle: MaterialIconButton.Small
            iconSize: 24
            iconSource: filterMenu.open ? "../icons/filter_filled.svg" : "../icons/filter.svg"
            color: "transparent"
            iconColor: Theme.onSurfaceColor
            onClicked: filterMenu.popup(filterBtn.x, filterBtn.y + filterBtn.height)
        }
    }

    Flickable {
        id: flick
        anchors.top: header.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 4
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        clip: true
        contentWidth: width
        contentHeight: col.height + 24
        flickableDirection: Flickable.VerticalFlick
        boundsBehavior: Flickable.StopAtBounds

        onDraggingVerticallyChanged: {
            if (page.ListView.view)
                page.ListView.view.interactive = !draggingVertically;
        }

        Column {
            id: col
            width: flick.width
            spacing: 0

            Grid {
                id: grid
                visible: page.mode === 0
                spacing: 0
                columns: Math.max(2, Math.floor(flick.width / 172))
                readonly property real cellW: Math.floor(flick.width / columns)
                readonly property real cellH: cellW + 56
                width: cellW * columns

                Repeater {
                    model: page.mode === 0 ? page.albumList : []

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
                                source: page.toUrl(cell.modelData.thumb)
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
                                font.pixelSize: 16
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
                            onClicked: page.openRequested("album", cell.modelData.name)
                        }
                    }
                }
            }

            MaterialList {
                width: parent.width
                visible: page.mode === 1
                variant: MaterialList.Standard
                selectionMode: MaterialList.NoSelection
                selectedColor: Theme.secondaryContainer
                textColor: Theme.onSurfaceColor
                selectedTextColor: Theme.onSurfaceColor
                supportingColor: Theme.onSurfaceVariantColor
                avatarColor: Theme.secondaryContainer
                avatarTextColor: Theme.onSurfaceColor
                dividerColor: Theme.outlineVariant
                model: page.mode === 1 ? page.artistModel : []
                onClicked: (index, item) => page.openRequested("artist", page.artistList[index].name)
            }

            Item {
                width: parent.width
                height: 160
                visible: Controller.tracks.length === 0

                Text {
                    anchors.centerIn: parent
                    text: "No music found"
                    color: Theme.onSurfaceVariantColor
                    font.pixelSize: 16
                }
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        enabled: filterMenu.open
        onClicked: filterMenu.close()
    }

    MaterialMenu {
        id: filterMenu
        model: [
            {
                label: "Albums"
            },
            {
                label: "Artists"
            }
        ]
        color: Theme.surfaceContainerHigh
        textColor: Theme.onSurfaceColor
        shortcutColor: Theme.onSurfaceVariantColor
        separatorColor: Theme.outlineVariant
        dangerColor: Theme.error
        onTriggered: (index, item) => {
            page.mode = index;
            Store.set("browseMode", index);
        }
    }
}
