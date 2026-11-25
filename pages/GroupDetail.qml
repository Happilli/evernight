pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Effects
import Evernight.Extraa
import Evernight

Item {
    id: root

    property string kind: ""
    property string groupName: ""
    readonly property real statusInset: 40

    signal closeRequested
    signal songLongPressed(string uri, string title, string artist, string playlistId)

    readonly property var group: {
        const l = root.kind === "album" ? Controller.albums : Controller.artists;
        for (let i = 0; i < l.length; i++)
            if (l[i].name === root.groupName)
                return l[i];
        return null;
    }

    onGroupChanged: {
        if (!group && groupName !== "")
            closeRequested();
    }

    function toUrl(s) {
        if (!s)
            return "";
        const v = String(s);
        return v.indexOf("://") >= 0 || v.indexOf("qrc:") === 0 ? v : "file://" + v;
    }

    readonly property var detailRows: {
        const out = {
            model: [],
            uris: [],
            thumb: ""
        };
        const g = root.group;
        if (!g)
            return out;
        const t = Controller.tracks;
        for (let k = 0; k < g.indices.length; k++) {
            const tr = t[g.indices[k]];
            if (!tr)
                continue;
            out.uris.push(tr.uri);
            if (out.thumb === "" && tr.thumb)
                out.thumb = root.toUrl(tr.thumb);
            out.model.push({
                headline: tr.title,
                supporting: root.kind === "album" ? tr.artist : tr.album,
                image: tr.thumb,
                avatarText: "\u266A"
            });
        }
        return out;
    }

    readonly property int shownCurrent: {
        const c = Controller.current;
        const t = Controller.tracks;
        if (c < 0 || c >= t.length)
            return -1;
        return root.detailRows.uris.indexOf(t[c].uri);
    }

    function playAll(shuffle) {
        const u = detailRows.uris;
        if (u.length === 0)
            return;
        if (shuffle)
            Controller.shuffle = true;
        Controller.playQueue(u, shuffle ? Math.floor(Math.random() * u.length) : 0);
    }

    function handleBack() {
        return false;
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.background
    }

    MouseArea {
        anchors.fill: parent
    }

    Flickable {
        id: dflick
        anchors.fill: parent
        anchors.topMargin: root.statusInset + bar.collapsedHeight
        clip: true
        contentWidth: width
        contentHeight: dcol.height + Theme.spaceXl
        flickableDirection: Flickable.VerticalFlick
        boundsBehavior: Flickable.StopAtBounds

        Column {
            id: dcol
            width: dflick.width
            spacing: Theme.spaceLg

            Item {
                id: hero
                width: parent.width
                height: width - Theme.pageMargin * 2 + Theme.spaceMd

                Item {
                    id: heroMask
                    x: Theme.pageMargin
                    y: Theme.spaceMd
                    width: hero.width - Theme.pageMargin * 2
                    height: width
                    layer.enabled: true
                    visible: false

                    Rectangle {
                        anchors.fill: parent
                        radius: Theme.radiusLg + Theme.spaceSm
                        color: Theme.onSurfaceColor
                    }
                }

                Item {
                    x: heroMask.x
                    y: heroMask.y
                    width: heroMask.width
                    height: heroMask.height
                    layer.enabled: true
                    layer.effect: MultiEffect {
                        maskEnabled: true
                        maskSource: heroMask
                    }

                    Rectangle {
                        anchors.fill: parent
                        color: Theme.secondaryContainer
                    }
                    Text {
                        anchors.centerIn: parent
                        text: "\u266B"
                        color: Theme.onSecondaryContainerColor
                        font.pixelSize: parent.width * 0.3
                        visible: art.status === Image.Error || root.detailRows.thumb === ""
                    }
                    Image {
                        id: art
                        anchors.fill: parent
                        source: root.detailRows.thumb
                        fillMode: Image.PreserveAspectCrop
                        asynchronous: false
                        sourceSize.width: 1080
                        sourceSize.height: 1080
                    }
                    Rectangle {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        height: parent.height * 0.45
                        gradient: Gradient {
                            GradientStop {
                                position: 0
                                color: Qt.alpha(Theme.background, 0)
                            }
                            GradientStop {
                                position: 1
                                color: Qt.alpha(Theme.background, 0.9)
                            }
                        }
                    }
                    Column {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        anchors.margins: Theme.spaceLg
                        spacing: 2
                        Text {
                            width: parent.width
                            text: root.group ? root.group.name : ""
                            elide: Text.ElideRight
                            color: Theme.onSurfaceColor
                            font.pixelSize: Theme.fsHeadline
                            font.weight: Font.DemiBold
                        }
                        Text {
                            width: parent.width
                            text: root.detailRows.uris.length + (root.detailRows.uris.length === 1 ? " song" : " songs")
                            color: Theme.onSurfaceVariantColor
                            font.pixelSize: Theme.fsBody
                        }
                    }
                }
            }

            Row {
                id: actions
                x: Theme.pageMargin
                width: parent.width - Theme.pageMargin * 2
                spacing: Theme.spaceMd
                readonly property real btnW: (width - spacing) / 2

                MaterialButton {
                    width: actions.btnW
                    label: "Play"
                    iconSource: "../icons/play.svg"
                    type: MaterialButton.Filled
                    sizeStyle: MaterialButton.Medium
                    color: Theme.primary
                    contentColor: Theme.onPrimaryColor
                    enabled: root.detailRows.uris.length > 0
                    onClicked: root.playAll(false)
                }
                MaterialButton {
                    width: actions.btnW
                    label: "Shuffle"
                    iconSource: "../icons/shuffle.svg"
                    type: MaterialButton.Tonal
                    sizeStyle: MaterialButton.Medium
                    color: Theme.secondaryContainer
                    contentColor: Theme.onSurfaceColor
                    enabled: root.detailRows.uris.length > 0
                    onClicked: root.playAll(true)
                }
            }

            MaterialList {
                x: Theme.pageMargin
                width: parent.width - Theme.pageMargin * 2
                visible: root.detailRows.uris.length > 0
                variant: MaterialList.Standard
                selectedColor: Theme.secondaryContainer
                textColor: Theme.onSurfaceColor
                selectedTextColor: Theme.onSurfaceColor
                supportingColor: Theme.onSurfaceVariantColor
                avatarColor: Theme.secondaryContainer
                avatarTextColor: Theme.onSurfaceColor
                dividerColor: Theme.outlineVariant
                selectionMode: MaterialList.Single
                model: root.detailRows.model
                checkedIndices: root.shownCurrent >= 0 ? [root.shownCurrent] : []
                onClicked: (index, item) => Controller.playQueue(root.detailRows.uris, index)
                onLongPressed: (index, item) => root.songLongPressed(root.detailRows.uris[index], item.headline, item.supporting, "")
            }
        }
    }

    Rectangle {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: root.statusInset
        color: Theme.background
    }

    MaterialAppBar {
        id: bar
        anchors.top: parent.top
        anchors.topMargin: root.statusInset
        anchors.left: parent.left
        anchors.right: parent.right
        type: MaterialAppBar.Small
        navigation: MaterialAppBar.Back
        color: Theme.background
        iconColor: Theme.onSurfaceColor
        titleColor: Theme.onSurfaceColor
        onNavigationClicked: root.closeRequested()
    }
}
