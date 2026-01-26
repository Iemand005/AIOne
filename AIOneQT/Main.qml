import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQuick.Dialogs

ApplicationWindow {
    id: window
    width: 640
    height: 480
    visible: true
    title: qsTr("Hello World")

    Material.theme: Material.Dark
    Material.accent: Material.LightGreen

    property string currentResponse: ""
    property bool hasImage: false
    property int imageCounter: 0

    Connections {
        target: inputHandler
        function onTokenReceived(token) {
            currentResponse += token
            if (messageList.count > 0) {
                messageList.setProperty(messageList.count - 1, "text", currentResponse)
                messageListView.positionViewAtEnd()
            }
        }
        function onImageGenerated(image) {
            hasImage = true
            imageCounter++  // Increment to force cache buster
            generatedImageDisplay.source = "image://generated/image?id=" + imageCounter
            console.log("Image generated successfully, counter:", imageCounter)
        }
    }

    TabBar {
        id: tabBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        TabButton {
            text: "LLM Chat"
        }
        TabButton {
            text: "Stable Diffusion"
        }
    }

    StackLayout {
        // Layout.top: tabBar.bottom
        anchors.top: tabBar.bottom
        Layout.fillWidth: true
        // Layout.bottom: parent.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        currentIndex: tabBar.currentIndex

        // LLM Tab
        ColumnLayout {
            anchors.fill: parent
            Layout.fillWidth: true
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

                        inputHandler.prompt(userMessage)
                    }
                }
            }
        }

        // Stable Diffusion Tab
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 20
            spacing: 10

            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                Layout.leftMargin: 10
                Layout.rightMargin: 10
                Layout.topMargin: 10

                FileDialog {
                    id: sdFileDialog
                    title: "Please choose a safetensors file"
                    nameFilters: ["SafeTensors files (*.safetensors)"]

                    onAccepted: {
                        if (selectedFile) console.log("There is a file", selectedFile)
                        var path = selectedFile.toString().replace("file:///", "")
                        console.log("Selected file:", path, selectedFile)
                        inputHandler.loadSDModel(path)
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
                        sdFileDialog.open()
                    }
                }
            }

            TextField {
                id: sdPromptField
                Layout.fillWidth: true
                placeholderText: "Enter prompt for image generation..."
            }

            Button {
                text: "Generate Image"
                Layout.fillWidth: true
                Material.background: Material.Purple
                Material.foreground: Material.Black

                onClicked: function() {
                    console.log("Fine.. I'll make one gimage for u bro", "oh and da promp is he", sdPromptField.text)
                    inputHandler.generateImage(sdPromptField.text)
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#1a1a1a"
                border.color: "#333"
                border.width: 1

                Image {
                    id: generatedImageDisplay
                    anchors.fill: parent
                    fillMode: Image.PreserveAspectFit
                    asynchronous: true
                }

                Text {
                    anchors.centerIn: parent
                    text: "Generated image will appear here"
                    color: "#888"
                    visible: generatedImageDisplay.source === ""
                }
            }
        }
    }
}
