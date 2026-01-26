#ifndef INPUTHANDLER_H
#define INPUTHANDLER_H

#include <QObject>
#include "../src/modelfactory.hpp"

class InputHandler : public QObject
{
    Q_OBJECT
public:
    explicit InputHandler(QObject *parent = nullptr);

    std::unique_ptr<ModelFactory> modelFactory;

    std::unique_ptr<LLModel> llm;

public slots:
    void handleButtonClick();
    void handleButtonClickWithParam(const QString &message);

    void loadModel(const QString &path);

    void prompt(const QString &message);

signals:
    void responseSent(const QString &response);
};

#endif // INPUTHANDLER_H
