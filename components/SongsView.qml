import QtQuick
import Evernight.Extraa
import Evernight

Flickable {
    id: flick
    property var matches: []
    property var infoCache: ({})
    readonly property int shownCurrent: Controller.current >= 0 ? matches.indexOf(Controller.current) : -1
    signal songLongPressed(string uri, string title, string artist, string playlistId)
    signal dragLock(bool locked)
    function refresh() {
        matches = Controller.search("");
    }
    function handleBack() {
        if (songList.expandedIndex >= 0) {
            songList.collapseAll();
            return true;
        }
        return false;
    }
    Component.onCompleted: refresh()
    onDraggingVerticallyChanged: dragLock(draggingVertically)
    clip: true
    contentWidth: width
    contentHeight: col.height + Theme.spaceLg
    flickableDirection: Flickable.VerticalFlick
    boundsBehavior: Flickable.StopAtBounds
    Connections {
        target: Controller
        function onTracksChanged() {
            flick.refresh();
        }
        function onInspected(uri, rows) {
            if (rows.length > 0)
                flick.infoCache[uri] = rows;
            const t = Controller.tracks;
            for (let k = 0; k < flick.matches.length; k++) {
                if (t[flick.matches[k]].uri === uri) {
                    songList.setDetails(k, rows);
                    break;
                }
            }
        }
    }
    Column {
        id: col
        x: Theme.pageMargin
        width: flick.width - 2 * Theme.pageMargin
        spacing: 0
        MaterialList {
            id: songList
            width: parent.width
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
            model: {
                const t = Controller.tracks;
                return flick.matches.map(function (i) {
                    return {
                        headline: t[i].title,
                        supporting: t[i].artist,
                        image: t[i].thumb,
                        avatarText: "\u266A",
                        trailingIcon: "\u22EE",
                        expandedIcon: "\u00D7",
                        details: flick.infoCache[t[i].uri] || []
                    };
                });
            }
            checkedIndices: flick.shownCurrent >= 0 ? [flick.shownCurrent] : []
            onClicked: (index, item) => Controller.playAt(flick.matches[index])
            onLongPressed: (index, item) => {
                const t = Controller.tracks[flick.matches[index]];
                flick.songLongPressed(t.uri, t.title, t.artist, "");
            }
            onExpandRequested: (index, item) => {
                Controller.inspect(Controller.tracks[flick.matches[index]].uri);
            }
        }
    }
}
