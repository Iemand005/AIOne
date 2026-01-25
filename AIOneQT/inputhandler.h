#ifndef INPUTHANDLER_H
#define INPUTHANDLER_H

#include <QObject>

class InputHandler : public QObject
{
    Q_OBJECT
public:
    explicit InputHandler(QObject *parent = nullptr);
public slots:
    void handleButtonClick();
    void handleButtonClickWithParam(const QString &message);

signals:
    void responseSent(const QString &response);
};

#endif // INPUTHANDLER_H
