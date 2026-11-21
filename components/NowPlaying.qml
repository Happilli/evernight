pragma ComponentBehavior: Bound
import QtQuick
import Evernight.Shapes
import Evernight.Extraa
import Evernight

Rectangle {
    id: root

    property real p: 0
    property bool dragging: false
    property real startP: 0
    property real lastVy: 0
    property real swipeX: 0
    property bool swiping: false
    property bool snapping: false
    property real lastVx: 0
    readonly property var shapePool: [MaterialShape.Cookie9Sided, MaterialShape.Cookie12Sided, MaterialShape.Cookie6Sided, MaterialShape.Clover4Leaf, MaterialShape.Clover8Leaf, MaterialShape.SoftBurst, MaterialShape.Flower, MaterialShape.Puffy, MaterialShape.PuffyDiamond, MaterialShape.Sunny, MaterialShape.VerySunny, MaterialShape.Pentagon, MaterialShape.Gem, MaterialShape.Ghostish, MaterialShape.Bun, MaterialShape.Heart, MaterialShape.Oval]
    property int coverShape: MaterialShape.Cookie9Sided

    function pickShape() {
        let n = coverShape;
        while (n === coverShape)
            n = shapePool[Math.floor(Math.random() * shapePool.length)];
        coverShape = n;
    }

    Connections {
        target: Controller
        function onCurrentChanged() {
            root.pickShape();
        }
    }

    readonly property int miniHeight: 76
    readonly property int miniMargin: 12
    readonly property int miniBottom: 16
    readonly property real swipeT: swipeX / Math.max(1, pw)
    readonly property real arcY: -artSize * 0.5 * swipeT * swipeT
    readonly property real pw: parent ? parent.width : 0
    readonly property real ph: parent ? parent.height : 0
    readonly property real travel: Math.max(1, ph - miniHeight - miniBottom)
    readonly property real artSize: Math.min(pw - 64, ph * 0.34, 300)
    readonly property real artTop: Math.max(60, (ph - artSize - 420) / 2)
    readonly property real controlsTop: artTop + artSize + 90
    readonly property var track: Controller.current >= 0 && Controller.current < Controller.tracks.length ? Controller.tracks[Controller.current] : null
    readonly property string titleText: track ? (Controller.info.title || track.title) : ""
    readonly property string subText: track ? [Controller.info.artist || track.artist, Controller.info.album].filter(function (s) {
        return s !== "";
    }).join(" \u00B7 ") : ""

    readonly property color ringTarget: {
        if (!Controller.info.hasCover)
            return "transparent";
        const a = Controller.info.accent;
        return Qt.hsla(Math.max(0, a.hslHue), Math.min(0.75, Math.max(0.35, a.hslSaturation)), Theme.dark ? 0.62 : 0.42, 1);
    }
    property color ringColor: ringTarget

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

    function commitSwipe(dir) {
        swiping = false;
        exitTimer.dir = dir;
        swipeX = dir * pw;
        exitTimer.restart();
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
            duration: 320
            easing.type: Easing.OutCubic
        }
    }

    Behavior on ringColor {
        ColorAnimation {
            duration: 500
        }
    }

    Behavior on swipeX {
        enabled: !root.swiping && !root.snapping
        SpringAnimation {
            spring: 2.5
            damping: 0.22
            epsilon: 0.5
        }
    }

    Timer {
        id: exitTimer
        property int dir: 0
        interval: 140
        onTriggered: {
            if (dir < 0)
                Controller.next();
            else
                Controller.skipPrev();
            root.snapping = true;
            root.swipeX = -dir * root.pw;
            root.snapping = false;
            root.swipeX = 0;
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: if (root.p < 0.5)
            root.expand()
    }

    DragHandler {
        target: null
        xAxis.enabled: false
        onActiveChanged: {
            if (active) {
                root.startP = root.p;
                root.lastVy = 0;
                root.dragging = true;
            } else if (root.dragging) {
                root.dragging = false;
                if (Math.abs(root.lastVy) > 500)
                    root.p = root.lastVy < 0 ? 1 : 0;
                else
                    root.p = root.p > 0.5 ? 1 : 0;
            }
        }
        onTranslationChanged: {
            if (!active)
                return;
            root.lastVy = centroid.velocity.y;
            root.p = Math.max(0, Math.min(1, root.startP - translation.y / root.travel));
        }
    }

    MaterialShape {
        x: root.mix(14, (root.pw - root.artSize) / 2) + root.swipeX * root.p
        y: root.mix(12, root.artTop) + root.arcY * root.p
        width: root.mix(44, root.artSize)
        height: width
        rotation: root.swipeT * 30 * root.p
        scale: 1 - 0.12 * Math.min(1, Math.abs(root.swipeT))
        shape: Controller.playing ? root.coverShape : MaterialShape.Circle
        color: Theme.secondaryContainer
        image: Controller.info.cover
        glyph: Controller.info.hasCover ? "" : "\u266A"
        glyphColor: Theme.onSurfaceColor
        glyphSize: root.mix(20, 96)
        border.width: Controller.info.hasCover ? root.mix(2, 5) : 0
        border.color: root.ringColor
    }

    Text {
        id: title
        width: root.mix(root.pw - 252, root.pw - 64)
        x: root.mix(68, 32) + root.p * Math.max(0, (width - contentWidth) / 2) + root.swipeX * 0.3 * root.p
        y: root.mix(16, root.artTop + root.artSize + 18)
        elide: Text.ElideRight
        color: Theme.onSurfaceColor
        font.pixelSize: Math.round(root.mix(15, 24))
        font.weight: Font.Medium
        text: root.titleText
        opacity: 1 - Math.min(1, Math.abs(root.swipeX) / Math.max(1, root.pw) * 1.5)
    }

    Text {
        id: sub
        width: title.width
        x: root.mix(68, 32) + root.p * Math.max(0, (width - contentWidth) / 2) + root.swipeX * 0.3 * root.p
        y: title.y + root.mix(20, 34)
        elide: Text.ElideRight
        color: Theme.onSurfaceVariantColor
        font.pixelSize: Math.round(root.mix(13, 16))
        text: root.subText
        opacity: 1 - Math.min(1, Math.abs(root.swipeX) / Math.max(1, root.pw) * 1.5)
    }

    Item {
        width: root.pw
        height: root.controlsTop
        enabled: root.p > 0.9

        DragHandler {
            target: null
            yAxis.enabled: false
            onActiveChanged: {
                if (active) {
                    root.swiping = true;
                    root.lastVx = 0;
                } else if (root.swiping) {
                    if (Math.abs(root.swipeX) > root.pw * 0.25 || Math.abs(root.lastVx) > 600)
                        root.commitSwipe((root.swipeX !== 0 ? root.swipeX : root.lastVx) < 0 ? -1 : 1);
                    else {
                        root.swiping = false;
                        root.swipeX = 0;
                    }
                }
            }
            onTranslationChanged: {
                if (!active)
                    return;
                root.lastVx = centroid.velocity.x;
                root.swipeX = translation.x * 0.85;
            }
        }
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

    Loader {
        x: 32
        y: root.controlsTop
        width: root.pw - 64
        active: root.p > 0
        opacity: Math.max(0, root.p * 2 - 1)
        visible: opacity > 0
        enabled: root.p > 0.6

        sourceComponent: NowPlayingControls {
            pw: root.pw
        }
    }
}
