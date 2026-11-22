import QtQuick
import Evernight.Extraa
import Evernight

MaterialSheet {
    id: sheet

    property string uri: ""
    property string heading: ""
    property string subheading: ""
    property string fromList: ""

    readonly property var lists: Playlists.playlists

    function openFor(songUri, title, artist, listId) {
        uri = songUri;
        heading = title;
        subheading = artist;
        fromList = listId;
        open = true;
    }

    edge: MaterialSheet.FromBottom
    extent: Math.min(height * 0.8, 36 + 20 + 64 + (fromList !== "" ? 68 : 0) + 44 + Math.max(1, lists.length) * 72 + 24)
    springStiffness: 320
    springDamping: 0.6
    color: Theme.surfaceContainerHigh
    scrimColor: Theme.scrim

    Flickable {
        id: flick
        width: parent.width
        height: parent.height
        clip: true
        contentWidth: width
        contentHeight: col.height + 24
        flickableDirection: Flickable.VerticalFlick
        boundsBehavior: Flickable.StopAtBounds

        Column {
            id: col
            x: 24
            y: 4
            width: flick.width - 48
            spacing: 12

            Column {
                width: parent.width
                spacing: 2

                Text {
                    width: parent.width
                    text: sheet.heading
                    elide: Text.ElideRight
                    color: Theme.onSurfaceColor
                    font.pixelSize: 20
                    font.weight: Font.Medium
                }

                Text {
                    width: parent.width
                    visible: sheet.subheading !== ""
                    text: sheet.subheading
                    elide: Text.ElideRight
                    color: Theme.onSurfaceVariantColor
                    font.pixelSize: 14
                }
            }

            MaterialButton {
                width: parent.width
                visible: sheet.fromList !== ""
                label: "Remove from playlist"
                type: MaterialButton.Tonal
                sizeStyle: MaterialButton.Medium
                color: Theme.surfaceContainer
                contentColor: Theme.onSurfaceColor
                onClicked: {
                    Playlists.remove(sheet.fromList, sheet.uri);
                    Snack.show("Removed from playlist", "removed", sheet.fromList, sheet.uri);
                    sheet.open = false;
                }
            }

            Text {
                text: "Add to playlist"
                color: Theme.onSurfaceVariantColor
                font.pixelSize: 14
                font.weight: Font.Medium
                topPadding: 8
            }

            MaterialList {
                width: parent.width
                visible: sheet.lists.length > 0
                variant: MaterialList.Standard
                selectionMode: MaterialList.NoSelection
                textColor: Theme.onSurfaceColor
                supportingColor: Theme.onSurfaceVariantColor
                avatarColor: Theme.secondaryContainer
                avatarTextColor: Theme.onSurfaceColor
                dividerColor: Theme.outlineVariant
                model: sheet.lists.map(function (p) {
                    return {
                        headline: p.name,
                        supporting: p.count + (p.count === 1 ? " song" : " songs"),
                        trailingText: p.tracks.indexOf(sheet.uri) >= 0 ? "Added" : "",
                        avatarText: "\u266B"
                    };
                })
                onClicked: (index, item) => {
                    const l = sheet.lists[index];
                    if (Playlists.contains(l.id, sheet.uri)) {
                        Snack.show("Already in " + l.name);
                    } else {
                        Playlists.add(l.id, sheet.uri);
                        Snack.show("Added to " + l.name, "added", l.id, sheet.uri);
                    }
                    sheet.open = false;
                }
            }

            Text {
                width: parent.width
                visible: sheet.lists.length === 0
                height: 72
                verticalAlignment: Text.AlignVCenter
                wrapMode: Text.WordWrap
                text: "No playlists yet. Create one in the Playlists tab."
                color: Theme.onSurfaceVariantColor
                font.pixelSize: 14
            }
        }
    }
}
