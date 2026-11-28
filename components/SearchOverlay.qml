import QtQuick
import Evernight.Extraa
import Evernight

Item {
    id: root

    property bool open: false
    property rect origin: Qt.rect(0, 0, 0, 0)
    property real originRadius: 20
    property real topInset: 40
    property var rows: []
    property var tracks: []
    property real p: open ? 1 : 0
    readonly property real q: Math.max(0, Math.min(1, p))
    readonly property bool active: open || p > 0.01

    visible: active

    Behavior on p {
        SpringAnimation {
            spring: root.open ? 4 : 12
            damping: root.open ? 0.45 : 0.9
            epsilon: 0.0005
        }
    }

    function openFrom(item) {
        const r = item.mapToItem(root, 0, 0);
        origin = Qt.rect(r.x, r.y, item.width, item.height);
        originRadius = Math.min(item.width, item.height) / 2;
        open = true;
    }

    function close() {
        open = false;
        box.text = "";
        rows = [];
    }

    function handleBack() {
        if (!open)
            return false;
        close();
        return true;
    }

    function runSearch() {
        const q = box.text.trim();
        if (q === "") {
            rows = [];
            return;
        }
        const idx = Controller.search(q);
        const n = Math.min(idx.length, 100);
        const out = [];
        for (let i = 0; i < n; ++i) {
            const t = tracks[idx[i]];
            out.push({
                headline: t.title,
                supporting: t.artist,
                thumb: t.thumb,
                index: idx[i]
            });
        }
        rows = out;
    }

    onOpenChanged: {
        if (open) {
            tracks = Controller.tracks;
            box.forceActiveFocus();
            Keyboard.show();
        } else {
            box.blur();
            Keyboard.hide();
        }
    }

    MouseArea {
        anchors.fill: parent
        enabled: root.active
    }

    Rectangle {
        id: panel
        x: root.origin.x * (1 - root.p)
        y: root.origin.y * (1 - root.p)
        width: root.origin.width + (root.width - root.origin.width) * root.p
        height: root.origin.height + (root.height - root.origin.height) * root.p
        radius: Math.max(0, root.originRadius * (1 - root.q))
        color: Qt.alpha(Theme.background, Math.min(1, root.q * 5))
        clip: true

        MaterialSearchBox {
            id: box
            x: 0
            y: root.topInset * root.q
            width: panel.width
            height: 56
            opacity: Math.max(0, (root.q - 0.35) / 0.65)
            color: "transparent"
            textColor: Theme.onSurfaceColor
            placeholderColor: Theme.onSurfaceVariantColor
            iconColor: Theme.onSurfaceVariantColor
            cursorColor: Theme.primary
            placeholderText: "Search songs"
            leadingIconSource: root.open ? "../icons/arrow_back.svg" : ""
            onLeadingClicked: root.close()
            onTextEdited: debounce.restart()
        }
        Timer {
            id: debounce
            interval: 120
            onTriggered: root.runSearch()
        }
        Rectangle {
            id: divider
            y: box.y + box.height
            width: panel.width
            height: 1
            color: Theme.outlineVariant
            opacity: root.q
        }

        MaterialSearchView {
            id: view
            y: divider.y + 1
            width: panel.width
            height: Math.max(0, panel.height - y)
            open: root.open
            fillHeight: true
            autoSelect: false
            color: "transparent"
            radius: 0
            textColor: Theme.onSurfaceColor
            supportingColor: Theme.onSurfaceVariantColor
            iconColor: Theme.onSurfaceVariantColor
            highlightColor: Qt.alpha(Theme.onSurfaceColor, 0.08)
            emptyText: box.text.length > 0 ? "No results" : "Search your music"
            model: root.rows
            onTriggered: (index, item) => {
                Controller.playAt(item.index);
                root.close();
            }
        }
    }
}
