#include "shell.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QLabel>
#include <QPushButton>
#include <QShortcut>
#include <QStackedWidget>
#include <QTimer>
#include <QVBoxLayout>

#include <functional>

#include "bike/main_window.h"
#include "common/theme.h"
#include "common/widgets.h"
#include "truck/main_window.h"

namespace {

QWidget* makeTaskCard(QWidget* parent, const QString& index, const QString& tag,
                      const QString& title, const QString& subtitle,
                      const QString& description, const QStringList& bullets,
                      std::function<void()> open) {
    auto* card = new ef::EfPanel(QString("TASK %1").arg(index), title, parent);
    card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    card->setMaximumHeight(430);

    auto* number = new QLabel(index, card);
    number->setObjectName(QStringLiteral("efNumeral"));
    number->setFocusPolicy(Qt::NoFocus);

    auto* tagLabel = new QLabel(tag, card);
    tagLabel->setObjectName(QStringLiteral("efEyebrow"));
    tagLabel->setFont(ef::font(ef::FontRole::Eyebrow));
    tagLabel->setFocusPolicy(Qt::NoFocus);

    auto* subtitleLabel = new QLabel(subtitle, card);
    QFont subtitleFont = ef::font(ef::FontRole::Label);
    subtitleFont.setPixelSize(14);
    subtitleFont.setWeight(QFont::Bold);
    subtitleLabel->setFont(subtitleFont);
    subtitleLabel->setFocusPolicy(Qt::NoFocus);

    auto* descriptionLabel = new QLabel(description, card);
    descriptionLabel->setObjectName(QStringLiteral("efHint"));
    descriptionLabel->setWordWrap(true);
    descriptionLabel->setFocusPolicy(Qt::NoFocus);

    auto* openButton = new ef::EfButton(QStringLiteral("进入任务"),
                                        ef::EfButton::Variant::Primary, card);
    openButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    openButton->setAccessibleName(QString("进入%1").arg(title));
    QObject::connect(openButton, &QPushButton::clicked, card, [open]() {
        if (open) {
            open();
        }
    });

    card->body()->addWidget(number);
    card->body()->addWidget(tagLabel);
    card->body()->addWidget(subtitleLabel);
    card->body()->addSpacing(4);
    card->body()->addWidget(descriptionLabel);
    card->body()->addSpacing(6);
    for (const QString& bullet : bullets) {
        auto* line = new QLabel(QString("—  %1").arg(bullet), card);
        line->setObjectName(QStringLiteral("efHint"));
        line->setWordWrap(true);
        line->setFocusPolicy(Qt::NoFocus);
        card->body()->addWidget(line);
    }
    card->body()->addStretch(1);
    card->body()->addWidget(openButton);
    card->setAccessibleName(QString("%1 %2").arg(index, title));
    card->setAccessibleDescription(description);
    return card;
}

}  // namespace

ShellWindow::ShellWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle(QStringLiteral("数据结构课程设计 / 任务选择"));
    resize(1440, 900);
    setMinimumSize(1200, 760);

    m_stack = new QStackedWidget(this);
    setCentralWidget(m_stack);

    m_launcher = buildLauncher();
    m_truck = new TruckWindow(m_stack);
    m_bike = new BikeWindow(m_stack);

    m_stack->addWidget(m_launcher);
    m_stack->addWidget(m_truck);
    m_stack->addWidget(m_bike);

    configureTaskBar(m_truck->commandBar());
    configureTaskBar(m_bike->commandBar());

    connect(m_launcherBar->themeButton(), &QPushButton::clicked, this,
            [this]() { toggleTheme(); });

    auto* toLauncher = new QShortcut(QKeySequence(QStringLiteral("Ctrl+0")), this);
    connect(toLauncher, &QShortcut::activated, this, [this]() { showTask(0); });
    auto* toTruck = new QShortcut(QKeySequence(QStringLiteral("Ctrl+1")), this);
    connect(toTruck, &QShortcut::activated, this, [this]() { showTask(1); });
    auto* toBike = new QShortcut(QKeySequence(QStringLiteral("Ctrl+2")), this);
    connect(toBike, &QShortcut::activated, this, [this]() { showTask(2); });
    auto* escape = new QShortcut(QKeySequence(QStringLiteral("Esc")), this);
    connect(escape, &QShortcut::activated, this, [this]() { showTask(0); });

    showTask(0);
    // 不要在启动时把焦点落在右上角的“深色”按钮上。
    QTimer::singleShot(0, this, [this]() {
        if (m_current == 0) {
            m_launcher->setFocus(Qt::OtherFocusReason);
        }
    });
}

QWidget* ShellWindow::buildLauncher() {
    auto* page = new QWidget;
    auto* root = new QVBoxLayout(page);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    root->addWidget(new ef::EfTriStrip(page));
    m_launcherBar = new ef::EfCommandBar(QStringLiteral("D"), QStringLiteral("DSA"),
                                         QStringLiteral("数据结构课程设计"),
                                         QStringLiteral("TASKS"),
                                         QStringLiteral("任务选择"), page);
    m_launcherBar->setFields(ef::fields({QStringLiteral("2 个任务"),
                                         QStringLiteral("1 个窗口")}));
    root->addWidget(m_launcherBar);

    auto* body = new QWidget(page);
    auto* layout = new QVBoxLayout(body);
    layout->setContentsMargins(36, 24, 36, 16);
    layout->setSpacing(10);

    layout->addWidget(ef::makeEyebrow(QStringLiteral("// SELECT A TASK"), body));

    auto* title = new QLabel(QStringLiteral("选择任务"), body);
    title->setObjectName(QStringLiteral("efDisplay"));
    title->setFocusPolicy(Qt::NoFocus);
    layout->addWidget(title);

    auto* subtitle = new QLabel(
        QStringLiteral("两个题目共用一套控制台。进入任务后，用顶部的“任务”按钮或 "
                       "Ctrl+1 / Ctrl+2 切换；已录入的数据会保留。"),
        body);
    subtitle->setObjectName(QStringLiteral("efHint"));
    subtitle->setWordWrap(true);
    layout->addWidget(subtitle);
    layout->addSpacing(8);

    auto* cards = new QHBoxLayout;
    cards->setContentsMargins(0, 0, 0, 0);
    cards->setSpacing(16);
    cards->addWidget(
        makeTaskCard(body, QStringLiteral("01"), QStringLiteral("// TRUCK / STACK + QUEUE"),
                     QStringLiteral("快递中转场"),
                     QStringLiteral("栈 + 多优先级队列混合调度"),
                     QStringLiteral("主装卸通道是栈，场外三条队列按生鲜 > 普通 > 大件补位；"
                                    "支持到达排队、让路离场、计费和临时区告警。"),
                     {QStringLiteral("主通道栈 + 临时等候区栈"),
                      QStringLiteral("生鲜 > 普通 > 大件，同级先到先服务"),
                      QStringLiteral("临时区不足：拒绝离场并弹出告警条")},
                     [this]() { showTask(1); }),
        1);
    cards->addWidget(
        makeTaskCard(body, QStringLiteral("02"), QStringLiteral("// BIKE / GRAPH + DIJKSTRA"),
                     QStringLiteral("公共自行车调度"),
                     QStringLiteral("无向带权图 + 最短路"),
                     QStringLiteral("站点与道路构成无向带权图；支持两种权重的最短路、"
                                    "目标集合路线规划、调度仿真和道路临时阻断。"),
                     {QStringLiteral("Dijkstra：按距离 / 按耗时"),
                      QStringLiteral("目标集合：最近邻 + 2-opt"),
                      QStringLiteral("道路阻断后重新检查目标可达性")},
                     [this]() { showTask(2); }),
        1);
    layout->addLayout(cards, 0);
    layout->addStretch(1);

    auto* hint = ef::makeHint(
        QStringLiteral("Ctrl+0 返回任务选择   ·   Ctrl+1 快递中转场   ·   "
                       "Ctrl+2 公共自行车"),
        body);
    layout->addWidget(hint);

    root->addWidget(body, 1);

    m_launcherStatus = new ef::EfStatusStrip(page);
    root->addWidget(m_launcherStatus);
    m_launcherStatus->showMessage(QStringLiteral("就绪：请选择一个任务"), 0);
    page->setFocusPolicy(Qt::StrongFocus);
    return page;
}

void ShellWindow::configureTaskBar(ef::EfCommandBar* bar) {
    if (bar == nullptr) {
        return;
    }
    bar->setTaskButtonVisible(true);
    connect(bar->taskButton(), &QPushButton::clicked, this, [this]() { showTask(0); });
    connect(bar->themeButton(), &QPushButton::clicked, this, [this]() {
        // 页面自己的 toggleTheme 已经同步执行；这里把另一页和任务选择页补齐。
        QTimer::singleShot(0, this, [this]() {
            updateThemeButtons();
            if (m_truck != nullptr) {
                m_truck->refreshTheme();
            }
            if (m_bike != nullptr) {
                m_bike->refreshTheme();
            }
            if (m_launcher != nullptr) {
                m_launcher->update();
            }
        });
    });
}

void ShellWindow::showTask(int index) {
    if (m_stack == nullptr || index < 0 || index > 2) {
        return;
    }
    if (m_current == index && m_stack->currentIndex() == index) {
        return;
    }

    suspendCurrent();
    m_current = index;
    m_stack->setCurrentIndex(index);

    switch (index) {
        case 0:
            setWindowTitle(QStringLiteral("数据结构课程设计 / 任务选择"));
            if (m_launcherStatus != nullptr) {
                m_launcherStatus->showMessage(QStringLiteral("就绪：请选择一个任务"), 0);
            }
            m_launcher->setFocus(Qt::OtherFocusReason);
            break;
        case 1:
            setWindowTitle(QStringLiteral("快递中转场 / 调度台"));
            m_truck->setFocus();
            break;
        case 2:
            setWindowTitle(QStringLiteral("城市公共自行车 / 调度台"));
            m_bike->setFocus();
            break;
    }
    updateThemeButtons();
}

void ShellWindow::suspendCurrent() {
    if (m_current == 1 && m_truck != nullptr) {
        m_truck->suspend();
    } else if (m_current == 2 && m_bike != nullptr) {
        m_bike->suspend();
    }
}

void ShellWindow::updateThemeButtons() {
    const QString text = ef::isDark() ? QStringLiteral("浅色") : QStringLiteral("深色");
    if (m_launcherBar != nullptr) {
        m_launcherBar->themeButton()->setText(text);
    }
    if (m_truck != nullptr) {
        m_truck->commandBar()->themeButton()->setText(text);
    }
    if (m_bike != nullptr) {
        m_bike->commandBar()->themeButton()->setText(text);
    }
}

void ShellWindow::toggleTheme() {
    ef::setDark(!ef::isDark());
    ef::apply(*qApp);
    if (m_truck != nullptr) {
        m_truck->refreshTheme();
    }
    if (m_bike != nullptr) {
        m_bike->refreshTheme();
    }
    if (m_launcher != nullptr) {
        m_launcher->update();
    }
    updateThemeButtons();
    if (m_launcherStatus != nullptr) {
        m_launcherStatus->showMessage(ef::isDark() ? QStringLiteral("已切换到深色主题")
                                                   : QStringLiteral("已切换到浅色主题"));
    }
}

void ShellWindow::runDemoForCurrent() {
    if (m_current == 2) {
        m_bike->runDemo();
        return;
    }
    if (m_current != 1) {
        showTask(1);
    }
    m_truck->runDemo();
}

void ShellWindow::runWarningDemo() {
    showTask(1);
    m_truck->runWarningDemo();
}

void ShellWindow::runLetWayDemo() {
    showTask(1);
    m_truck->runLetWayDemo();
}
