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

    // InputHandler {
    //     id: inputHandler
    //     onGenerationComplete: {
    //         console.log("Done! Full text:", fullText)
    //     }
    // }


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
                // inputHandler.loadModel(modelPathField.text)
            }
        }
    }

    ListView {
        model: ListModel {
            id: messageList
        }
        delegate: Text { text: model.text }
    }

    RowLayout {
        Layout.fillWidth: true
        Layout.minimumHeight: 50
        spacing: 10
        anchors.margins: 10

        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.left: parent.left

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
                inputHandler.prompt(messageField.text, function (response) {
                    console.log("I GOT A REPONSE", response)
                    messageList.append({"text": response})
                })
            }
        }
    }
}
