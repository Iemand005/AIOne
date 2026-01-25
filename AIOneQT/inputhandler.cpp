#include "inputhandler.h"

#include <QDebug>

InputHandler::InputHandler(QObject *parent)
    : QObject{parent}
{}


void InputHandler::handleButtonClick() {
    qDebug() << "Button clicked from C++!";

    emit responseSent("Clicked handled in C++!");
}

void InputHandler::handleButtonClickWithParam(const QString &message) {
    qDebug() << "Received from QML:" << message;
}
