#include <QApplication>
#include <QFile>
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
    const bool ok = image.save(path, "BMP");
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
    app.setApplicationName("快递中转栈-队列混合调度模拟");
    applyAppTheme(app);
    app.setFont(QFont("Microsoft YaHei UI", 10));

    MainWindow window;
    window.show();

    const QStringList args = app.arguments();
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
