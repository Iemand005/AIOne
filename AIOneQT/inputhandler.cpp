#include "inputhandler.h"

#include <QDebug>
#include <iostream>

UIHandler::UIHandler(QObject *parent)
    : QObject{parent}
{
    modelFactory = std::make_unique<ModelFactory>();

    std::cout << "Llama.cpp System Info: " << modelFactory->systemInfoStr() << std::endl;
}


void UIHandler::handleButtonClick() {
    qDebug() << "Button clicked from C++!";

    emit responseSent("Clicked handled in C++!");
}

void UIHandler::handleButtonClickWithParam(const QString &message) {
    qDebug() << "Received from QML:" << message;
}

void UIHandler::loadModel(const QString &path) {
    qDebug() << "Loading model at:" << path;


    // llama_model_params model_params = llama_model_default_params();
    // // llama_model *model = llama_model_load_from_file(path.toStdString().c_str(), model_params);

    // llama_model_ptr model(llama_model_load_from_file(path.toStdString().c_str(), model_params));
    this->llm = modelFactory->loadLLM(path.toStdString());
}

void UIHandler::prompt(const QString &message, const QJSValue &tokenCallback) {
    qDebug() << "Generating response to:" << message;

    QString *response = new QString();

    this->llm->prompt(message.toStdString(), [this, tokenCallback](const std::string &token) {
        callJsFunction(tokenCallback, {QString(token.c_str())});
    });

    emit responseSent(*response);
}

void UIHandler::callJsFunction(const QJSValue &function, const QVariantList &args)
{
    if (!function.isCallable()) {
        return;
    }

    QJSValueList jsArgs;
    for (const QVariant &arg : args) {
        jsArgs << QJSValue(arg.toString());
    }

    QJSValue result = function.call(jsArgs);
    if (result.isError()) {
        qWarning() << result.toString();
    }
}
