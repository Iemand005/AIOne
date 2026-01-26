#include "inputhandler.h"

#include <QDebug>
#include <iostream>

InputHandler::InputHandler(QObject *parent)
    : QObject{parent}
{
    llama_backend_init();
    ggml_backend_load_all();

    std::cout << "Llama.cpp System Info: " << llama_print_system_info() << std::endl;
}


void InputHandler::handleButtonClick() {
    qDebug() << "Button clicked from C++!";

    emit responseSent("Clicked handled in C++!");
}

void InputHandler::handleButtonClickWithParam(const QString &message) {
    qDebug() << "Received from QML:" << message;
}

void InputHandler::loadModel(const QString &path) {
    qDebug() << "Loading model at:" << path;


    // llama_model_params model_params = llama_model_default_params();
    // // llama_model *model = llama_model_load_from_file(path.toStdString().c_str(), model_params);

    // llama_model_ptr model(llama_model_load_from_file(path.toStdString().c_str(), model_params));
    this->model = std::make_unique<Model>(path.toStdString());
}

void InputHandler::prompt(const QString &message) {
    qDebug() << "Generating response to:" << message;

    // const llama_vocab * vocab = llama_model_get_vocab(model);
}
