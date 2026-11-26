import QtQuick
import Evernight.Extraa
import Evernight

Column {
    id: root
    property real pw: 0
    property real sideMargin: 20
    property bool coverMode: false
    property string trackUri: ""
    property string titleText: ""
    property real textAlpha: 1
    property real textShift: 0
    property string subText: ""
    property bool liked: false
    readonly property color accent: Controller.info.thumbAccent
    readonly property color fg: Theme.onSurfaceColor
    readonly property color fgDim: Theme.onSurfaceVariantColor
    readonly property color active: {
        if (!coverMode || accent.hslHue < 0 || accent.hslSaturation < 0.12)
            return Theme.primary;
        return Qt.hsla(accent.hslHue, accent.hslSaturation, Theme.dark ? Math.max(0.65, accent.hslLightness) : Math.min(0.4, accent.hslLightness), 1);
    }
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
    width: pw - 2 * sideMargin
    spacing: 12
    Item {
        id: bar
        width: parent.width
        height: 40
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
        Text {
            id: posText
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            width: 40
            horizontalAlignment: Text.AlignLeft
            color: root.fgDim
            font.pixelSize: Theme.fsLabel
            text: root.fmt((seek.pressed || bar.holding) ? bar.seekFrac * Controller.player.duration : Controller.player.position)
        }
        Text {
            id: durText
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            width: 40
            horizontalAlignment: Text.AlignRight
            color: root.fgDim
            font.pixelSize: Theme.fsLabel
            text: root.fmt(Controller.player.duration)
        }
        Item {
            id: trackArea
            anchors.left: posText.right
            anchors.right: durText.left
            anchors.leftMargin: 6
            anchors.rightMargin: 6
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            WavyProgress {
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width
                height: 20
                style: WavyProgress.Linear
                indeterminate: false
                progress: bar.shown
                animated: Controller.playing && !seek.pressed
                color: root.active
                showStopDot: true
                trackColor: Qt.alpha(root.fg, 0.25)
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
    }
    Column {
        width: parent.width
        spacing: 2
        MorphText {
            width: parent.width
            color: root.fg
            font.pixelSize: Theme.fsHeadline
            font.weight: Font.Medium
            stiffness: 520
            damping: 30
            text: root.titleText
        }
        MorphText {
            width: parent.width
            color: root.fgDim
            font.pixelSize: Theme.fsBodyLarge
            stiffness: 620
            damping: 36
            text: root.subText
        }
    }
    Item {
        width: parent.width
        height: Math.max(leftRow.height, rightRow.height)
        Row {
            id: leftRow
            anchors.left: parent.left
            anchors.leftMargin: -6
            anchors.verticalCenter: parent.verticalCenter
            spacing: 2
            MaterialIconButton {
                anchors.verticalCenter: parent.verticalCenter
                type: MaterialIconButton.Standard
                sizeStyle: MaterialIconButton.Small
                iconSize: 28
                stateLayer: false
                color: "transparent"
                iconColor: root.fg
                iconSource: "../icons/prev.svg"
                onClicked: root.skipRequested(1)
            }
            MaterialIconButton {
                anchors.verticalCenter: parent.verticalCenter
                type: MaterialIconButton.Standard
                sizeStyle: MaterialIconButton.Medium
                iconSize: 36
                stateLayer: false
                color: "transparent"
                iconColor: root.fg
                iconSource: Controller.playing ? "../icons/pause.svg" : "../icons/play.svg"
                onClicked: Controller.toggle()
            }
            MaterialIconButton {
                anchors.verticalCenter: parent.verticalCenter
                type: MaterialIconButton.Standard
                sizeStyle: MaterialIconButton.Small
                iconSize: 28
                stateLayer: false
                color: "transparent"
                iconColor: root.fg
                iconSource: "../icons/next.svg"
                onClicked: root.skipRequested(-1)
            }
        }
        Row {
            id: rightRow
            anchors.right: parent.right
            anchors.rightMargin: -8
            anchors.verticalCenter: parent.verticalCenter
            spacing: 0
            MaterialIconButton {
                type: MaterialIconButton.Standard
                sizeStyle: MaterialIconButton.Small
                iconSize: 24
                iconSource: root.liked ? "../icons/heart_filled.svg" : "../icons/heart.svg"
                checked: root.liked
                stateLayer: false
                color: "transparent"
                checkedColor: "transparent"
                iconColor: root.fgDim
                checkedIconColor: root.active
                onClicked: {
                    Playlists.toggleLike(root.trackUri);
                    Snack.show(Playlists.isLiked(root.trackUri) ? "Added to liked songs" : "Removed from liked songs");
                }
            }
            MaterialIconButton {
                type: MaterialIconButton.Standard
                sizeStyle: MaterialIconButton.Small
                iconSize: 24
                iconSource: "../icons/queue.svg"
                stateLayer: false
                color: "transparent"
                iconColor: root.fgDim
                onClicked: root.queueRequested()
            }
        }
    }
}
