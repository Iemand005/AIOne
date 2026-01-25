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

void InputHandler::loadModel(const QString &path) {
    qDebug() << "Loading model at:" << path;


    llama_model_params model_params = llama_model_default_params();
    // llama_model *model = llama_model_load_from_file(path.toStdString().c_str(), model_params);

    llama_model_ptr model(llama_model_load_from_file(path.toStdString().c_str(), model_params));
    this->model = std::move(model);
}
