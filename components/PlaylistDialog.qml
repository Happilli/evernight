import QtQuick
import Evernight.Extraa
import Evernight

Item {
    id: root

    property string playlistId: ""
    property string playlistName: ""
    property string mode: "rename"
    readonly property bool opened: dialog.open

    readonly property string trimmed: field.text.trim()
    readonly property bool taken: mode !== "delete" && trimmed !== "" && trimmed !== playlistName && Playlists.exists(trimmed)

    function show(m, id, name) {
        mode = m;
        playlistId = id;
        playlistName = name;
        field.text = m === "rename" ? name : "";
        dialog.open = true;
        if (m !== "delete")
            Qt.callLater(() => field.forceActiveFocus());
    }
    function openCreate() {
        show("create", "", "");
    }
    function openRename(id, name) {
        show("rename", id, name);
    }
    function openDelete(id, name) {
        show("delete", id, name);
    }
    function close() {
        dialog.reject();
    }
    function handleBack() {
        if (dialog.open) {
            dialog.reject();
            return true;
        }
        return false;
    }

    MaterialDialog {
        id: dialog
        anchors.fill: parent

        title: root.mode === "create" ? "New playlist" : root.mode === "rename" ? "Rename playlist" : "Delete playlist?"
        text: root.mode === "delete" ? "\"" + root.playlistName + "\" will be removed. Your songs stay on the device." : ""
        confirmText: root.mode === "delete" ? "Delete" : root.mode === "create" ? "Create" : "Save"
        destructive: root.mode === "delete"
        confirmEnabled: root.mode === "delete" || (root.trimmed !== "" && !root.taken)

        color: Theme.surfaceContainerHigh
        scrimColor: Theme.scrim
        titleColor: Theme.onSurfaceColor
        textColor: Theme.onSurfaceVariantColor
        actionColor: Theme.onSurfaceColor
        dangerColor: Theme.error

        onAccepted: {
            if (root.mode === "delete")
                Playlists.removeList(root.playlistId);
            else if (root.mode === "create")
                Playlists.create(root.trimmed);
            else if (root.trimmed !== root.playlistName)
                Playlists.rename(root.playlistId, root.trimmed);
        }
        onOpenChanged: {
            if (!open)
                field.blur();
        }

        MaterialTextField {
            id: field
            width: parent.width
            visible: root.mode !== "delete"
            type: MaterialTextField.Outlined
            label: "Playlist name"
            supportingText: root.taken ? "That name is already used" : " "
            error: root.taken
            maximumLength: 40
            cursorBlink: true
            contentColor: Theme.onSurfaceColor
            labelColor: Theme.onSurfaceVariantColor
            accentColor: Theme.onSurfaceColor
            outlineColor: Theme.outline
            errorColor: Theme.onSurfaceColor
            onAccepted: dialog.accept()
        }
    }
}
