import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Window {
    id: window
    width: 640
    height: 480
    visible: true
    title: qsTr("Hello World")
    color: "black"

    Material.theme: Material.Dark
    Material.accent: Material.LightGreen



    RowLayout {
        TextArea {
            id: modelPathField
            Layout.fillWidth: true

            Layout.preferredHeight: 100
        }

        Button {
            text: "Load Model"

            Material.background: Material.Orange
            Material.foreground: Material.Black

            onClicked: {
                console.log("Button was clicked!")
                inputHandler.loadModel(modelPathField.text)
            }
        }
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
                inputHandler.handleButtonClickWithParam("HRawrrar ello")
            }
        }
    }
}
