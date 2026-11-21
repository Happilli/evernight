import QtQuick
import Evernight.Extraa
import Evernight

MaterialSheet {
    id: sheet

    property var items: []
    readonly property int curPos: {
        const t = Controller.tracks;
        const c = Controller.current;
        if (c < 0 || c >= t.length)
            return -1;
        const u = t[c].uri;
        for (let i = 0; i < items.length; i++)
            if (items[i].uri === u)
                return i;
        return -1;
    }

    function refresh() {
        items = Controller.queue;
    }

    onOpenChanged: {
        if (!open)
            return;
        refresh();
        Qt.callLater(function () {
            const max = Math.max(0, flick.contentHeight - flick.height);
            flick.contentY = Math.min(max, Math.max(0, 64 + Math.max(0, sheet.curPos - 1) * 72));
        });
    }

    Connections {
        target: Controller
        function onQueueChanged() {
            if (sheet.open)
                sheet.refresh();
        }
    }

    edge: MaterialSheet.FromBottom
    extent: height * 0.75
    springStiffness: 320
    springDamping: 0.6
    color: Theme.surfaceContainerHigh
    scrimColor: Theme.scrim

    Flickable {
        id: flick
        width: parent.width
        height: parent.height
        clip: true
        contentWidth: width
        contentHeight: col.height + 24
        flickableDirection: Flickable.VerticalFlick
        boundsBehavior: Flickable.StopAtBounds

        Column {
            id: col
            x: 24
            y: 4
            width: flick.width - 48
            spacing: 12

            Column {
                width: parent.width
                spacing: 2
                Text {
                    width: parent.width
                    text: "Up next"
                    color: Theme.onSurfaceColor
                    font.pixelSize: 20
                    font.weight: Font.Medium
                }
                Text {
                    width: parent.width
                    text: sheet.items.length + (sheet.items.length === 1 ? " song" : " songs") + (Controller.shuffle ? " \u00B7 Shuffle is on, order may differ" : "")
                    elide: Text.ElideRight
                    color: Theme.onSurfaceVariantColor
                    font.pixelSize: 14
                }
            }

            MaterialList {
                width: parent.width
                visible: sheet.items.length > 0
                variant: MaterialList.Standard
                selectionMode: MaterialList.Single
                selectedColor: Theme.secondaryContainer
                textColor: Theme.onSurfaceColor
                selectedTextColor: Theme.onSurfaceColor
                supportingColor: Theme.onSurfaceVariantColor
                avatarColor: Theme.secondaryContainer
                avatarTextColor: Theme.onSurfaceColor
                dividerColor: Theme.outlineVariant
                equalizerIndex: sheet.curPos
                equalizerLevels: Controller.spectrum.levels
                checkedIndices: sheet.curPos >= 0 ? [sheet.curPos] : []
                model: sheet.items.map(function (t) {
                    return {
                        headline: t.title,
                        supporting: t.artist,
                        image: t.thumb,
                        avatarText: "\u266A"
                    };
                })
                onClicked: (index, item) => Controller.playFromQueue(index)
            }
        }
    }
}
