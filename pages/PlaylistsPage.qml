pragma ComponentBehavior: Bound
import QtQuick
import Evernight

FadePage {
    id: page

    signal openRequested(string playlistId)
    signal createRequested

    function handleBack() {
        return false;
    }

    function toUrl(s) {
        if (!s)
            return "";
        const v = String(s);
        return v.indexOf("://") >= 0 || v.indexOf("qrc:") === 0 ? v : "file://" + v;
    }

    function thumbFor(list) {
        const t = Controller.tracks;
        const byUri = {};
        for (let i = 0; i < t.length; i++)
            byUri[t[i].uri] = t[i];
        for (let k = 0; k < list.tracks.length; k++) {
            const tr = byUri[list.tracks[k]];
            if (tr && tr.thumb)
                return page.toUrl(tr.thumb);
        }
        return "";
    }

    Flickable {
        id: flick
        flickableDirection: Flickable.VerticalFlick
        onDraggingVerticallyChanged: {
            page.ListView.view.interactive = !draggingVertically;
        }
        anchors.fill: parent
        anchors.topMargin: 12
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        clip: true
        contentWidth: width
        contentHeight: col.height + 24
        boundsBehavior: Flickable.StopAtBounds

        Column {
            id: col
            spacing: 0

            Item {
                width: grid.width
                height: 96

                Rectangle {
                    id: addBox
                    anchors.fill: parent
                    anchors.bottomMargin: 16
                    radius: 28
                    color: "transparent"
                    border.width: 1
                    border.color: Theme.outlineVariant

                    Rectangle {
                        anchors.fill: parent
                        radius: parent.radius
                        color: Theme.onSurfaceColor
                        opacity: addArea.pressed ? 0.08 : 0
                    }

                    Row {
                        anchors.centerIn: parent
                        spacing: 12

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: "+"
                            color: Theme.onSurfaceColor
                            font.pixelSize: 34
                            font.weight: Font.Light
                        }
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: "New playlist"
                            color: Theme.onSurfaceColor
                            font.pixelSize: 17
                            font.weight: Font.Medium
                        }
                    }

                    MouseArea {
                        id: addArea
                        anchors.fill: parent
                        onClicked: page.createRequested()
                    }
                }
            }

            Grid {
                id: grid
                spacing: 0
                columns: Math.max(2, Math.floor(flick.width / 172))
                readonly property real cellW: Math.floor(flick.width / columns)
                readonly property real cellH: cellW + 56
                width: cellW * columns

                Repeater {
                    model: Playlists.playlists

                    Item {
                        id: cell
                        required property var modelData
                        required property int index
                        readonly property string thumb: page.thumbFor(modelData)
                        width: grid.cellW
                        height: grid.cellH

                        Rectangle {
                            x: 0
                            y: 0
                            width: grid.cellW
                            height: grid.cellW
                            color: Theme.secondaryContainer

                            Text {
                                anchors.centerIn: parent
                                text: "\u266B"
                                color: Theme.onSurfaceColor
                                font.pixelSize: parent.width * 0.35
                                visible: art.status !== Image.Ready
                            }

                            Image {
                                id: art
                                anchors.fill: parent
                                source: cell.thumb
                                fillMode: Image.PreserveAspectCrop
                                asynchronous: true
                                sourceSize.width: 400
                                sourceSize.height: 400
                            }
                        }

                        Column {
                            x: 12
                            y: grid.cellW + 8
                            width: parent.width - 24
                            spacing: 2

                            Text {
                                width: parent.width
                                text: cell.modelData.name
                                elide: Text.ElideRight
                                color: Theme.onSurfaceColor
                                font.pixelSize: 16
                                font.weight: Font.Medium
                            }
                            Text {
                                width: parent.width
                                text: cell.modelData.count + (cell.modelData.count === 1 ? " song" : " songs")
                                elide: Text.ElideRight
                                color: Theme.onSurfaceVariantColor
                                font.pixelSize: 13
                            }
                        }

                        Rectangle {
                            anchors.fill: parent
                            color: Theme.onSurfaceColor
                            opacity: area.pressed ? 0.08 : 0
                        }

                        Rectangle {
                            anchors.left: parent.left
                            height: parent.height
                            width: 1
                            color: Theme.outlineVariant
                            visible: cell.index % grid.columns === 0
                        }
                        Rectangle {
                            anchors.right: parent.right
                            height: parent.height
                            width: 1
                            color: Theme.outlineVariant
                        }
                        Rectangle {
                            anchors.bottom: parent.bottom
                            width: parent.width
                            height: 1
                            color: Theme.outlineVariant
                        }

                        MouseArea {
                            id: area
                            anchors.fill: parent
                            onClicked: page.openRequested(cell.modelData.id)
                        }
                    }
                }
            }
        }
    }
}
