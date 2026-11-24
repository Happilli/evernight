import QtQuick
import Evernight.Extraa
import Evernight

Column {
    id: root
    property bool coverMode: false
    readonly property color accent: Controller.info.thumbAccent
    readonly property color fgDim: Theme.onSurfaceVariantColor
    readonly property color active: {
        if (!coverMode || accent.hslHue < 0 || accent.hslSaturation < 0.12)
            return Theme.primary;
        return Qt.hsla(accent.hslHue, accent.hslSaturation, Theme.dark ? Math.max(0.65, accent.hslLightness) : Math.min(0.4, accent.hslLightness), 1);
    }

    spacing: 2

    MaterialIconButton {
        type: MaterialIconButton.Standard
        sizeStyle: MaterialIconButton.Small
        iconSize: 24
        iconSource: "../icons/shuffle.svg"
        checked: Controller.shuffle
        stateLayer: false
        color: "transparent"
        checkedColor: "transparent"
        iconColor: root.fgDim
        checkedIconColor: root.active
        onClicked: {
            Controller.shuffle = !Controller.shuffle;
            Store.set("shuffle", Controller.shuffle);
            Snack.show(Controller.shuffle ? "Shuffle on" : "Shuffle off");
        }
    }
    MaterialIconButton {
        type: MaterialIconButton.Standard
        sizeStyle: MaterialIconButton.Small
        iconSize: 24
        iconSource: Controller.repeatMode === 2 ? "../icons/repeat_one.svg" : "../icons/repeat.svg"
        checked: Controller.repeatMode > 0
        stateLayer: false
        color: "transparent"
        checkedColor: "transparent"
        iconColor: root.fgDim
        checkedIconColor: root.active
        onClicked: {
            Controller.repeatMode = (Controller.repeatMode + 1) % 3;
            Store.set("repeatMode", Controller.repeatMode);
            Snack.show(["Repeat off", "Repeat all", "Repeat one"][Controller.repeatMode]);
        }
    }
    MaterialIconButton {
        type: MaterialIconButton.Standard
        sizeStyle: MaterialIconButton.Small
        iconSize: 24
        iconSource: "../icons/auto_next.svg"
        checked: Controller.autoNext
        stateLayer: false
        color: "transparent"
        checkedColor: "transparent"
        iconColor: root.fgDim
        checkedIconColor: root.active
        onClicked: {
            Controller.autoNext = !Controller.autoNext;
            Store.set("autoNext", Controller.autoNext);
            Snack.show(Controller.autoNext ? "Auto-next on" : "Auto-next off");
        }
    }
}
