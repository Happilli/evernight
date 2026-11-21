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
    readonly property real playerInset: np.visible ? np.miniHeight + np.miniBottom + 12 : 0

    function handleBack() {
        if (np.p > 0.5) {
            np.collapse();
            return true;
        }
        if (playlistSheet.handleBack())
            return true;
        if (songSheet.open) {
            songSheet.open = false;
            return true;
        }
        if (detailLoader.page && detailLoader.page.handleBack())
            return true;
        if (detailId !== "") {
            detailId = "";
            return true;
        }
        return homePage.handleBack() || playlistsPage.handleBack();
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
            }
            PlaylistsPage {
                id: playlistsPage
                onOpenRequested: playlistId => root.detailId = playlistId
            }
            Loader {
                width: ListView.view ? ListView.view.width : 0
                height: ListView.view ? ListView.view.height : 0
                active: view.currentIndex === 2
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
            PlaylistDetail {}
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
            playlistSheet.openRename(id, name);
        }
        function onDeleteRequested(id, name) {
            playlistSheet.openDelete(id, name);
        }
        function onSongLongPressed(uri, title, artist, playlistId) {
            songSheet.openFor(uri, title, artist, playlistId);
        }
    }

    NowPlaying {
        id: np
    }

    Item {
        id: overlayLayer
        anchors.fill: parent

        PlaylistSheet {
            id: playlistSheet
            anchors.fill: parent
        }

        SongSheet {
            id: songSheet
            anchors.fill: parent
        }
    }
}
