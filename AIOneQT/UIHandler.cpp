#include "inputhandler.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <QDebug>
#include <QImage>
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

QImage convertToQImage(const sd_image_t& sd_img) {
    if (sd_img.channel == 3) {
        // Convert RGB to RGBA QImage
        QImage image(sd_img.width, sd_img.height, QImage::Format_RGB32);

        for (int y = 0; y < sd_img.height; y++) {
            QRgb* scanline = (QRgb*)image.scanLine(y);
            const uint8_t* src = sd_img.data + (y * sd_img.width * 3);

            for (int x = 0; x < sd_img.width; x++) {
                scanline[x] = qRgb(src[x * 3], src[x * 3 + 1], src[x * 3 + 2]);
            }
        }
        return image;
    } else if (sd_img.channel == 4) {
        // Already RGBA
        return QImage(sd_img.data, sd_img.width, sd_img.height,
                      sd_img.width * 4, QImage::Format_RGBA8888).copy();
    }

    return QImage();
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
    
    if (llmWorkerThread && llmWorkerThread->isRunning()) {
        qWarning() << "Already loading LLM model";
        return;
    }

    std::string pathStr = path.toStdString();
    llmWorkerThread = new QThread();

    llmWorkerThread->setStackSize(256 * 1024 * 1024);

    // this->llm = modelFactory->loadLLM(pathStr);

    QObject::connect(llmWorkerThread, &QThread::started, [this, pathStr]() {
        QMutexLocker locker(&llmMutex);
        try {
            this->llm = modelFactory->loadLLM(pathStr);
            qDebug() << "LLM model loaded successfully";
        } catch (const std::exception &e) {
            qCritical() << "Failed to load LLM model:" << e.what();
            this->llm = nullptr;
        }
        llmWorkerThread->quit();
    });
    
    QObject::connect(llmWorkerThread, &QThread::finished, [this]() {
        llmWorkerThread->deleteLater();
        llmWorkerThread = nullptr;
    });

    llmWorkerThread->start();
}

void UIHandler::loadSDModel(const QString &path) {
    qDebug() << "Loading SD model at:" << path;
    
    if (sdWorkerThread && sdWorkerThread->isRunning()) {
        qWarning() << "Already loading SD model";
        return;
    }

    std::string pathStr = path.toStdString();
    sdWorkerThread = new QThread();
    
    // Increase stack size to 8MB for complex tokenizer patterns
    sdWorkerThread->setStackSize(8 * 1024 * 1024);

    QObject::connect(sdWorkerThread, &QThread::started, [this, pathStr]() {
        QMutexLocker locker(&sdMutex);
        try {
            this->sdm = modelFactory->loadSDM(pathStr);
            qDebug() << "SD model loaded successfully";
        } catch (const std::exception &e) {
            qCritical() << "Failed to load SD model:" << e.what();
            this->sdm = nullptr;
        }
        sdWorkerThread->quit();
    });
    
    QObject::connect(sdWorkerThread, &QThread::finished, [this]() {
        sdWorkerThread->deleteLater();
        sdWorkerThread = nullptr;
    });

    sdWorkerThread->start();
}

void UIHandler::prompt(const QString &message) {
    qDebug() << "Generating response to:" << message;

    if (workerThread && workerThread->isRunning()) {
        qWarning() << "Already processing a prompt";
        return;
    }

    workerThread = new QThread();

    QObject::connect(workerThread, &QThread::started, [this, message]() {
        QMutexLocker locker(&llmMutex);

        if (!llm) {
            qWarning() << "LLM model not loaded";
            return;
        }

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
    qDebug() << "Generating image for:" << prompt;
    
    // Check if model is loaded
    if (!sdm) {
        qWarning() << "SD model not loaded";
        return;
    }

    // Check if already generating
    if (sdWorkerThread && sdWorkerThread->isRunning()) {
        qWarning() << "Already generating an image";
        return;
    }

    // Run image generation on a worker thread
    sdWorkerThread = new QThread();

    QObject::connect(sdWorkerThread, &QThread::started, [this, prompt]() {
        QMutexLocker locker(&sdMutex);
        try {
            if (!sdm) {
                qWarning() << "SD model disappeared during generation";
                return;
            }

            sd_image_t image = sdm->generateImage(prompt.toStdString());
            
            if (!image.data) {
                qWarning() << "Failed to generate image";
                return;
            }

            // Convert to QImage (this is thread-safe)
            QImage qimage = convertToQImage(image);
            
            // Emit signal to display in QML (Qt handles thread-safe signal emission)
            emit imageGenerated(qimage);
            
            // Save as PNG
            saveImageAsPNG(image, (prompt + "test.png").toStdString());
            
            qDebug() << "Image generation completed successfully";
        } catch (const std::exception &e) {
            qCritical() << "Image generation failed:" << e.what();
        }
        sdWorkerThread->quit();
    });

    QObject::connect(sdWorkerThread, &QThread::finished, [this]() {
        sdWorkerThread->deleteLater();
        sdWorkerThread = nullptr;
    });

    sdWorkerThread->start();
}
