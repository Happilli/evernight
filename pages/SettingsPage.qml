import QtQuick
import Evernight.Extraa
import Evernight

FadePage {
    id: page

    function fmt(s) {
        const h = Math.floor(s / 3600);
        const m = Math.floor((s % 3600) / 60);
        const sec = s % 60;
        const mm = String(m).padStart(2, "0");
        const ss = String(sec).padStart(2, "0");
        return h > 0 ? h + ":" + mm + ":" + ss : m + ":" + ss;
    }

    component Divider: Rectangle {
        width: parent ? parent.width : 0
        height: 1
        color: Theme.outlineVariant
    }

    component SettingRow: Item {
        id: row
        property string title: ""
        property string subtitle: ""
        default property alias trailing: slot.data
        width: parent ? parent.width : 0
        height: 56

        Column {
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            width: row.width - slot.width - 16
            spacing: 2

            Text {
                width: parent.width
                text: row.title
                elide: Text.ElideRight
                color: Theme.onSurfaceColor
                font.pixelSize: 16
            }

            Text {
                width: parent.width
                text: row.subtitle
                elide: Text.ElideRight
                color: Theme.onSurfaceVariantColor
                font.pixelSize: 13
            }
        }

        Item {
            id: slot
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            width: childrenRect.width
            height: childrenRect.height
        }
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
        contentHeight: col.height + 24 + 16
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

                Text {
                    text: "Palette"
                    color: Theme.onSurfaceColor
                    font.pixelSize: 16
                    font.weight: Font.Medium
                }

                Flickable {
                    width: parent.width
                    height: palettex.height
                    contentWidth: palettex.width
                    contentHeight: palettex.height
                    clip: true
                    flickableDirection: Flickable.HorizontalFlick
                    boundsBehavior: Flickable.StopAtBounds

                    MaterialButtonGroup {
                        id: palettex
                        labels: ThemeLoader.available.map(t => t.name)
                        variant: MaterialButtonGroup.Connected
                        sizeStyle: MaterialButtonGroup.Small
                        selectionMode: MaterialButtonGroup.Single
                        equalWidth: false
                        fillWidth: false
                        currentIndex: Math.max(0, ThemeLoader.available.findIndex(t => t.id === ThemeLoader.current))
                        color: Theme.surfaceContainer
                        checkedColor: Theme.secondaryContainer
                        textColor: Theme.onSurfaceVariantColor
                        checkedTextColor: Theme.onSecondaryContainerColor
                        borderColor: Theme.outline
                        onClicked: index => ThemeLoader.current = ThemeLoader.available[index].id
                    }
                }
            }

            Divider {}

            SettingRow {
                title: "Discord Rich Presence"
                subtitle: {
                    if (Controller.discord.connected)
                        return "Connected";
                    if (!Controller.discord.enabled)
                        return "Show what you're listening to";
                    return Controller.discord.hasToken() ? "Connecting…" : "Waiting for authorization…";
                }

                MaterialButton {
                    label: Controller.discord.enabled ? "Disconnect" : "Connect"
                    type: Controller.discord.enabled ? MaterialButton.Text : MaterialButton.Tonal
                    color: Theme.surfaceContainer
                    contentColor: Theme.onSurfaceColor
                    onClicked: {
                        if (Controller.discord.enabled) {
                            Controller.discord.logout();
                            Controller.discord.enabled = false;
                            Store.set("discord", false);
                        } else {
                            Controller.discord.enabled = true;
                            Store.set("discord", true);
                            if (!Controller.discord.hasToken())
                                Controller.discord.login();
                        }
                    }
                }
            }

            Divider {}

            SettingRow {
                title: "Sleep timer"
                subtitle: Controller.sleepRemaining > 0 ? "Stops in " + page.fmt(Controller.sleepRemaining) : "Off"

                Row {
                    spacing: 8

                    MaterialButton {
                        visible: Controller.sleepRemaining > 0
                        label: "Off"
                        type: MaterialButton.Text
                        color: Theme.surfaceContainer
                        contentColor: Theme.onSurfaceColor
                        onClicked: {
                            Controller.setSleepTimer(0);
                            Snack.show("Sleep timer off");
                        }
                    }

                    MaterialButton {
                        label: Controller.sleepRemaining > 0 ? "Change" : "Set"
                        type: MaterialButton.Tonal
                        color: Theme.surfaceContainer
                        contentColor: Theme.onSurfaceColor
                        onClicked: {
                            picker.totalSeconds = 1800;
                            picker.mode = MaterialTimePicker.Minutes;
                            sleepDialog.open = true;
                        }
                    }
                }
            }
        }
    }

    MaterialDialog {
        id: sleepDialog
        anchors.fill: parent
        title: "Sleep timer"
        confirmText: "Start"
        cancelText: "Cancel"
        color: Theme.surfaceContainerHigh
        titleColor: Theme.onSurfaceColor
        textColor: Theme.onSurfaceVariantColor
        actionColor: Theme.primary
        confirmEnabled: picker.totalSeconds > 0
        onAccepted: {
            Controller.setSleepTimer(picker.totalSeconds);
            Snack.show("Sleep timer " + page.fmt(picker.totalSeconds));
        }

        MaterialTimePicker {
            id: picker
            width: parent ? parent.width : 256
            clock: false
            showSeconds: true
            color: Theme.surfaceContainerHighest
            fieldColor: Theme.surfaceContainerHighest
            selectedFieldColor: Theme.primaryContainer
            fieldTextColor: Theme.onSurfaceColor
            selectedFieldTextColor: Theme.onPrimaryContainerColor
            handleColor: Theme.primary
            handleTextColor: Theme.onPrimaryColor
            textColor: Theme.onSurfaceColor
            outlineColor: Theme.outline
        }
    }
}
