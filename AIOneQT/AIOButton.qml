import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material

Button {
    id: aioButton
    property color backgroundColor: Material.Orange
    
    Material.background: backgroundColor
    contentItem: Text {
        text: aioButton.text
        color: "black"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        font: aioButton.font
    }
}
