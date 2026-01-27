#ifndef INPUTHANDLER_H
#define INPUTHANDLER_H

#include <QObject>
#include <QString>
#include <QThread>
#include <QMutex>

#include "../src/ModelFactory.hpp"

class UIHandler : public QObject
{
    Q_OBJECT

public:
    explicit UIHandler(QObject *parent = nullptr);

    std::unique_ptr<ModelFactory> modelFactory;

    std::unique_ptr<LLModel> llm;
    std::unique_ptr<SDModel> sdm;

public slots:
    void handleButtonClick();
    void handleButtonClickWithParam(const QString &message);

    void loadModel(const QString &path);
    void loadSDModel(const QString &path);

    void prompt(const QString &message);
    void generateImage(const QString &prompt);

signals:
    void responseSent(const QString &response);
    void tokenReceived(const QString &token);
    void imageGenerated(const QImage &image);

private:
    QThread *workerThread = nullptr;
    QThread *llmWorkerThread = nullptr;
    QThread *sdWorkerThread = nullptr;
    QMutex mutex;
    QMutex llmMutex;
    QMutex sdMutex;
};

#endif // INPUTHANDLER_H
