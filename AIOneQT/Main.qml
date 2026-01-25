import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material

Window {
    id: window
    width: 640
    height: 480
    visible: true
    title: qsTr("Hello World")

    Material.theme: Material.Dark

    Button {
        text: "Rawrrrr7"

        width: 150
        height: 50

        anchors.centerIn: parent

        background: Rectangle {
            color: button.down ? "#a0a0a0" : (button.hovered ? "#707070" : "#505050")
            radius: 5
        }

        Material.background: Material.Blue
        Material.foreground: Material.White

        onClicked: {
            console.log("Button was clicked!")
        }
    }
}
