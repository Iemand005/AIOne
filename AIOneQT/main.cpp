#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "inputhandler.h"



int main(int argc, char *argv[])
{
    qputenv("QT_IM_MODULE", QByteArray("qtvirtualkeyboard"));

    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("AIOneQT", "Main");

    qmlRegisterType<UIHandler>("AI", 1, 0, "InputHandler");

    UIHandler inputHandler;

    engine.rootContext()->setContextProperty("inputHandler", &inputHandler);

    return app.exec();
}
