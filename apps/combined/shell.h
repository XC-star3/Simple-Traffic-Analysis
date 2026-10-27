#pragma once

#include <QMainWindow>

namespace ef {
class EfCommandBar;
class EfStatusStrip;
}  // namespace ef

class QStackedWidget;
class QWidget;
class TruckWindow;
class BikeWindow;

// 单窗口任务台：0 为任务选择，1 为题 2，2 为题 7。
// 两个任务页保持存活，切换时只暂停各自的定时器，数据不丢。
class ShellWindow : public QMainWindow {
public:
    explicit ShellWindow(QWidget* parent = nullptr);

    void showTask(int index);
    int currentTask() const { return m_current; }
    void runDemoForCurrent();
    void runWarningDemo();
    void runLetWayDemo();

private:
    QWidget* buildLauncher();
    void configureTaskBar(ef::EfCommandBar* bar);
    void suspendCurrent();
    void updateThemeButtons();
    void toggleTheme();

    QStackedWidget* m_stack = nullptr;
    QWidget* m_launcher = nullptr;
    ef::EfCommandBar* m_launcherBar = nullptr;
    ef::EfStatusStrip* m_launcherStatus = nullptr;
    TruckWindow* m_truck = nullptr;
    BikeWindow* m_bike = nullptr;
    int m_current = -1;
};
