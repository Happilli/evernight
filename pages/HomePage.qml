import QtQuick
import Evernight.Extraa
import Evernight

FadePage {
    id: page
    property var recent: []
    property bool fromCarousel: false
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
    Component.onCompleted: refreshRecent()
    Connections {
        target: Controller
        function onTracksChanged() {
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
        anchors.topMargin: Theme.spaceMd
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
