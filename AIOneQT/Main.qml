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


    ColumnLayout {
        Layout.fillHeight: true
        Layout.fillWidth: true

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
            Layout.fillHeight: true
            Layout.fillWidth: true
            anchors.margins: 10

            anchors.top: parent.top
            anchors.bottom: parent.bottom

            model: ListModel {
                id: messageList
            }
            delegate: Text {
                text: model.text
                color: "white"
                font.pixelSize: 14
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
                    console.log("Button was clicked!")
                    messageList.append({"text": "You: " + messageField.text})

                    messageList.append({"text": "Assistant: "})
                    var currentResponse = "Assistant: "

                    inputHandler.prompt(messageField.text, function(token) {
                        console.log("got a token", token)

                        currentResponse += token
                        if (messageList.count > 0) {
                            messageList.setProperty(messageList.count - 1, "text", currentResponse)
                        } else {
                            messageList.append({"text": token})
                            currentResponse = token
                        }
                    })

                    messageField.text = ""
                }
            }
        }
    }
}
