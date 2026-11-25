pragma ComponentBehavior: Bound
import Evernight.Extraa
import QtQuick
import QtQuick.Window
import QtQml.Models
import Evernight

Window {
    id: root
    visible: true
    title: "Evernight"
    color: Theme.background

    property string detailId: ""
    property string groupKind: ""
    property string groupName: ""
    readonly property real playerInset: np.visible ? np.miniHeight + np.miniBottom + 12 : 0

    function handleBack() {
        if (queueSheet.open) {
            queueSheet.open = false;
            return true;
        }
        if (np.p > 0.5) {
            np.collapse();
            return true;
        }
        if (playlistDialog.handleBack())
            return true;
        if (songSheet.open) {
            songSheet.open = false;
            return true;
        }
        if (detailLoader.page && detailLoader.page.handleBack())
            return true;
        if (groupKind !== "") {
            groupKind = "";
            groupName = "";
            return true;
        }
        if (detailId !== "") {
            detailId = "";
            return true;
        }
        return homePage.handleBack() || playlistsPage.handleBack() || browsePage.handleBack();
    }

    onClosing: close => {
        if (handleBack())
            close.accepted = false;
    }

    Binding {
        target: view
        property: "interactive"
        value: false
        when: np.p > 0.02
    }

    AppTabs {
        id: tabs
        anchors.top: parent.top
        anchors.topMargin: 40
        anchors.left: parent.left
        anchors.right: parent.right
        onCurrentIndexChanged: {
            Keyboard.hide();
            if (view.currentIndex !== currentIndex)
                view.currentIndex = currentIndex;
        }
    }

    ListView {
        id: view
        anchors.top: tabs.bottom
        anchors.bottom: parent.bottom
        anchors.bottomMargin: root.playerInset
        anchors.left: parent.left
        anchors.right: parent.right
        clip: true
        orientation: ListView.Horizontal
        flickableDirection: Flickable.HorizontalFlick
        snapMode: ListView.SnapOneItem
        highlightRangeMode: ListView.StrictlyEnforceRange
        highlightMoveDuration: 450
        highlightMoveVelocity: -1
        boundsBehavior: Flickable.DragAndOvershootBounds
        flickDeceleration: 1500
        maximumFlickVelocity: 2500
        cacheBuffer: width * 3

        model: ObjectModel {
            HomePage {
                id: homePage
                onSongLongPressed: (uri, title, artist, playlistId) => songSheet.openFor(uri, title, artist, playlistId)
            }

            PlaylistsPage {
                id: playlistsPage
                onOpenRequested: playlistId => root.detailId = playlistId
                onCreateRequested: playlistDialog.openCreate()
            }

            BrowsePage {
                id: browsePage
                onOpenRequested: (kind, name) => {
                    root.groupKind = kind;
                    root.groupName = name;
                }
            }

            Loader {
                width: ListView.view ? ListView.view.width : 0
                height: ListView.view ? ListView.view.height : 0
                active: view.currentIndex === 3
                sourceComponent: SettingsPage {}
            }
        }

        onCurrentIndexChanged: {
            if (tabs.currentIndex !== currentIndex)
                tabs.currentIndex = currentIndex;
        }
    }

    Loader {
        id: detailLoader
        readonly property PlaylistDetail page: item as PlaylistDetail
        anchors.fill: parent
        anchors.bottomMargin: root.playerInset
        active: root.detailId !== ""
        sourceComponent: Component {
            PlaylistDetail {
                playlistId: root.detailId
            }
        }
    }

    Binding {
        target: detailLoader.item
        property: "playlistId"
        value: root.detailId
        when: detailLoader.item !== null
    }

    Connections {
        target: detailLoader.item
        ignoreUnknownSignals: true

        function onCloseRequested() {
            root.detailId = "";
        }

        function onRenameRequested(id, name) {
            playlistDialog.openRename(id, name);
        }

        function onDeleteRequested(id, name) {
            playlistDialog.openDelete(id, name);
        }

        function onSongLongPressed(uri, title, artist, playlistId) {
            songSheet.openFor(uri, title, artist, playlistId);
        }
    }

    Loader {
        id: groupLoader
        anchors.fill: parent
        anchors.bottomMargin: root.playerInset
        active: root.groupKind !== ""
        sourceComponent: Component {
            GroupDetail {
                kind: root.groupKind
                groupName: root.groupName
            }
        }
    }

    Connections {
        target: groupLoader.item
        ignoreUnknownSignals: true

        function onCloseRequested() {
            root.groupKind = "";
            root.groupName = "";
        }

        function onSongLongPressed(uri, title, artist, playlistId) {
            songSheet.openFor(uri, title, artist, playlistId);
        }
    }

    NowPlaying {
        id: np
        blocked: queueSheet.open
        onQueueRequested: {
            queueSheet.open = true;
        }
    }

    Item {
        id: overlayLayer
        anchors.fill: parent

        SongSheet {
            id: songSheet
            anchors.fill: parent
        }

        QueueSheet {
            id: queueSheet
            anchors.fill: parent
        }

        PlaylistDialog {
            id: playlistDialog
            anchors.fill: parent
        }

        MaterialSnackbar {
            id: snack
            anchors.fill: parent
            property string kind: ""
            property string listId: ""
            property string songUri: ""
            color: Theme.inverseSurface
            bottomMargin: 24
            textColor: Theme.inverseOnSurface
            actionColor: Theme.inversePrimary
            onActionClicked: {
                if (kind === "added")
                    Playlists.remove(listId, songUri);
                else if (kind === "removed")
                    Playlists.add(listId, songUri);
            }
        }

        Connections {
            target: Snack
            function onRequested(message, kind, listId, songUri) {
                snack.kind = kind;
                snack.listId = listId;
                snack.songUri = songUri;
                snack.show(message, kind !== "" ? "Undo" : "");
            }
        }
    }
}
