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

    Connections {
        target: inputHandler
        function onTokenReceived(token) {
            Qt.callLater(function() {
                currentResponse += token
                if (messageList.count > 0) {
                    var lastIndex = messageList.count - 1
                    messageList.setProperty(lastIndex, "text", currentResponse)
                }
                messageListView.model = messageListView.model
            })
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            Layout.leftMargin: 10
            Layout.rightMargin: 10
            Layout.topMargin: 10

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
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 10
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
            Layout.fillWidth: true
            Layout.maximumHeight: 60
            Layout.leftMargin: 10
            Layout.rightMargin: 10
            Layout.bottomMargin: 10
            spacing: 10

            TextArea {
                id: messageField
                Layout.fillWidth: true
                Layout.minimumHeight: 40
                Layout.maximumHeight: 60
            }

            Button {
                text: "Send"
                Layout.minimumWidth: 100
                Layout.minimumHeight: 40
                Material.background: Material.Orange
                Material.foreground: Material.Black

                onClicked: {
                    if (messageField.text.trim() === "") return

                    messageList.append({"text": "You: " + messageField.text})

                    currentResponse = "Assistant: "
                    messageList.append({"text": currentResponse})

                    var userMessage = messageField.text
                    messageField.text = ""
                    messageListView.positionViewAtEnd()

                    inputHandler.prompt(userMessage, function(token) {
                        Qt.callLater(function() {
                            currentResponse += token
                            if (messageList.count > 0) {
                                var lastIndex = messageList.count - 1
                                messageList.setProperty(lastIndex, "text", currentResponse)
                            }
                            messageListView.model = messageListView.model
                        })
                    })
                }
            }
        }
    }
}
