import QtQuick
import Evernight.Extraa
import Evernight

Column {
    id: root

    property real pw: 0
    property string trackUri: ""
    property bool liked: false

    signal queueRequested
    signal skipRequested(int dir)

    function refreshLiked() {
        liked = trackUri !== "" && Playlists.isLiked(trackUri);
    }

    function fmt(ms) {
        var s = Math.floor(Math.max(0, ms) / 1000);
        var m = Math.floor(s / 60);
        s = s % 60;
        return m + ":" + (s < 10 ? "0" : "") + s;
    }

    onTrackUriChanged: refreshLiked()
    Component.onCompleted: refreshLiked()

    Connections {
        target: Playlists
        function onChanged() {
            root.refreshLiked();
        }
    }

    width: pw - 64
    spacing: 18

    Item {
        id: bar
        width: parent.width
        height: 44

        property real seekFrac: 0
        property bool holding: false
        readonly property real target: Math.max(0, Math.min(1, (seek.pressed || holding) ? seekFrac : (Controller.player.duration > 0 ? Controller.player.position / Controller.player.duration : 0)))
        property real shown: target

        Behavior on shown {
            enabled: seek.pressed || bar.holding
            NumberAnimation {
                duration: seek.pressed ? 70 : 160
                easing.type: Easing.Linear
            }
        }

        Timer {
            id: hold
            interval: 600
            onTriggered: bar.holding = false
        }

        WavyProgress {
            anchors.verticalCenter: parent.verticalCenter
            width: parent.width
            height: 20
            style: WavyProgress.Linear
            indeterminate: false
            progress: bar.shown
            animated: Controller.playing && !seek.pressed && !bar.holding
            color: Theme.onSurfaceColor
            trackColor: Theme.surfaceContainerHighest
            wavelength: 28
            strokeWidth: 3
            amplitude: 3
            waveSpeed: 0.8
        }

        MouseArea {
            id: seek
            anchors.fill: parent
            preventStealing: true

            function frac() {
                return Math.max(0, Math.min(1, mouseX / width));
            }

            onPressed: bar.seekFrac = frac()
            onMouseXChanged: if (pressed)
                bar.seekFrac = frac()
            onReleased: {
                bar.seekFrac = frac();
                bar.holding = true;
                hold.restart();
                if (Controller.player.duration > 0)
                    Controller.player.position = bar.seekFrac * Controller.player.duration;
            }
        }
    }

    Item {
        width: parent.width
        height: 20

        Text {
            anchors.left: parent.left
            color: Theme.onSurfaceVariantColor
            font.pixelSize: 14
            text: root.fmt((seek.pressed || bar.holding) ? bar.seekFrac * Controller.player.duration : Controller.player.position)
        }

        Text {
            anchors.right: parent.right
            color: Theme.onSurfaceVariantColor
            font.pixelSize: 14
            text: root.fmt(Controller.player.duration)
        }
    }

    Row {
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 20

        MaterialIconButton {
            anchors.verticalCenter: parent.verticalCenter
            type: MaterialIconButton.Tonal
            sizeStyle: MaterialIconButton.Medium
            iconSize: 36
            color: Theme.secondaryContainer
            iconColor: Theme.onSecondaryContainerColor
            iconSource: "../icons/prev.svg"
            onClicked: root.skipRequested(1)
        }

        MaterialIconButton {
            type: MaterialIconButton.Filled
            sizeStyle: MaterialIconButton.Large
            shape: Controller.playing ? MaterialIconButton.Round : MaterialIconButton.Square
            iconSize: 56
            color: Theme.primary
            iconColor: Theme.onPrimaryColor
            iconSource: Controller.playing ? "../icons/pause.svg" : "../icons/play.svg"
            onClicked: Controller.toggle()
        }

        MaterialIconButton {
            anchors.verticalCenter: parent.verticalCenter
            type: MaterialIconButton.Tonal
            sizeStyle: MaterialIconButton.Medium
            iconSize: 36
            color: Theme.secondaryContainer
            iconColor: Theme.onSecondaryContainerColor
            iconSource: "../icons/next.svg"
            onClicked: root.skipRequested(-1)
        }
    }

    Row {
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 14

        MaterialIconButton {
            type: MaterialIconButton.Standard
            sizeStyle: MaterialIconButton.Medium
            iconSize: 28
            iconSource: root.liked ? "../icons/heart_filled.svg" : "../icons/heart.svg"
            checked: root.liked
            color: "transparent"
            checkedColor: Theme.secondaryContainer
            iconColor: Theme.onSurfaceVariantColor
            checkedIconColor: Theme.onSurfaceColor
            onClicked: {
                Playlists.toggleLike(root.trackUri);
                Snack.show(Playlists.isLiked(root.trackUri) ? "Added to liked songs" : "Removed from liked songs");
            }
        }

        MaterialIconButton {
            type: MaterialIconButton.Standard
            sizeStyle: MaterialIconButton.Medium
            iconSize: 28
            iconSource: "../icons/shuffle.svg"
            checked: Controller.shuffle
            color: "transparent"
            checkedColor: Theme.secondaryContainer
            iconColor: Theme.onSurfaceVariantColor
            checkedIconColor: Theme.onSurfaceColor
            onClicked: {
                Controller.shuffle = !Controller.shuffle;
                Store.set("shuffle", Controller.shuffle);
                Snack.show(Controller.shuffle ? "Shuffle on" : "Shuffle off");
            }
        }

        MaterialIconButton {
            type: MaterialIconButton.Standard
            sizeStyle: MaterialIconButton.Medium
            iconSize: 28
            iconSource: Controller.repeatMode === 2 ? "../icons/repeat_one.svg" : "../icons/repeat.svg"
            checked: Controller.repeatMode > 0
            color: "transparent"
            checkedColor: Theme.secondaryContainer
            iconColor: Theme.onSurfaceVariantColor
            checkedIconColor: Theme.onSurfaceColor
            onClicked: {
                Controller.repeatMode = (Controller.repeatMode + 1) % 3;
                Store.set("repeatMode", Controller.repeatMode);
                Snack.show(["Repeat off", "Repeat all", "Repeat one"][Controller.repeatMode]);
            }
        }

        MaterialIconButton {
            type: MaterialIconButton.Standard
            sizeStyle: MaterialIconButton.Medium
            iconSize: 28
            iconSource: "../icons/auto_next.svg"
            checked: Controller.autoNext
            color: "transparent"
            checkedColor: Theme.secondaryContainer
            iconColor: Theme.onSurfaceVariantColor
            checkedIconColor: Theme.onSurfaceColor
            onClicked: {
                Controller.autoNext = !Controller.autoNext;
                Store.set("autoNext", Controller.autoNext);
                Snack.show(Controller.autoNext ? "Auto-next on" : "Auto-next off");
            }
        }

        MaterialIconButton {
            type: MaterialIconButton.Standard
            sizeStyle: MaterialIconButton.Medium
            iconSize: 28
            iconSource: "../icons/queue.svg"
            color: "transparent"
            iconColor: Theme.onSurfaceVariantColor
            onClicked: {
                root.queueRequested();
            }
        }
    }
}
