import QtQuick
import Evernight.Extraa
import Evernight

FadePage {
    id: page

    Component.onCompleted: {
        const m = Number(Store.get("themeMode", 0));
        Theme.mode = m >= 0 && m <= 2 ? m : 0;
        const a = Store.get("autoNext", true);
        Controller.autoNext = a === true || a === "true";
    }
    Connections {
        target: Controller.discord
        function onAuthRequired() {
            Controller.discord.login();
        }
    }

    Flickable {
        id: flick
        anchors.fill: parent
        anchors.leftMargin: 20
        anchors.rightMargin: 20
        clip: true
        contentWidth: width
        contentHeight: col.height + 24 + (Controller.current >= 0 ? 120 : 32)
        flickableDirection: Flickable.VerticalFlick
        boundsBehavior: Flickable.StopAtBounds
        onDraggingVerticallyChanged: {
            if (page.ListView.view)
                page.ListView.view.interactive = !draggingVertically;
        }

        Column {
            id: col
            y: 24
            width: flick.width
            spacing: 20

            Column {
                width: parent.width
                spacing: 10

                Text {
                    text: "Theme"
                    color: Theme.onSurfaceColor
                    font.pixelSize: 16
                    font.weight: Font.Medium
                }

                MaterialSegmentedButton {
                    width: parent.width
                    fillWidth: true
                    labels: ["System", "Light", "Dark"]
                    selectionMode: MaterialSegmentedButton.Single
                    currentIndex: Theme.mode
                    color: "transparent"
                    checkedColor: Theme.secondaryContainer
                    textColor: Theme.onSurfaceVariantColor
                    checkedTextColor: Theme.onSurfaceColor
                    borderColor: Theme.outline
                    onClicked: index => {
                        Theme.mode = index;
                        Store.set("themeMode", index);
                    }
                }
            }

            Rectangle {
                width: parent.width
                height: 1
                color: Theme.outlineVariant
            }

            Item {
                width: parent.width
                height: 56

                Column {
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 2
                    Text {
                        text: "Auto-play next"
                        color: Theme.onSurfaceColor
                        font.pixelSize: 16
                    }
                    Text {
                        text: "Continue with the next song when one ends"
                        color: Theme.onSurfaceVariantColor
                        font.pixelSize: 13
                    }
                }

                MaterialSwitch {
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    checked: Controller.autoNext
                    trackColor: Theme.surfaceContainerHighest
                    handleColor: Theme.outline
                    checkedTrackColor: Theme.onSurfaceColor
                    checkedHandleColor: Theme.onPrimaryColor
                    borderColor: checked ? "transparent" : Theme.outline
                    onToggled: {
                        Controller.autoNext = checked;
                        Store.set("autoNext", checked);
                    }
                }
            }

            Rectangle {
                width: parent.width
                height: 1
                color: Theme.outlineVariant
            }

            Item {
                width: parent.width
                height: 56

                Column {
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 2
                    Text {
                        text: "Discord Rich Presence"
                        color: Theme.onSurfaceColor
                        font.pixelSize: 16
                    }
                    Text {
                        text: Controller.discord.connected ? "Connected" : (Controller.discord.enabled ? (Controller.discord.hasToken() ? "Connecting…" : "Authorization required") : "Show what you're listening to")
                        color: Theme.onSurfaceVariantColor
                        font.pixelSize: 13
                    }
                }

                MaterialSwitch {
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    checked: Controller.discord.enabled
                    trackColor: Theme.surfaceContainerHighest
                    handleColor: Theme.outline
                    checkedTrackColor: Theme.onSurfaceColor
                    checkedHandleColor: Theme.onPrimaryColor
                    borderColor: checked ? "transparent" : Theme.outline
                    onToggled: {
                        Controller.discord.enabled = checked;
                        Store.set("discord", checked);
                        if (checked && !Controller.discord.hasToken())
                            Controller.discord.login();
                    }
                }
            }

            Rectangle {
                width: parent.width
                height: 1
                color: Theme.outlineVariant
            }

            Item {
                width: parent.width
                height: 56

                Column {
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 2
                    Text {
                        text: "Library"
                        color: Theme.onSurfaceColor
                        font.pixelSize: 16
                    }
                    Text {
                        text: Controller.tracks.length + " songs"
                        color: Theme.onSurfaceVariantColor
                        font.pixelSize: 13
                    }
                }

                MaterialButton {
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    label: "Rescan"
                    type: MaterialButton.Tonal
                    color: Theme.surfaceContainer
                    contentColor: Theme.onSurfaceColor
                    onClicked: Controller.scanLibrary()
                }
            }
        }
    }
}
