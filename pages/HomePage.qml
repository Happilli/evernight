pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Effects
import Evernight.Extraa
import Evernight

FadePage {
    id: page

    property var recent: []
    property bool fromCarousel: false
    property var picks: []

    signal openPlaylist(string playlistId)
    signal songLongPressed(string uri, string title, string artist, string playlistId)

    readonly property var byUri: {
        const t = Controller.tracks;
        const m = {};
        for (let i = 0; i < t.length; i++)
            m[t[i].uri] = t[i];
        return m;
    }

    readonly property var liked: {
        const l = Playlists.playlists;
        for (let i = 0; i < l.length; i++)
            if (l[i].id === "liked")
                return l[i];
        return null;
    }

    readonly property var pickRows: {
        const t = Controller.tracks;
        const out = {
            model: [],
            uris: []
        };
        for (let k = 0; k < picks.length; k++) {
            const tr = t[picks[k]];
            if (!tr)
                continue;
            out.uris.push(tr.uri);
            out.model.push({
                headline: tr.title,
                supporting: tr.artist,
                image: tr.thumb,
                avatarText: "\u266A"
            });
        }
        return out;
    }

    readonly property int pickCurrent: {
        const c = Controller.current;
        const t = Controller.tracks;
        if (c < 0 || c >= t.length)
            return -1;
        return pickRows.uris.indexOf(t[c].uri);
    }

    function toUrl(s) {
        if (!s)
            return "";
        const v = String(s);
        return v.indexOf("://") >= 0 || v.indexOf("qrc:") === 0 ? v : "file://" + v;
    }

    function thumbFor(list) {
        if (!list)
            return "";
        for (let k = list.tracks.length - 1; k >= 0; k--) {
            const tr = byUri[list.tracks[k]];
            if (tr && tr.thumb)
                return toUrl(tr.thumb);
        }
        return "";
    }

    function randomize() {
        const n = Controller.tracks.length;
        const idx = [];
        for (let i = 0; i < n; i++)
            idx.push(i);
        const take = Math.min(10, n);
        for (let i = 0; i < take; i++) {
            const j = i + Math.floor(Math.random() * (n - i));
            const tmp = idx[i];
            idx[i] = idx[j];
            idx[j] = tmp;
        }
        picks = idx.slice(0, take);
    }

    function refreshRecent() {
        recent = Controller.recent;
    }

    function playFromCarousel(i) {
        fromCarousel = true;
        Controller.playAt(i);
        carouselHold.restart();
    }

    function handleBack() {
        return false;
    }

    Timer {
        id: carouselHold
        interval: 900
        onTriggered: page.fromCarousel = false
    }

    Component.onCompleted: {
        refreshRecent();
        randomize();
    }

    Connections {
        target: Controller
        function onTracksChanged() {
            if (page.picks.length === 0)
                page.randomize();
            if (!page.fromCarousel)
                page.refreshRecent();
        }
        function onRecentChanged() {
            if (!page.fromCarousel)
                page.refreshRecent();
        }
    }

    Flickable {
        id: flick
        anchors.fill: parent
        clip: true
        contentWidth: width
        contentHeight: Math.max(height, col.height + Theme.spaceXl)
        flickableDirection: Flickable.VerticalFlick
        boundsBehavior: Flickable.StopAtBounds
        onDraggingVerticallyChanged: {
            if (page.ListView.view)
                page.ListView.view.interactive = !draggingVertically;
        }

        Column {
            id: col
            width: flick.width
            spacing: Theme.spaceXl

            Item {
                id: hero
                width: parent.width
                height: width - Theme.pageMargin * 2 + Theme.spaceMd
                visible: page.liked !== null

                readonly property string thumb: page.thumbFor(page.liked)

                Item {
                    id: heroMask
                    x: Theme.pageMargin
                    y: Theme.spaceMd
                    width: hero.width - Theme.pageMargin * 2
                    height: width
                    layer.enabled: true
                    visible: false

                    Rectangle {
                        anchors.fill: parent
                        radius: Theme.radiusLg + Theme.spaceSm
                        color: Theme.onSurfaceColor
                    }
                }

                Item {
                    id: card
                    x: heroMask.x
                    y: heroMask.y
                    width: heroMask.width
                    height: heroMask.height
                    layer.enabled: true
                    layer.effect: MultiEffect {
                        maskEnabled: true
                        maskSource: heroMask
                    }

                    Rectangle {
                        anchors.fill: parent
                        color: Theme.secondaryContainer
                    }
                    Text {
                        anchors.centerIn: parent
                        text: "\u2605\uFE0E"
                        color: Theme.onSecondaryContainerColor
                        font.pixelSize: parent.width * 0.3
                        visible: art.status === Image.Error || (hero.thumb === "" && Controller.tracks.length > 0)
                    }
                    Image {
                        id: art
                        anchors.fill: parent
                        source: hero.thumb
                        fillMode: Image.PreserveAspectCrop
                        asynchronous: true
                        sourceSize.width: 1080
                        sourceSize.height: 1080
                        opacity: status === Image.Ready ? 1 : 0
                        Behavior on opacity {
                            NumberAnimation {
                                duration: 150
                            }
                        }
                    }
                    Rectangle {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        height: parent.height * 0.4
                        gradient: Gradient {
                            GradientStop {
                                position: 0
                                color: Qt.alpha(Theme.background, 0)
                            }
                            GradientStop {
                                position: 1
                                color: Qt.alpha(Theme.background, 0.85)
                            }
                        }
                    }
                    Column {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        anchors.margins: Theme.spaceLg
                        spacing: 2
                        Text {
                            width: parent.width
                            text: page.liked ? page.liked.name : ""
                            elide: Text.ElideRight
                            color: Theme.onSurfaceColor
                            font.pixelSize: Theme.fsHeadline
                            font.weight: Font.DemiBold
                        }
                        Text {
                            width: parent.width
                            text: page.liked ? page.liked.count + (page.liked.count === 1 ? " song" : " songs") : ""
                            color: Theme.onSurfaceVariantColor
                            font.pixelSize: Theme.fsBody
                        }
                    }
                    TapHandler {
                        onTapped: page.openPlaylist("liked")
                    }
                }
            }

            Column {
                id: picksBox
                width: parent.width
                spacing: Theme.spaceSm
                visible: page.pickRows.uris.length > 0

                Item {
                    x: Theme.pageMargin
                    width: parent.width - Theme.pageMargin * 2
                    height: 40

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: "Random picks"
                        color: Theme.onSurfaceColor
                        font.pixelSize: Theme.fsTitle
                        font.weight: Font.DemiBold
                    }
                    MaterialButton {
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        label: "Randomize"
                        iconSource: "../icons/shuffle.svg"
                        type: MaterialButton.Text
                        sizeStyle: MaterialButton.Small
                        contentColor: Theme.primary
                        onClicked: page.randomize()
                    }
                }

                MaterialList {
                    x: Theme.pageMargin
                    width: parent.width - Theme.pageMargin * 2
                    variant: MaterialList.Standard
                    selectedColor: Theme.secondaryContainer
                    textColor: Theme.onSurfaceColor
                    selectedTextColor: Theme.onSurfaceColor
                    supportingColor: Theme.onSurfaceVariantColor
                    avatarColor: Theme.secondaryContainer
                    avatarTextColor: Theme.onSurfaceColor
                    dividerColor: Theme.outlineVariant
                    selectionMode: MaterialList.Single
                    model: page.pickRows.model
                    checkedIndices: page.pickCurrent >= 0 ? [page.pickCurrent] : []
                    onClicked: (index, item) => Controller.playQueue(page.pickRows.uris, index)
                    onLongPressed: (index, item) => page.songLongPressed(page.pickRows.uris[index], item.headline, item.supporting, "")
                }
            }

            Item {
                width: parent.width
                height: page.recent.length >= 3 ? 190 : 0
                visible: page.recent.length >= 3

                MaterialCarousel {
                    id: carousel
                    property bool swiped: false
                    anchors.fill: parent
                    anchors.leftMargin: Theme.pageMargin
                    anchors.rightMargin: Theme.pageMargin
                    type: MaterialCarousel.Hero
                    snap: true
                    minSmallItemSize: 28
                    maxSmallItemSize: 40
                    itemSpacing: Theme.spaceSm
                    itemRadius: Theme.radiusLg
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
                        page.playFromCarousel(i);
                    }

                    Timer {
                        id: settle
                        interval: 350
                        onTriggered: {
                            carousel.swiped = false;
                            const i = page.recent[carousel.currentIndex];
                            if (i === undefined || i === Controller.current)
                                return;
                            page.playFromCarousel(i);
                        }
                    }
                }
            }
        }
    }
}
