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

void UIHandler::prompt(const QString &message) {
    qDebug() << "Generating response to:" << message;

    if (m_workerThread && m_workerThread->isRunning()) {
        qWarning() << "Already processing a prompt";
        return;
    }

    m_workerThread = new QThread();

    QObject::connect(m_workerThread, &QThread::started, [this, message]() {
        QMutexLocker locker(&m_mutex);


        this->llm->prompt(message.toStdString(), [this](const std::string &token) {
            tokenReceived(QString(token.c_str()));
        });

        m_workerThread->quit();
    });
    QObject::connect(m_workerThread, &QThread::finished, [this]() {
        m_workerThread->deleteLater();
        m_workerThread = nullptr;
    });

    m_workerThread->start();
}
