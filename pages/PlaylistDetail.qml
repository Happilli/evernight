import QtQuick
import Evernight.Extraa
import Evernight

Item {
    id: root

    property string playlistId: ""
    property var shown: null
    readonly property real statusInset: 40

    signal closeRequested
    signal renameRequested(string id, string name)
    signal deleteRequested(string id, string name)
    signal songLongPressed(string uri, string title, string artist, string playlistId)

    readonly property var detailList: {
        const l = Playlists.playlists;
        for (let i = 0; i < l.length; i++)
            if (l[i].id === root.playlistId)
                return l[i];
        return null;
    }

    readonly property var detailRows: {
        const out = {
            model: [],
            uris: []
        };
        const l = root.shown;
        if (!l)
            return out;
        const t = Controller.tracks;
        const byUri = {};
        for (let i = 0; i < t.length; i++)
            byUri[t[i].uri] = t[i];
        for (let k = 0; k < l.tracks.length; k++) {
            const tr = byUri[l.tracks[k]];
            if (!tr)
                continue;
            out.uris.push(l.tracks[k]);
            out.model.push({
                headline: tr.title,
                supporting: tr.artist,
                image: tr.thumb,
                avatarText: "\u266A"
            });
        }
        return out;
    }

    readonly property int shownCurrent: {
        const c = Controller.current;
        const t = Controller.tracks;
        if (c < 0 || c >= t.length)
            return -1;
        return root.detailRows.uris.indexOf(t[c].uri);
    }

    function playAll(shuffle) {
        const u = detailRows.uris;
        if (u.length === 0)
            return;
        if (shuffle)
            Controller.shuffle = true;
        Controller.playQueue(u, shuffle ? Math.floor(Math.random() * u.length) : 0);
    }

    function handleBack() {
        if (moreMenu.open) {
            moreMenu.close();
            return true;
        }
        return false;
    }

    onDetailListChanged: {
        if (detailList)
            shown = detailList;
        else if (playlistId !== "")
            closeRequested();
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.background
    }

    MouseArea {
        anchors.fill: parent
    }

    Flickable {
        id: dflick
        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        topMargin: root.statusInset + bar.expandedHeight
        clip: true
        contentWidth: width
        contentHeight: dcol.height + 40
        flickableDirection: Flickable.VerticalFlick
        boundsBehavior: Flickable.StopAtBounds
        Component.onCompleted: contentY = -topMargin

        Column {
            id: dcol
            width: dflick.width
            spacing: 12

            Row {
                spacing: 12

                MaterialButton {
                    label: "Play"
                    type: MaterialButton.Filled
                    color: Theme.primary
                    contentColor: Theme.onPrimaryColor
                    enabled: root.detailRows.uris.length > 0
                    onClicked: root.playAll(false)
                }

                MaterialButton {
                    label: "Shuffle"
                    type: MaterialButton.Tonal
                    color: Theme.secondaryContainer
                    contentColor: Theme.onSurfaceColor
                    enabled: root.detailRows.uris.length > 0
                    onClicked: root.playAll(true)
                }
            }

            MaterialList {
                width: parent.width
                visible: root.detailRows.uris.length > 0
                variant: MaterialList.Standard
                selectedColor: Theme.secondaryContainer
                textColor: Theme.onSurfaceColor
                selectedTextColor: Theme.onSurfaceColor
                supportingColor: Theme.onSurfaceVariantColor
                avatarColor: Theme.secondaryContainer
                avatarTextColor: Theme.onSurfaceColor
                dividerColor: Theme.outlineVariant
                selectionMode: MaterialList.Single
                equalizerIndex: root.shownCurrent
                equalizerLevels: Controller.spectrum.levels
                model: root.detailRows.model
                checkedIndices: root.shownCurrent >= 0 ? [root.shownCurrent] : []
                onClicked: (index, item) => Controller.playQueue(root.detailRows.uris, index)
                onLongPressed: (index, item) => root.songLongPressed(root.detailRows.uris[index], item.headline, item.supporting, root.playlistId)
            }

            Item {
                width: parent.width
                height: 160
                visible: root.detailRows.uris.length === 0

                Column {
                    anchors.centerIn: parent
                    spacing: 6

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: "No songs yet"
                        color: Theme.onSurfaceColor
                        font.pixelSize: 18
                    }

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: "Long-press a song to add it here"
                        color: Theme.onSurfaceVariantColor
                        font.pixelSize: 14
                    }
                }
            }
        }
    }

    Rectangle {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: root.statusInset
        color: bar.scrolled ? Theme.surfaceContainer : Theme.background
    }

    MaterialAppBar {
        id: bar
        anchors.top: parent.top
        anchors.topMargin: root.statusInset
        anchors.left: parent.left
        anchors.right: parent.right
        type: MaterialAppBar.Medium
        navigation: MaterialAppBar.Back
        title: root.shown ? root.shown.name : ""
        subtitle: root.detailRows.uris.length + (root.detailRows.uris.length === 1 ? " song" : " songs")
        actions: [
            {
                kind: "more"
            }
        ]
        color: Theme.background
        scrolledColor: Theme.surfaceContainer
        titleColor: Theme.onSurfaceColor
        subtitleColor: Theme.onSurfaceVariantColor
        iconColor: Theme.onSurfaceColor
        collapse: bar.collapseRange > 0 ? Math.max(0, Math.min(1, (dflick.contentY + dflick.topMargin - root.statusInset) / bar.collapseRange)) : 0
        scrolled: dflick.contentY + dflick.topMargin - root.statusInset > 1
        onNavigationClicked: root.closeRequested()
        onActionClicked: (index, item, anchor) => moreMenu.popup(anchor.x, anchor.y + root.statusInset)
    }

    MouseArea {
        anchors.fill: parent
        enabled: moreMenu.open
        onClicked: moreMenu.close()
    }

    MaterialMenu {
        id: moreMenu
        model: [
            {
                label: "Rename"
            },
            {
                label: "Delete",
                danger: true
            }
        ]
        color: Theme.surfaceContainerHigh
        textColor: Theme.onSurfaceColor
        shortcutColor: Theme.onSurfaceVariantColor
        separatorColor: Theme.outlineVariant
        dangerColor: Theme.error
        onTriggered: (index, item) => {
            const name = root.shown ? root.shown.name : "";
            if (index === 0)
                root.renameRequested(root.playlistId, name);
            else
                root.deleteRequested(root.playlistId, name);
        }
    }
}
