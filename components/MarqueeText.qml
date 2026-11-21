import QtQuick

Item {
    id: root

    property string text: ""
    property color color: "white"
    property alias font: label.font
    property bool running: true
    property real centering: 0
    property real offset: 0

    readonly property real textWidth: measure.contentWidth
    readonly property bool overflow: textWidth > width + 0.5
    readonly property bool active: overflow && running
    readonly property real distance: Math.max(0, textWidth - width + 12)

    implicitHeight: label.implicitHeight
    clip: true

    onActiveChanged: if (!active)
        offset = 0

    Text {
        id: measure
        visible: false
        text: root.text
        font: label.font
    }

    Connections {
        target: measure
        function onContentWidthChanged() {
            root.offset = 0;
            if (root.active)
                anim.restart();
        }
    }

    Text {
        id: label
        text: root.text
        color: root.color
        x: root.overflow ? root.offset : Math.max(0, (root.width - root.textWidth) / 2) * root.centering
    }

    SequentialAnimation {
        id: anim
        running: root.active
        loops: Animation.Infinite

        PauseAnimation {
            duration: 1800
        }
        NumberAnimation {
            target: root
            property: "offset"
            to: -root.distance
            duration: root.distance * 28
            easing.type: Easing.InOutSine
        }
        PauseAnimation {
            duration: 1200
        }
        NumberAnimation {
            target: root
            property: "offset"
            to: 0
            duration: root.distance * 28
            easing.type: Easing.InOutSine
        }
    }
}
