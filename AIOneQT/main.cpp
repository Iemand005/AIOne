#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickImageProvider>
#include <QImage>

#include "UIHandler.h"

class GeneratedImageProvider : public QQuickImageProvider {
public:
    GeneratedImageProvider() : QQuickImageProvider(QQuickImageProvider::Pixmap) {}
    
    QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize) override {
        Q_UNUSED(id);
        Q_UNUSED(requestedSize);
        
        if (size) {
            *size = lastImage.size();
        }
        return QPixmap::fromImage(lastImage);
    }
    
    QImage lastImage;
};

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
    
    GeneratedImageProvider *imageProvider = new GeneratedImageProvider();
    engine.addImageProvider(QLatin1String("generated"), imageProvider);

    qmlRegisterType<UIHandler>("AI", 1, 0, "InputHandler");

    UIHandler inputHandler;
    
    QObject::connect(&inputHandler, &UIHandler::imageGenerated, [imageProvider](const QImage &image) {
        imageProvider->lastImage = image;
    });

    engine.rootContext()->setContextProperty("inputHandler", &inputHandler);
    
    engine.loadFromModule("AIOneQT", "Main");

    return app.exec();
}
