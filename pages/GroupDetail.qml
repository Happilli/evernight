import QtQuick
import Evernight.Extraa
import Evernight

Item {
    id: root

    property string kind: ""
    property string groupName: ""
    readonly property real statusInset: 40
    property bool ready: false
    readonly property real scrollOffset: ready ? Math.max(0, dflick.contentY + dflick.topMargin - statusInset) : 0

    signal closeRequested
    signal songLongPressed(string uri, string title, string artist, string playlistId)

    readonly property var group: {
        const l = root.kind === "album" ? Controller.albums : Controller.artists;
        for (let i = 0; i < l.length; i++)
            if (l[i].name === root.groupName)
                return l[i];
        return null;
    }

    onGroupChanged: {
        if (!group && groupName !== "")
            closeRequested();
    }

    readonly property var detailRows: {
        const out = {
            model: [],
            uris: []
        };
        const g = root.group;
        if (!g)
            return out;
        const t = Controller.tracks;
        for (let k = 0; k < g.indices.length; k++) {
            const tr = t[g.indices[k]];
            if (!tr)
                continue;
            out.uris.push(tr.uri);
            out.model.push({
                headline: tr.title,
                supporting: root.kind === "album" ? tr.artist : tr.album,
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
        return false;
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

        Component.onCompleted: {
            contentY = -topMargin;
            root.ready = true;
        }

        Column {
            id: dcol
            width: dflick.width
            spacing: 12

            Row {
                id: actions
                width: parent.width
                spacing: 12
                readonly property real btnW: (width - spacing) / 2

                MaterialButton {
                    width: actions.btnW
                    label: "Play"
                    iconSource: "../icons/play.svg"
                    type: MaterialButton.Filled
                    sizeStyle: MaterialButton.Medium
                    color: Theme.primary
                    contentColor: Theme.onPrimaryColor
                    enabled: root.detailRows.uris.length > 0
                    onClicked: root.playAll(false)
                }

                MaterialButton {
                    width: actions.btnW
                    label: "Shuffle"
                    iconSource: "../icons/shuffle.svg"
                    type: MaterialButton.Tonal
                    sizeStyle: MaterialButton.Medium
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
                model: root.detailRows.model
                checkedIndices: root.shownCurrent >= 0 ? [root.shownCurrent] : []
                onClicked: (index, item) => Controller.playQueue(root.detailRows.uris, index)
                onLongPressed: (index, item) => root.songLongPressed(root.detailRows.uris[index], item.headline, item.supporting, "")
            }
        }
    }

    Rectangle {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: root.statusInset
        color: root.scrollOffset > 1 ? Theme.surfaceContainer : Theme.background
    }

    MaterialAppBar {
        id: bar
        anchors.top: parent.top
        anchors.topMargin: root.statusInset
        anchors.left: parent.left
        anchors.right: parent.right
        type: MaterialAppBar.Medium
        navigation: MaterialAppBar.Back
        title: root.group ? root.group.name : ""
        subtitle: root.detailRows.uris.length + (root.detailRows.uris.length === 1 ? " song" : " songs")
        color: Theme.background
        scrolledColor: Theme.surfaceContainer
        titleColor: Theme.onSurfaceColor
        subtitleColor: Theme.onSurfaceVariantColor
        iconColor: Theme.onSurfaceColor
        collapse: bar.collapseRange > 0 ? Math.min(1, root.scrollOffset / bar.collapseRange) : 0
        scrolled: root.scrollOffset > 1
        onNavigationClicked: root.closeRequested()
    }
}
