#ifndef INPUTHANDLER_H
#define INPUTHANDLER_H

#include <QObject>
#include <QJSEngine>


#include "../src/modelfactory.hpp"

class InputHandler : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)

public:
    explicit InputHandler(QObject *parent = nullptr);

    std::unique_ptr<ModelFactory> modelFactory;

    std::unique_ptr<LLModel> llm;

    void callJsFunction(const QJSValue &function, const QVariantList &args = {});

public slots:
    void handleButtonClick();
    void handleButtonClickWithParam(const QString &message);

    void loadModel(const QString &path);

    void prompt(const QString &message, const QJSValue &tokenCallback);

signals:
    void responseSent(const QString &response);
};

#endif // INPUTHANDLER_H
