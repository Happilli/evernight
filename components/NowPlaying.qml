pragma ComponentBehavior: Bound
import QtQuick
import Evernight.Extraa
import Evernight

Rectangle {
    id: root
    property real p: 0
    property bool dragging: false
    property real startP: 0
    property real lastVy: 0
    property bool blocked: false
    signal queueRequested
    readonly property int miniHeight: 76
    readonly property int miniMargin: 12
    readonly property int miniBottom: 16
    readonly property real sideMargin: 20
    readonly property real pw: parent ? parent.width : 0
    readonly property real ph: parent ? parent.height : 0
    readonly property real travel: Math.max(1, ph - miniHeight - miniBottom)
    readonly property real dragTravel: Math.min(travel, 380)
    readonly property real controlsHeight: ctl.height > 0 ? ctl.height : 190
    readonly property real controlsTop: ph - controlsHeight - 24
    readonly property var track: {
        const t = Controller.tracks;
        const c = Controller.current;
        return c >= 0 && c < t.length ? t[c] : null;
    }
    readonly property string titleText: track ? (Controller.info.title || track.title) : ""
    readonly property string subText: track ? [Controller.info.artist || track.artist, Controller.info.album].filter(function (s) {
        return s !== "";
    }).join(" \u00B7 ") : ""
    readonly property string coverUrl: track && track.thumb ? "file://" + track.thumb : ""
    readonly property bool hasCover: coverUrl !== ""

    function mix(a, b) {
        return a + (b - a) * p;
    }
    function expand() {
        dragging = false;
        p = 1;
    }
    function collapse() {
        dragging = false;
        p = 0;
    }
    function skip(dir) {
        if (dir > 0 && Controller.player.position > 3000)
            Controller.prev();
        else if (dir > 0)
            Controller.skipPrev();
        else
            Controller.next();
    }

    visible: Controller.current >= 0
    clip: true
    x: mix(miniMargin, 0)
    y: mix(ph - miniHeight - miniBottom, 0)
    width: mix(pw - 2 * miniMargin, pw)
    height: mix(miniHeight, ph)
    radius: mix(24, 0)
    color: Qt.tint(Theme.surfaceContainer, Qt.rgba(Theme.background.r, Theme.background.g, Theme.background.b, p))

    Behavior on p {
        enabled: !root.dragging
        NumberAnimation {
            duration: 240
            easing.type: Easing.OutQuart
        }
    }

    MouseArea {
        anchors.fill: parent
        enabled: !root.blocked
        onClicked: if (root.p < 0.5)
            root.expand()
    }

    DragHandler {
        enabled: !root.blocked
        target: null
        xAxis.enabled: false
        onActiveChanged: {
            if (active) {
                root.startP = root.p;
                root.lastVy = 0;
                root.dragging = true;
            } else if (root.dragging) {
                root.dragging = false;
                if (Math.abs(root.lastVy) > 200)
                    root.p = root.lastVy < 0 ? 1 : 0;
                else if (root.startP < 0.5)
                    root.p = root.p > 0.15 ? 1 : 0;
                else
                    root.p = root.p < 0.85 ? 0 : 1;
            }
        }
        onTranslationChanged: {
            if (!active)
                return;
            root.lastVy = centroid.velocity.y;
            root.p = Math.max(0, Math.min(1, root.startP - translation.y / root.dragTravel));
        }
    }

    Image {
        id: cover
        x: 0
        y: 0
        width: parent.width
        height: root.controlsTop + 60
        source: root.coverUrl
        fillMode: Image.PreserveAspectCrop
        asynchronous: true
        cache: true
        smooth: true
        opacity: root.p
        visible: root.hasCover && opacity > 0
    }

    Rectangle {
        width: parent.width
        height: cover.height * 0.55
        y: cover.height - height
        opacity: root.p
        visible: root.hasCover && opacity > 0
        gradient: Gradient {
            GradientStop {
                position: 0.0
                color: Qt.alpha(Theme.background, 0)
            }
            GradientStop {
                position: 1.0
                color: Theme.background
            }
        }
    }

    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        y: root.controlsTop / 2 - height / 2
        text: "\u266A"
        font.pixelSize: 120
        color: Theme.onSurfaceVariantColor
        opacity: root.p
        visible: !root.hasCover && opacity > 0
    }

    Text {
        id: title
        width: root.pw - 184
        x: 20
        y: 16
        elide: Text.ElideRight
        maximumLineCount: 1
        color: Theme.onSurfaceColor
        font.pixelSize: 15
        font.weight: Font.Medium
        text: root.titleText
        opacity: Math.max(0, 1 - root.p * 3)
        visible: opacity > 0
    }

    Text {
        width: title.width
        x: title.x
        y: title.y + 20
        elide: Text.ElideRight
        maximumLineCount: 1
        color: Theme.onSurfaceVariantColor
        font.pixelSize: 13
        text: root.subText
        opacity: title.opacity
        visible: opacity > 0
    }

    Row {
        id: miniControls
        anchors.right: parent.right
        anchors.rightMargin: 10
        y: (root.miniHeight - 8 - height) / 2
        spacing: 6
        opacity: Math.max(0, 1 - root.p * 3)
        visible: opacity > 0
        enabled: root.p < 0.3
        MaterialIconButton {
            type: MaterialIconButton.Tonal
            sizeStyle: MaterialIconButton.Small
            color: Theme.secondaryContainer
            iconColor: Theme.onSecondaryContainerColor
            iconSource: "../icons/prev.svg"
            onClicked: Controller.prev()
        }
        MaterialIconButton {
            type: MaterialIconButton.Filled
            sizeStyle: MaterialIconButton.Small
            color: Theme.primary
            iconColor: Theme.onPrimaryColor
            iconSource: Controller.playing ? "../icons/pause.svg" : "../icons/play.svg"
            onClicked: Controller.toggle()
        }
        MaterialIconButton {
            type: MaterialIconButton.Tonal
            sizeStyle: MaterialIconButton.Small
            color: Theme.secondaryContainer
            iconColor: Theme.onSecondaryContainerColor
            iconSource: "../icons/next.svg"
            onClicked: Controller.next()
        }
    }

    Rectangle {
        x: 18
        y: root.miniHeight - 11
        width: root.pw - 2 * root.miniMargin - 36
        height: 3
        radius: 1.5
        color: Theme.surfaceContainerHighest
        opacity: Math.max(0, 1 - root.p * 3)
        visible: opacity > 0
        Rectangle {
            height: parent.height
            radius: parent.radius
            color: Theme.onSurfaceColor
            width: Controller.player.duration > 0 ? parent.width * Math.min(1, Controller.player.position / Controller.player.duration) : 0
        }
    }

    NowPlayingActions {
        x: root.pw - root.sideMargin - width + 8
        y: root.controlsTop - height - 8
        coverMode: root.hasCover
        opacity: Math.max(0, root.p * 2 - 1)
        visible: opacity > 0
        enabled: root.p > 0.6
    }

    Loader {
        id: ctl
        x: root.sideMargin
        y: root.controlsTop
        width: root.pw - 2 * root.sideMargin
        active: root.visible
        asynchronous: true
        opacity: Math.max(0, root.p * 2 - 1)
        visible: opacity > 0
        enabled: root.p > 0.6
        sourceComponent: NowPlayingControls {
            pw: root.pw
            sideMargin: root.sideMargin
            coverMode: root.hasCover
            trackUri: root.track ? root.track.uri : ""
            titleText: root.titleText
            subText: root.subText
            onQueueRequested: root.queueRequested()
            onSkipRequested: dir => root.skip(dir)
        }
    }
}
