import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQuick.Dialogs

Window {
    id: window
    width: 640
    height: 480
    visible: true
    title: qsTr("Hello World")
    color: "black"

    Material.theme: Material.Dark
    Material.accent: Material.LightGreen

    property string currentResponse: ""

    function addToken(token) {
        currentResponse += token
        if (messageList.count > 0) {
            messageList.setProperty(messageList.count - 1, "text", currentResponse)
        }
        // Auto-scroll to bottom
        messageListView.positionViewAtEnd()
    }

    RowLayout {
        FileDialog {
            id: fileDialog
            title: "Please choose a gguf file"
            nameFilters: ["GGUF files (*.gguf)"]

            onAccepted: {
                if (selectedFile) console.log("There is a file", selectedFile)
                var path = selectedFile.toString().replace("file:///", "")
                console.log("Selected file:", path, selectedFile)
                inputHandler.loadModel(path)
            }

            onRejected: {
                console.log("File selection cancelled")
            }
        }

        Button {
            text: "Load Model"
            Material.background: Material.Orange
            Material.foreground: Material.Black

            onClicked: {
                console.log("Button was clicked!")
                fileDialog.open()
            }
        }
    }

    ListView {
        id: messageListView
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: messageField.top
        anchors.margins: 10
        spacing: 10

        model: ListModel {
            id: messageList
        }

        delegate: Rectangle {
            width: messageListView.width
            height: content.height + 20
            color: model.text.startsWith("You:") ? "#2a2a2a" : "#1a1a1a"
            radius: 5

            Text {
                id: content
                width: parent.width - 20
                anchors.centerIn: parent
                text: model.text
                color: model.text.startsWith("You:") ? "#88ff88" : "white"
                font.pixelSize: 14
                wrapMode: Text.WordWrap
            }
        }
    }

    RowLayout {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 10
        spacing: 10

        TextArea {
            id: messageField
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        Button {
            text: "Send"
            width: 150
            height: 50
            Material.background: Material.Orange
            Material.foreground: Material.Black

            onClicked: {
                if (messageField.text.trim() === "") return

                console.log("Button was clicked!")

                // Add user message to list
                messageList.append({"text": "You: " + messageField.text})

                // Add empty assistant message that will be updated token-by-token
                messageList.append({"text": "Assistant: "})
                currentResponse = "Assistant: "

                // Clear the response before starting
                inputHandler.prompt(messageField.text, function(token) {
                    window.addToken(token)
                })

                messageField.text = ""
                messageListView.positionViewAtEnd()
            }
        }
    }
}
