#ifndef INPUTHANDLER_H
#define INPUTHANDLER_H

#include <QObject>
#include <llama-cpp.h>

class InputHandler : public QObject
{
    Q_OBJECT
public:
    explicit InputHandler(QObject *parent = nullptr);

    llama_model_ptr model;

public slots:
    void handleButtonClick();
    void handleButtonClickWithParam(const QString &message);

    void loadModel(const QString &path);

    void prompt(const QString &message);

signals:
    void responseSent(const QString &response);
};

#endif // INPUTHANDLER_H
