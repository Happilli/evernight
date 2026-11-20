import QtQuick

Item {
    id: page

    width: ListView.view ? ListView.view.width : (parent ? parent.width : 0)
    height: ListView.view ? ListView.view.height : (parent ? parent.height : 0)

    readonly property real dist: ListView.view ? Math.min(1, Math.abs(x - ListView.view.contentX) / width) : 0

    opacity: 1 - 0.6 * dist
}
