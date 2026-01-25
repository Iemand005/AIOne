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

    background: Rectangle {

    }

    color: "black"

    Material.theme: Material.Dark
    Material.accent: Material.LightGreen

    Button {
        text: "Rawrrrr7"

        width: 150
        height: 50

        anchors.centerIn: parent

        Material.background: Material.Blue
        Material.foreground: Material.White

        onClicked: {
            console.log("Button was clicked!")
            inputHandler.handleButtonClickWithParam("HRawrrar ello")
        }
    }

    TextArea {
        Layout.fillWidth: true

        Layout.preferredHeight: 100
    }
}
