pragma Singleton
import QtQml

QtObject {
    signal requested(string message, string kind, string listId, string songUri)

    function show(message, kind, listId, songUri) {
        requested(message, kind || "", listId || "", songUri || "");
    }
}
