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

    this->llm = modelFactory->loadLLM(path.toStdString());
}

void UIHandler::loadSDModel(const QString &path) {
    qDebug() << "Loading model at:" << path;

    this->sdm = modelFactory->loadSDM(path.toStdString());
}

void UIHandler::prompt(const QString &message) {
    qDebug() << "Generating response to:" << message;

    if (workerThread && workerThread->isRunning()) {
        qWarning() << "Already processing a prompt";
        return;
    }

    workerThread = new QThread();

    QObject::connect(workerThread, &QThread::started, [this, message]() {
        QMutexLocker locker(&mutex);


        this->llm->prompt(message.toStdString(), [this](const std::string &token) {
            tokenReceived(QString(token.c_str()));
        });

        workerThread->quit();
    });
    QObject::connect(workerThread, &QThread::finished, [this]() {
        workerThread->deleteLater();
        workerThread = nullptr;
    });

    workerThread->start();
}

void UIHandler::generateImage(const QString &prompt) {
    sdm->generateImage(prompt.toStdString());
}
