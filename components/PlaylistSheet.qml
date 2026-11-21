import QtQuick
import Evernight.Extraa
import Evernight

Item {
    id: root

    property string playlistId: ""
    property string playlistName: ""
    property string mode: "rename"
    property string error: ""
    readonly property bool opened: sheet.open

    function openCreate() {
        playlistId = "";
        playlistName = "";
        mode = "create";
        field.text = "";
        error = "";
        sheet.open = true;
    }

    function openRename(id, name) {
        playlistId = id;
        playlistName = name;
        mode = "rename";
        field.text = name;
        error = "";
        sheet.open = true;
    }

    function openDelete(id, name) {
        playlistId = id;
        playlistName = name;
        mode = "delete";
        error = "";
        sheet.open = true;
    }

    function close() {
        field.blur();
        sheet.open = false;
    }

    function handleBack() {
        if (sheet.open) {
            close();
            return true;
        }
        return false;
    }

    function submit() {
        if (mode === "delete") {
            Playlists.removeList(playlistId);
            close();
            return;
        }
        const n = field.text.trim();
        if (n === "") {
            error = "Enter a name";
            return;
        }
        const ok = mode === "create" ? Playlists.create(n) : Playlists.rename(playlistId, n);
        if (!ok) {
            error = "That name is already used";
            return;
        }
        close();
    }

    MaterialSheet {
        id: sheet
        anchors.fill: parent
        edge: MaterialSheet.FromTop
        extent: root.mode === "delete" ? 220 : 300
        springStiffness: 320
        springDamping: 0.48
        color: Theme.surfaceContainerHigh
        scrimColor: Theme.scrim
        onDismissed: field.blur()

        Column {
            x: 24
            y: 28
            width: parent.width - 48
            spacing: 20

            Text {
                text: root.mode === "create" ? "New playlist" : root.mode === "rename" ? "Rename playlist" : "Delete playlist?"
                color: Theme.onSurfaceColor
                font.pixelSize: 24
                font.weight: Font.Medium
            }

            Text {
                width: parent.width
                visible: root.mode === "delete"
                wrapMode: Text.WordWrap
                text: "\"" + root.playlistName + "\" will be removed. Your songs stay on the device."
                color: Theme.onSurfaceVariantColor
                font.pixelSize: 15
            }

            MaterialTextField {
                id: field
                width: parent.width
                visible: root.mode !== "delete"
                type: MaterialTextField.Outlined
                label: "Playlist name"
                supportingText: root.error
                error: root.error !== ""
                maximumLength: 40
                cursorBlink: true
                contentColor: Theme.onSurfaceColor
                labelColor: Theme.onSurfaceVariantColor
                accentColor: Theme.onSurfaceColor
                outlineColor: Theme.outline
                errorColor: Theme.onSurfaceColor
                onTextChanged: root.error = ""
                onAccepted: root.submit()
            }

            Row {
                anchors.right: parent.right
                spacing: 8

                MaterialButton {
                    label: "Cancel"
                    type: MaterialButton.Text
                    contentColor: Theme.onSurfaceColor
                    onClicked: root.close()
                }

                MaterialButton {
                    label: root.mode === "delete" ? "Delete" : root.mode === "create" ? "Create" : "Save"
                    type: MaterialButton.Filled
                    color: Theme.primary
                    contentColor: Theme.onPrimaryColor
                    onClicked: root.submit()
                }
            }
        }
    }
}
