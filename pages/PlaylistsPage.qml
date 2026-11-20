pragma ComponentBehavior: Bound

import QtQuick
import Evernight.Shapes
import Evernight.Extraa
import Evernight

FadePage {
    id: page

    signal openRequested(string playlistId)

    property string error: ""

    function openCreate() {
        field.text = "";
        error = "";
        sheet.open = true;
    }

    function closeSheet() {
        field.blur();
        sheet.open = false;
    }

    function submit() {
        const n = field.text.trim();
        if (n === "") {
            error = "Enter a name";
            return;
        }
        if (!Playlists.create(n)) {
            error = "That name is already used";
            return;
        }
        closeSheet();
    }

    function handleBack() {
        if (sheet.open) {
            closeSheet();
            return true;
        }
        return false;
    }

    Flickable {
        id: flick
        flickableDirection: Flickable.VerticalFlick
        onDraggingVerticallyChanged: {
            page.ListView.view.interactive = !draggingVertically;
            if (draggingVertically)
                field.blur();
        }
        anchors.fill: parent
        anchors.topMargin: 12
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        clip: true
        contentWidth: width
        contentHeight: grid.height + (Controller.current >= 0 ? 130 : 40)
        boundsBehavior: Flickable.StopAtBounds

        Grid {
            id: grid
            width: flick.width
            spacing: 12
            columns: Math.max(2, Math.floor((flick.width + spacing) / 172))
            readonly property real cellW: (width - spacing * (columns - 1)) / columns
            readonly property real cellH: cellW * 1.18

            Rectangle {
                width: grid.cellW
                height: grid.cellH
                radius: 28
                color: "transparent"
                border.width: 2
                border.color: Theme.outline
                scale: addArea.pressed ? 0.96 : 1

                Behavior on scale {
                    NumberAnimation {
                        duration: 120
                        easing.type: Easing.OutCubic
                    }
                }

                Column {
                    anchors.centerIn: parent
                    spacing: 8

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: "+"
                        color: Theme.onSurfaceColor
                        font.pixelSize: 64
                        font.weight: Font.Light
                    }

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: "New playlist"
                        color: Theme.onSurfaceVariantColor
                        font.pixelSize: 15
                        font.weight: Font.Medium
                    }
                }

                MouseArea {
                    id: addArea
                    anchors.fill: parent
                    onClicked: page.openCreate()
                }
            }

            Repeater {
                model: Playlists.playlists

                Rectangle {
                    id: card
                    required property var modelData
                    width: grid.cellW
                    height: grid.cellH
                    radius: 28
                    color: Theme.surfaceContainer
                    scale: area.pressed ? 0.96 : 1

                    Behavior on scale {
                        NumberAnimation {
                            duration: 120
                            easing.type: Easing.OutCubic
                        }
                    }

                    MaterialShape {
                        x: (parent.width - width) / 2
                        y: 16
                        width: parent.width * 0.62
                        height: width
                        shape: MaterialShape.Cookie9Sided
                        color: Theme.secondaryContainer
                        glyph: "\u266B"
                        glyphColor: Theme.onSurfaceColor
                        glyphSize: width * 0.5
                    }

                    Column {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        anchors.leftMargin: 16
                        anchors.rightMargin: 16
                        anchors.bottomMargin: 14
                        spacing: 2

                        Text {
                            width: parent.width
                            text: card.modelData.name
                            elide: Text.ElideRight
                            color: Theme.onSurfaceColor
                            font.pixelSize: 16
                            font.weight: Font.Medium
                        }

                        Text {
                            width: parent.width
                            text: card.modelData.count + (card.modelData.count === 1 ? " song" : " songs")
                            elide: Text.ElideRight
                            color: Theme.onSurfaceVariantColor
                            font.pixelSize: 13
                        }
                    }

                    MouseArea {
                        id: area
                        anchors.fill: parent
                        onClicked: page.openRequested(card.modelData.id)
                    }
                }
            }
        }
    }

    MaterialSheet {
        id: sheet
        anchors.fill: parent
        z: 10
        edge: MaterialSheet.FromTop
        extent: 300
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
                text: "New playlist"
                color: Theme.onSurfaceColor
                font.pixelSize: 24
                font.weight: Font.Medium
            }

            MaterialTextField {
                id: field
                width: parent.width
                type: MaterialTextField.Outlined
                label: "Playlist name"
                supportingText: page.error
                error: page.error !== ""
                maximumLength: 40
                cursorBlink: true
                contentColor: Theme.onSurfaceColor
                labelColor: Theme.onSurfaceVariantColor
                accentColor: Theme.onSurfaceColor
                outlineColor: Theme.outline
                errorColor: Theme.onSurfaceColor
                onTextChanged: page.error = ""
                onAccepted: page.submit()
            }

            Row {
                anchors.right: parent.right
                spacing: 8

                MaterialButton {
                    label: "Cancel"
                    type: MaterialButton.Text
                    contentColor: Theme.onSurfaceColor
                    onClicked: page.closeSheet()
                }

                MaterialButton {
                    label: "Create"
                    type: MaterialButton.Filled
                    color: Theme.primary
                    contentColor: Theme.onPrimaryColor
                    onClicked: page.submit()
                }
            }
        }
    }
}
