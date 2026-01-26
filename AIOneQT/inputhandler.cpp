#include "inputhandler.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <QDebug>
#include <iostream>

bool saveImageAsPNG(const sd_image_t& image, const std::string& filename) {
    if (!image.data || image.width == 0 || image.height == 0) {
            std::cerr << "Invalid image data" << std::endl;
        return false;
    }

    int success = stbi_write_png(filename.c_str(),
                                 image.width,
                                 image.height,
                                 image.channel,  // RGB
                                 image.data,
                                 image.width * image.channel);

    if (success) {
        std::cout << "Saved image to: " << filename << std::endl;
        return true;
    } else {
        std::cerr << "Failed to save image: " << filename << std::endl;
        return false;
    }
}

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
    sd_image_t image = sdm->generateImage(prompt.toStdString());
    saveImageAsPNG(image, "test.png");
}
