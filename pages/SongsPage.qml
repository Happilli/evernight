import QtQuick
import Evernight.Extraa
import Evernight

FadePage {
    id: page
    property string query: ""
    property var matches: []

    function refresh() {
        matches = Controller.search(query);
    }

    onQueryChanged: refresh()
    Component.onCompleted: refresh()

    Connections {
        target: Controller
        function onTracksChanged() {
            page.refresh();
        }
    }
    readonly property int shownCurrent: Controller.current >= 0 ? page.matches.indexOf(Controller.current) : -1

    function handleBack() {
        if (songSheet.open) {
            songSheet.open = false;
            return true;
        }
        return false;
    }

    MaterialSearchBox {
        id: search
        anchors.top: parent.top
        anchors.topMargin: 12
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        placeholderText: "Search songs"
        clearable: true
        showSearchIcon: true
        color: Theme.surfaceContainerHigh
        textColor: Theme.onSurfaceColor
        placeholderColor: Theme.onSurfaceVariantColor
        iconColor: Theme.onSurfaceVariantColor
        cursorColor: Theme.onSurfaceColor
        selectionColor: Qt.alpha(Theme.onSurfaceColor, 0.3)
        onTextChanged: page.query = text
        onAccepted: search.blur()
    }

    Flickable {
        id: flick
        flickableDirection: Flickable.VerticalFlick
        onDraggingVerticallyChanged: {
            page.ListView.view.interactive = !draggingVertically;
            if (draggingVertically)
                search.blur();
        }
        anchors.top: search.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 8
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        clip: true
        contentWidth: width
        contentHeight: col.height + (Controller.current >= 0 ? 120 : 32)
        boundsBehavior: Flickable.StopAtBounds

        Column {
            id: col
            width: flick.width
            spacing: 0

            MaterialList {
                width: parent.width
                visible: page.matches.length > 0
                variant: MaterialList.Standard
                selectedColor: Theme.secondaryContainer
                textColor: Theme.onSurfaceColor
                selectedTextColor: Theme.onSurfaceColor
                supportingColor: Theme.onSurfaceVariantColor
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
                            avatarText: "\u266A"
                        };
                    });
                }
                checkedIndices: page.shownCurrent >= 0 ? [page.shownCurrent] : []
                onClicked: (index, item) => {
                    search.blur();
                    Controller.playAt(page.matches[index]);
                }
                onLongPressed: (index, item) => {
                    search.blur();
                    const t = Controller.tracks[page.matches[index]];
                    songSheet.openFor(t.uri, t.title, t.artist, "");
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
                        text: Controller.tracks.length === 0 ? "No songs found" : "No results"
                        color: Theme.onSurfaceColor
                        font.pixelSize: 18
                    }

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: Controller.tracks.length === 0 ? "Put music in your Music folder" : "Nothing matches \"" + page.query.trim() + "\""
                        color: Theme.onSurfaceVariantColor
                        font.pixelSize: 14
                    }
                }
            }
        }
    }

    SongSheet {
        id: songSheet
        anchors.fill: parent
        z: 10
    }
}
