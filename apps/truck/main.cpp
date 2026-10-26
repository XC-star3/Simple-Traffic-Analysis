#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QImage>
#include <QStringList>
#include <QTimer>
#include <QWidget>

#include "common/theme.h"
#include "main_window.h"

namespace {

void saveScreenshot(QWidget& window, const QString& path) {
    QImage image(window.size(), QImage::Format_ARGB32);
    image.fill(Qt::white);
    window.render(&image);

    const QFileInfo info(path);
    if (!info.absolutePath().isEmpty()) {
        QDir().mkpath(info.absolutePath());
    }
    const bool ok = info.suffix().isEmpty() ? image.save(path, "BMP") : image.save(path);
    QFile logFile{QStringLiteral("screenshot_log.txt")};
    if (logFile.open(QIODevice::Append)) {
        logFile.write(QString("path=%1 ok=%2 size=%3x%4\n")
                          .arg(path)
                          .arg(ok)
                          .arg(image.width())
                          .arg(image.height())
                          .toUtf8());
    }
}

}  // namespace

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    app.setApplicationName("快递中转场调度台");

    const QStringList args = app.arguments();
    ef::setDark(args.contains("--dark"));
    ef::apply(app);

    MainWindow window;
    const int sizeIndex = args.indexOf("--size");
    if (sizeIndex >= 0 && sizeIndex + 1 < args.size()) {
        const QStringList parts = args.at(sizeIndex + 1).split(QLatin1Char('x'),
                                                              Qt::SkipEmptyParts);
        if (parts.size() == 2) {
            bool okWidth = false;
            bool okHeight = false;
            const int width = parts[0].toInt(&okWidth);
            const int height = parts[1].toInt(&okHeight);
            if (okWidth && okHeight && width > 0 && height > 0) {
                window.resize(width, height);
            }
        }
    }
    window.show();

    if (args.contains("--demo")) {
        QTimer::singleShot(150, &app, [&window]() { window.runDemo(); });
    }
    if (args.contains("--smoke")) {
        QTimer::singleShot(900, &app, []() { QCoreApplication::quit(); });
    }
    const int shotIndex = args.indexOf("--screenshot");
    if (shotIndex >= 0 && shotIndex + 1 < args.size()) {
        const QString path = args[shotIndex + 1];
        QTimer::singleShot(800, &app, [&window, path]() { saveScreenshot(window, path); });
        QTimer::singleShot(1100, &app, []() { QCoreApplication::quit(); });
    }
    return app.exec();
}
