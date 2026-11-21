import QtQuick
import Evernight.Extraa
import Evernight

FadePage {
    id: page

    property var matches: []
    property var recent: []
    property bool fromCarousel: false
    property var infoCache: ({})

    signal songLongPressed(string uri, string title, string artist, string playlistId)

    function refresh() {
        matches = Controller.search("");
    }
    function refreshRecent() {
        recent = Controller.recent;
    }

    Component.onCompleted: {
        refresh();
        refreshRecent();
    }

    Connections {
        target: Controller
        function onTracksChanged() {
            page.refresh();
            if (!page.fromCarousel)
                page.refreshRecent();
        }
        function onRecentChanged() {
            if (!page.fromCarousel)
                page.refreshRecent();
        }
        function onInspected(uri, rows) {
            if (rows.length > 0)
                page.infoCache[uri] = rows;
            const t = Controller.tracks;
            for (let k = 0; k < page.matches.length; k++) {
                if (t[page.matches[k]].uri === uri) {
                    songList.setDetails(k, rows);
                    break;
                }
            }
        }
    }

    readonly property int shownCurrent: Controller.current >= 0 ? page.matches.indexOf(Controller.current) : -1

    function handleBack() {
        if (songList.expandedIndex >= 0) {
            songList.collapseAll();
            return true;
        }
        return false;
    }

    Item {
        id: pinned
        anchors.top: parent.top
        anchors.topMargin: 12
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        height: visible ? 190 : 0
        visible: page.recent.length >= 3

        MaterialCarousel {
            id: carousel
            property bool swiped: false
            anchors.fill: parent
            type: MaterialCarousel.Hero
            snap: true
            minSmallItemSize: 28
            maxSmallItemSize: 40
            itemSpacing: 8
            itemRadius: 24
            itemColor: Theme.secondaryContainer
            textColor: Theme.onSurfaceColor
            imageTextColor: Theme.onSurfaceColor
            scrimColor: Qt.alpha(Theme.scrim, 0.6)
            model: {
                const t = Controller.tracks;
                return page.recent.map(function (i) {
                    return {
                        headline: t[i].title,
                        supporting: t[i].artist,
                        image: t[i].thumb
                    };
                });
            }
            onDraggingChanged: {
                if (dragging)
                    swiped = true;
            }
            onCurrentIndexChanged: if (swiped)
                settle.restart()
            onClicked: (index, item) => {
                settle.stop();
                swiped = false;
                const i = page.recent[index];
                if (i === undefined || i === Controller.current)
                    return;
                page.fromCarousel = true;
                Controller.playAt(i);
                page.fromCarousel = false;
            }

            Timer {
                id: settle
                interval: 350
                onTriggered: {
                    carousel.swiped = false;
                    const i = page.recent[carousel.currentIndex];
                    if (i === undefined || i === Controller.current)
                        return;
                    page.fromCarousel = true;
                    Controller.playAt(i);
                    page.fromCarousel = false;
                }
            }
        }
    }

    Flickable {
        id: flick
        flickableDirection: Flickable.VerticalFlick
        onDraggingVerticallyChanged: {
            page.ListView.view.interactive = !draggingVertically;
        }
        anchors.top: pinned.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 8
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        clip: true
        contentWidth: width
        contentHeight: col.height + 16
        boundsBehavior: Flickable.StopAtBounds

        Column {
            id: col
            width: flick.width
            spacing: 0

            MaterialList {
                id: songList
                width: parent.width
                visible: page.matches.length > 0
                variant: MaterialList.Standard
                expandable: true
                accordion: true
                springDamping: 0.8
                selectedColor: Theme.secondaryContainer
                textColor: Theme.onSurfaceColor
                selectedTextColor: Theme.onSurfaceColor
                supportingColor: Theme.onSurfaceVariantColor
                iconColor: Theme.onSurfaceVariantColor
                avatarColor: Theme.secondaryContainer
                avatarTextColor: Theme.onSurfaceColor
                dividerColor: Theme.outlineVariant
                selectionMode: MaterialList.Single
                equalizerIndex: page.shownCurrent
                equalizerLevels: Controller.spectrum.levels
                model: {
                    const t = Controller.tracks;
                    return page.matches.map(function (i) {
                        return {
                            headline: t[i].title,
                            supporting: t[i].artist,
                            image: t[i].thumb,
                            avatarText: "\u266A",
                            trailingIcon: "\u22EE",
                            expandedIcon: "\u00D7",
                            details: page.infoCache[t[i].uri] || []
                        };
                    });
                }
                checkedIndices: page.shownCurrent >= 0 ? [page.shownCurrent] : []
                onClicked: (index, item) => Controller.playAt(page.matches[index])
                onLongPressed: (index, item) => {
                    const t = Controller.tracks[page.matches[index]];
                    page.songLongPressed(t.uri, t.title, t.artist, "");
                }
                onExpandRequested: (index, item) => {
                    Controller.inspect(Controller.tracks[page.matches[index]].uri);
                }
            }

            Item {
                width: parent.width
                height: 200
                visible: page.matches.length === 0

                Column {
                    anchors.centerIn: parent
                    spacing: 6

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: "No songs found"
                        color: Theme.onSurfaceColor
                        font.pixelSize: 18
                    }
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: "Put music in your Music folder"
                        color: Theme.onSurfaceVariantColor
                        font.pixelSize: 14
                    }
                }
            }
        }
    }
}
