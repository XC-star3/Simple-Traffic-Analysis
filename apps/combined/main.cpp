#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QStringList>
#include <QTimer>

#include "common/theme.h"
#include "shell.h"

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

int taskFromArguments(const QStringList& args) {
    int task = 0;
    const int index = args.indexOf("--task");
    if (index >= 0 && index + 1 < args.size()) {
        const QString value = args.at(index + 1).trimmed().toLower();
        if (value == QStringLiteral("truck") || value == QStringLiteral("2")) {
            task = 1;
        } else if (value == QStringLiteral("bike") || value == QStringLiteral("7")) {
            task = 2;
        }
    }
    if (args.contains(QStringLiteral("--demo-warning")) ||
        args.contains(QStringLiteral("--demo-letway"))) {
        task = 1;
    } else if (task == 0 && args.contains(QStringLiteral("--demo"))) {
        task = 1;
    }
    return task;
}

}  // namespace

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("数据结构课程设计任务台"));

    const QStringList args = app.arguments();
    ef::setDark(args.contains(QStringLiteral("--dark")));
    ef::apply(app);

    ShellWindow window;
    const int sizeIndex = args.indexOf(QStringLiteral("--size"));
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

    window.showTask(taskFromArguments(args));
    window.show();

    if (args.contains(QStringLiteral("--demo-warning"))) {
        QTimer::singleShot(250, &app, [&window]() { window.runWarningDemo(); });
    } else if (args.contains(QStringLiteral("--demo-letway"))) {
        QTimer::singleShot(250, &app, [&window]() { window.runLetWayDemo(); });
    } else if (args.contains(QStringLiteral("--demo"))) {
        QTimer::singleShot(250, &app, [&window]() { window.runDemoForCurrent(); });
    }
    if (args.contains(QStringLiteral("--smoke"))) {
        QTimer::singleShot(1200, &app, []() { QCoreApplication::quit(); });
    }
    const int shotIndex = args.indexOf(QStringLiteral("--screenshot"));
    if (shotIndex >= 0 && shotIndex + 1 < args.size()) {
        const QString path = args[shotIndex + 1];
        QTimer::singleShot(1400, &app, [&window, path]() { saveScreenshot(window, path); });
        QTimer::singleShot(1700, &app, []() { QCoreApplication::quit(); });
    }
    return app.exec();
}
