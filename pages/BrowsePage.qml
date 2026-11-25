pragma ComponentBehavior: Bound
import QtQuick
import Evernight.Extraa
import Evernight

FadePage {
    id: page
    signal openRequested(string kind, string name)
    signal songLongPressed(string uri, string title, string artist, string playlistId)
    readonly property bool isCurrent: ListView.isCurrentItem
    property int mode: {
        const m = Number(Store.get("browseMode", 0));
        return m >= 0 && m <= 2 ? m : 0;
    }
    readonly property var titles: ["Albums", "Artists", "Songs"]
    onIsCurrentChanged: {
        if (!isCurrent)
            filterMenu.close();
    }
    function handleBack() {
        if (filterMenu.open) {
            filterMenu.close();
            return true;
        }
        if (body.songs && body.songs.handleBack())
            return true;
        return false;
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
            text: page.titles[page.mode]
            color: Theme.onSurfaceColor
            font.pixelSize: Theme.fsTitle
            font.weight: Font.Medium
        }
        MaterialIconButton {
            id: filterBtn
            anchors.right: parent.right
            anchors.rightMargin: Theme.spaceMd
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
    Loader {
        id: body
        readonly property SongsView songs: item as SongsView
        anchors.top: header.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: Theme.spaceXs
        active: Controller.tracks.length > 0
        sourceComponent: page.mode === 0 ? albumsView : page.mode === 1 ? artistsView : songsView
    }
    Component {
        id: albumsView
        AlbumsView {}
    }
    Component {
        id: artistsView
        ArtistsView {}
    }
    Component {
        id: songsView
        SongsView {}
    }
    Connections {
        target: body.item
        ignoreUnknownSignals: true
        function onOpenRequested(kind, name) {
            page.openRequested(kind, name);
        }
        function onSongLongPressed(uri, title, artist, playlistId) {
            page.songLongPressed(uri, title, artist, playlistId);
        }
        function onDragLock(locked) {
            if (page.ListView.view)
                page.ListView.view.interactive = !locked;
        }
    }
    Item {
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 160
        visible: Controller.tracks.length === 0
        Text {
            anchors.centerIn: parent
            text: "No music found"
            color: Theme.onSurfaceVariantColor
            font.pixelSize: Theme.fsBodyLarge
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
            },
            {
                label: "Songs"
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
