#include "main_window.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFont>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QRandomGenerator>
#include <QScrollArea>
#include <QSpinBox>
#include <QStatusBar>
#include <QTimer>
#include <QVBoxLayout>
#include <QVector>

#include "common/theme.h"
#include "common/widgets.h"
#include "yard_canvas.h"

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    buildUi();
    applyConfig();
}

void MainWindow::runDemo() {
    applyConfig();
    m_yard.arrive(1001, ds::CargoType::Fresh, 0.0);
    m_yard.arrive(1002, ds::CargoType::Normal, 1.0);
    m_yard.arrive(1003, ds::CargoType::Bulk, 2.0);
    m_yard.arrive(1004, ds::CargoType::Fresh, 3.0);
    m_yard.arrive(1005, ds::CargoType::Normal, 4.0);
    m_yard.arrive(1006, ds::CargoType::Bulk, 5.0);
    m_yard.arrive(1007, ds::CargoType::Fresh, 6.0);
    m_yard.arrive(1008, ds::CargoType::Normal, 7.0);
    m_yard.arrive(1009, ds::CargoType::Bulk, 8.0);
    m_yard.arrive(1010, ds::CargoType::Fresh, 9.0);

    m_now = 12.0;
    double fee = 0.0;
    char err[128] = {0};
    const bool departed =
        m_yard.depart(1002, m_now, &fee, nullptr, err, sizeof(err));
    m_nowSpin->setValue(m_now);
    m_nextId = 1011;
    m_arriveId->setValue(m_nextId);
    m_departId->setValue(1003);
    refreshAll();
    setStatus(departed
                  ? QString("演示数据：10 辆车已到达，T1002 让路离场，费用 %1 元")
                        .arg(fee, 0, 'f', 2)
                  : QString("演示数据已装载，但 T1002 离场失败：%1").arg(QString::fromUtf8(err)));
}

void MainWindow::buildUi() {
    setWindowTitle("快递中转场 / 调度台");
    resize(1440, 900);
    setMinimumSize(1200, 760);

    auto* central = new QWidget(this);
    setCentralWidget(central);
    auto* root = new QVBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    root->addWidget(new ef::EfTriStrip(central));
    m_bar = new ef::EfCommandBar(QStringLiteral("T"), QStringLiteral("TRANSIT"),
                                 QStringLiteral("快递中转场"), QStringLiteral("LIVE"),
                                 QStringLiteral("运行态势"), central);
    root->addWidget(m_bar);

    auto* content = new QWidget(central);
    auto* columns = new QHBoxLayout(content);
    columns->setContentsMargins(14, 14, 14, 14);
    columns->setSpacing(12);
    root->addWidget(content, 1);

    // ---------------- 左侧控制区 ----------------
    auto* left = new QWidget;
    auto* leftLayout = new QVBoxLayout(left);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(8);

    auto* config = new ef::EfPanel(QStringLiteral("PARAMETERS"), QStringLiteral("场地参数"));
    auto* configForm = new QFormLayout;
    configForm->setContentsMargins(0, 0, 0, 0);
    configForm->setHorizontalSpacing(10);
    configForm->setVerticalSpacing(6);
    m_capacity = new QSpinBox;
    m_capacity->setRange(1, 12);
    m_capacity->setValue(8);
    m_capacity->setSuffix(QStringLiteral(" 辆"));
    m_tempCapacity = new QSpinBox;
    m_tempCapacity->setRange(0, 12);
    m_tempCapacity->setValue(7);
    m_tempCapacity->setSuffix(QStringLiteral(" 辆"));
    m_fee = new QDoubleSpinBox;
    m_fee->setRange(0.0, 10000.0);
    m_fee->setDecimals(2);
    m_fee->setValue(5.0);
    m_fee->setSuffix(QStringLiteral(" 元/小时"));
    configForm->addRow(QStringLiteral("主通道容量"), m_capacity);
    configForm->addRow(QStringLiteral("临时区容量"), m_tempCapacity);
    configForm->addRow(QStringLiteral("停车费率"), m_fee);
    config->body()->addLayout(configForm);
    auto* applyButton =
        new ef::EfButton(QStringLiteral("应用参数并重置"), ef::EfButton::Variant::Secondary);
    applyButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    config->body()->addWidget(applyButton);
    leftLayout->addWidget(config);

    auto* arrive = new ef::EfPanel(QStringLiteral("ARRIVAL"), QStringLiteral("货车到达"));
    auto* arriveForm = new QFormLayout;
    arriveForm->setContentsMargins(0, 0, 0, 0);
    arriveForm->setHorizontalSpacing(10);
    arriveForm->setVerticalSpacing(6);
    m_arriveId = new QSpinBox;
    m_arriveId->setRange(1, 999999);
    m_arriveId->setValue(1001);
    m_type = new QComboBox;
    m_type->addItem(QStringLiteral("生鲜件（优先级最高）"));
    m_type->addItem(QStringLiteral("普通件"));
    m_type->addItem(QStringLiteral("大件货（优先级最低）"));
    arriveForm->addRow(QStringLiteral("货车编号"), m_arriveId);
    arriveForm->addRow(QStringLiteral("货物类型"), m_type);
    arrive->body()->addLayout(arriveForm);
    auto* arriveButton =
        new ef::EfButton(QStringLiteral("到达并排队"), ef::EfButton::Variant::Primary);
    arriveButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    arrive->body()->addWidget(arriveButton);
    arrive->body()->addWidget(
        ef::makeHint(QStringLiteral("到达时刻取仿真时钟里的当前时刻。"), arrive));
    leftLayout->addWidget(arrive);

    auto* depart = new ef::EfPanel(QStringLiteral("DEPARTURE"), QStringLiteral("货车离场"));
    auto* departForm = new QFormLayout;
    departForm->setContentsMargins(0, 0, 0, 0);
    departForm->setHorizontalSpacing(10);
    departForm->setVerticalSpacing(6);
    m_departId = new QSpinBox;
    m_departId->setRange(1, 999999);
    m_departId->setValue(1001);
    departForm->addRow(QStringLiteral("货车编号"), m_departId);
    depart->body()->addLayout(departForm);
    auto* departButton = new ef::EfButton(QStringLiteral("离场（自动让路）"),
                                          ef::EfButton::Variant::Primary);
    departButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    depart->body()->addWidget(departButton);
    leftLayout->addWidget(depart);
    leftLayout->addStretch(1);

    auto* leftScroll = new QScrollArea;
    leftScroll->setWidgetResizable(true);
    leftScroll->setFrameShape(QFrame::NoFrame);
    leftScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    leftScroll->setWidget(left);
    leftScroll->setFixedWidth(300);
    columns->addWidget(leftScroll);

    // ---------------- 中间运行态势 ----------------
    auto* center = new QWidget;
    center->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto* centerLayout = new QVBoxLayout(center);
    centerLayout->setContentsMargins(0, 0, 0, 0);
    centerLayout->setSpacing(10);
    auto* yardPanel = new ef::EfPanel(QStringLiteral("LIVE YARD"), QStringLiteral("运行态势"));
    m_canvas = new YardCanvas(yardPanel);
    m_canvas->setYard(&m_yard);
    m_canvas->setAccessibleName(QStringLiteral("主通道、临时区与优先级队列示意图"));
    m_canvas->setMinimumSize(480, 438);
    yardPanel->body()->setContentsMargins(0, 0, 0, 0);
    yardPanel->body()->addWidget(m_canvas, 1);
    centerLayout->addWidget(yardPanel, 1);

    auto* simBar = new ef::EfPanel(QStringLiteral("SIMULATION"), QStringLiteral("仿真时钟"));
    auto* simRows = new QVBoxLayout;
    simRows->setContentsMargins(0, 0, 0, 0);
    simRows->setSpacing(6);
    auto* fieldRow = new QHBoxLayout;
    fieldRow->setContentsMargins(0, 0, 0, 0);
    fieldRow->setSpacing(8);
    m_nowSpin = new QDoubleSpinBox;
    m_nowSpin->setRange(0.0, 100000.0);
    m_nowSpin->setDecimals(1);
    m_nowSpin->setValue(0.0);
    m_nowSpin->setSuffix(QStringLiteral(" 分钟"));
    m_nowSpin->setFixedWidth(112);
    m_interval = new QSpinBox;
    m_interval->setRange(200, 3000);
    m_interval->setSingleStep(100);
    m_interval->setValue(700);
    m_interval->setSuffix(QStringLiteral(" ms"));
    m_interval->setFixedWidth(92);
    fieldRow->addWidget(ef::makeField(QStringLiteral("当前时刻"), simBar));
    fieldRow->addWidget(m_nowSpin);
    fieldRow->addWidget(ef::makeField(QStringLiteral("事件间隔"), simBar));
    fieldRow->addWidget(m_interval);
    fieldRow->addStretch(1);
    simRows->addLayout(fieldRow);
    auto* actionRow = new QHBoxLayout;
    actionRow->setContentsMargins(0, 0, 0, 0);
    actionRow->setSpacing(6);
    m_autoButton =
        new ef::EfButton(QStringLiteral("开始自动模拟"), ef::EfButton::Variant::Primary);
    auto* stepButton =
        new ef::EfButton(QStringLiteral("单步推进 2 分钟"), ef::EfButton::Variant::Secondary);
    auto* resetButton =
        new ef::EfButton(QStringLiteral("重置系统"), ef::EfButton::Variant::Secondary);
    m_autoButton->setFixedWidth(126);
    stepButton->setFixedWidth(142);
    resetButton->setFixedWidth(92);
    actionRow->addWidget(m_autoButton);
    actionRow->addWidget(stepButton);
    actionRow->addWidget(resetButton);
    actionRow->addStretch(1);
    simRows->addLayout(actionRow);
    simBar->body()->addLayout(simRows);
    // 仿真条的内容有固定高度，别再被下面的画布挤压；画布自己会占用剩余空间。
    simBar->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    centerLayout->addWidget(simBar);
    columns->addWidget(center, 1);

    // ---------------- 右侧统计与日志 ----------------
    auto* right = new QWidget;
    right->setFixedWidth(360);
    auto* rightLayout = new QVBoxLayout(right);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(10);

    auto* metricsPanel =
        new ef::EfPanel(QStringLiteral("METRICS"), QStringLiteral("统计"));
    m_metrics = new ef::EfMetrics(3, metricsPanel);
    metricsPanel->body()->addWidget(m_metrics);
    m_queueFields = ef::makeField(QStringLiteral("生鲜 0  │  普通 0  │  大件 0"), metricsPanel);
    metricsPanel->body()->addWidget(m_queueFields);
    rightLayout->addWidget(metricsPanel);

    auto* logPanel = new ef::EfPanel(QStringLiteral("EVENT LOG"), QStringLiteral("事件日志"));
    logPanel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    m_log = new QListWidget(logPanel);
    m_log->setSelectionMode(QAbstractItemView::NoSelection);
    m_log->setFocusPolicy(Qt::NoFocus);
    m_log->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_log->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_log->setUniformItemSizes(true);
    m_log->setWordWrap(false);
    m_log->setFont(ef::font(ef::FontRole::Field));
    m_log->setItemDelegate(new ef::EfListDelegate(m_log));
    m_log->setAccessibleName(QStringLiteral("事件日志"));
    m_log->setMinimumHeight(240);
    logPanel->body()->addWidget(m_log, 1);
    rightLayout->addWidget(logPanel, 1);
    columns->addWidget(right);

    // ---------------- 连接 ----------------
    connect(applyButton, &QPushButton::clicked, this, [this]() { applyConfig(); });
    connect(arriveButton, &QPushButton::clicked, this, [this]() { doArrive(); });
    connect(departButton, &QPushButton::clicked, this, [this]() { doDepart(); });
    connect(m_autoButton, &QPushButton::clicked, this, [this]() { toggleAuto(); });
    connect(stepButton, &QPushButton::clicked, this, [this]() { autoTick(false); });
    connect(resetButton, &QPushButton::clicked, this, [this]() { resetAll(); });
    connect(m_interval, &QSpinBox::valueChanged, this, [this](int value) {
        if (m_autoTimer->isActive()) {
            m_autoTimer->setInterval(value);
        }
    });
    connect(m_bar->themeButton(), &QPushButton::clicked, this, [this]() { toggleTheme(); });

    m_autoTimer = new QTimer(this);
    connect(m_autoTimer, &QTimer::timeout, this, [this]() { autoTick(true); });

    statusBar()->setSizeGripEnabled(false);
    auto* ready = new QLabel(QStringLiteral("// READY"), this);
    ready->setObjectName(QStringLiteral("efEyebrow"));
    ready->setFont(ef::font(ef::FontRole::Eyebrow));
    statusBar()->addPermanentWidget(ready);
    statusBar()->showMessage(QStringLiteral("就绪"));

    // 启动时不要把焦点落在命令栏的“深色”按钮上；让容器持有焦点，
    // 用户按 Tab 后再进入第一个输入控件。
    central->setFocusPolicy(Qt::StrongFocus);
    central->setFocus(Qt::OtherFocusReason);
}

void MainWindow::applyConfig() {
    m_autoTimer->stop();
    m_autoButton->setText(QStringLiteral("开始自动模拟"));
    if (!m_yard.configure(m_capacity->value(), m_tempCapacity->value(), m_fee->value())) {
        setStatus(QStringLiteral("参数非法，请检查容量与费率"));
        return;
    }
    m_now = 0.0;
    m_nextId = 1001;
    m_arriveId->setValue(m_nextId);
    m_departId->setValue(m_nextId);
    m_nowSpin->setValue(0.0);
    m_lastEventSeq = -1;
    m_log->clear();
    m_canvas->resetAnimation();
    refreshAll();
    setStatus(QStringLiteral("参数已应用，系统已重置"));
}

void MainWindow::doArrive() {
    if (!m_yard.configured()) {
        setStatus(QStringLiteral("系统尚未初始化"));
        return;
    }
    const int id = m_arriveId->value();
    const auto type = static_cast<ds::CargoType>(m_type->currentIndex());
    const double t = m_nowSpin->value();
    char err[128] = {0};
    if (!m_yard.arrive(id, type, t, err, sizeof(err))) {
        setStatus(QString::fromUtf8(err));
        return;
    }
    if (id >= m_nextId) {
        m_nextId = id + 1;
    }
    m_arriveId->setValue(m_nextId);
    m_departId->setValue(id);
    if (t > m_now) {
        m_now = t;
    }
    refreshAll();
    setStatus(QString("货车 T%1 到达，当前时刻 %2 分钟").arg(id).arg(m_now, 0, 'f', 1));
}

void MainWindow::doDepart() {
    if (!m_yard.configured()) {
        setStatus(QStringLiteral("系统尚未初始化"));
        return;
    }
    const int id = m_departId->value();
    const double t = m_nowSpin->value();
    double fee = 0.0;
    double dwell = 0.0;
    char err[128] = {0};
    if (!m_yard.depart(id, t, &fee, &dwell, err, sizeof(err))) {
        setStatus(QString::fromUtf8(err));
        return;
    }
    if (t > m_now) {
        m_now = t;
    }
    refreshAll();
    setStatus(QString("货车 T%1 离场：停留 %2 分钟，费用 %3 元")
                  .arg(id)
                  .arg(dwell, 0, 'f', 1)
                  .arg(fee, 0, 'f', 2));
}

void MainWindow::autoTick(bool randomEvent) {
    if (!m_yard.configured()) {
        return;
    }
    m_now += 2.0;
    m_nowSpin->setValue(m_now);

    if (!randomEvent) {
        refreshAll();
        setStatus(QString("时间已推进到 %1 分钟").arg(m_now, 0, 'f', 1));
        return;
    }

    const int dice = QRandomGenerator::global()->bounded(100);
    if (dice < 65) {
        const int id = m_nextId++;
        const auto type = static_cast<ds::CargoType>(QRandomGenerator::global()->bounded(3));
        char err[128] = {0};
        m_yard.arrive(id, type, m_now, err, sizeof(err));
        m_arriveId->setValue(m_nextId);
        m_departId->setValue(id);
    } else {
        QVector<int> ids;
        const ds::Vector<ds::Truck>& trucks = m_yard.trucks();
        for (int i = 0; i < trucks.size(); ++i) {
            if (trucks[i].state == ds::TruckState::Channel) {
                ids.push_back(trucks[i].id);
            }
        }
        if (ids.isEmpty()) {
            const int id = m_nextId++;
            const auto type = static_cast<ds::CargoType>(QRandomGenerator::global()->bounded(3));
            char err[128] = {0};
            m_yard.arrive(id, type, m_now, err, sizeof(err));
            m_arriveId->setValue(m_nextId);
        } else {
            const int pick = ids[QRandomGenerator::global()->bounded(ids.size())];
            m_yard.depart(pick, m_now);
        }
    }
    refreshAll();
}

void MainWindow::toggleAuto() {
    if (m_autoTimer->isActive()) {
        m_autoTimer->stop();
        m_autoButton->setText(QStringLiteral("开始自动模拟"));
        setStatus(QStringLiteral("自动模拟已暂停"));
    } else {
        m_autoTimer->setInterval(m_interval->value());
        m_autoTimer->start();
        m_autoButton->setText(QStringLiteral("暂停自动模拟"));
        setStatus(QStringLiteral("自动模拟运行中"));
    }
}

void MainWindow::resetAll() {
    m_autoTimer->stop();
    m_autoButton->setText(QStringLiteral("开始自动模拟"));
    applyConfig();
}

void MainWindow::refreshAll() {
    if (m_canvas != nullptr) {
        m_canvas->update();
    }
    const ds::YardStats s = m_yard.stats();
    const int totalQueue = s.waiting[0] + s.waiting[1] + s.waiting[2];
    const int capacity = m_yard.capacity();
    if (m_metrics != nullptr) {
        m_metrics->setMetrics({
            {QStringLiteral("REVENUE"), QString::number(s.revenue, 'f', 2),
             QStringLiteral("元"), QStringLiteral("累计收入"), false},
            {QStringLiteral("CHANNEL"),
             QString("%1/%2").arg(s.inChannel).arg(capacity), QStringLiteral("辆"),
             QStringLiteral("主通道"), capacity > 0 && s.inChannel == capacity},
            {QStringLiteral("QUEUE"), QString::number(totalQueue), QStringLiteral("辆"),
             QStringLiteral("排队"), false},
            {QStringLiteral("TEMP"),
             QString("%1/%2").arg(s.inTemp).arg(m_yard.tempCapacity()),
             QStringLiteral("辆"), QStringLiteral("临时区"), false},
            {QStringLiteral("ARRIVED"),
             QString("%1/%2").arg(s.arrived).arg(s.served), QStringLiteral("辆"),
             QStringLiteral("到达 / 服务"), false},
            {QStringLiteral("PEAK"), QString::number(s.maxTempUsed), QStringLiteral("辆"),
             QStringLiteral("临时区峰值"), false},
        });
    }
    if (m_queueFields != nullptr) {
        m_queueFields->setText(ef::fields({
            QString("生鲜 %1").arg(s.waiting[0]),
            QString("普通 %1").arg(s.waiting[1]),
            QString("大件 %1").arg(s.waiting[2]),
            QString("空位 %1").arg(s.freeSlots),
        }));
    }
    if (m_bar != nullptr) {
        m_bar->setFields(ef::fields({
            QString("主通道 %1/%2").arg(s.inChannel).arg(capacity),
            QString("排队 %1").arg(totalQueue),
            QString("时钟 %1 分钟").arg(m_now, 0, 'f', 1),
        }));
    }
    if (m_canvas != nullptr) {
        m_canvas->setAccessibleDescription(
            QString("主通道 %1/%2，临时区 %3/%4，生鲜排队 %5，普通排队 %6，"
                    "大件排队 %7，累计收入 %8 元。")
                .arg(s.inChannel)
                .arg(capacity)
                .arg(s.inTemp)
                .arg(m_yard.tempCapacity())
                .arg(s.waiting[0])
                .arg(s.waiting[1])
                .arg(s.waiting[2])
                .arg(s.revenue, 0, 'f', 2));
    }
    syncEvents();
}

void MainWindow::syncEvents() {
    if (m_log == nullptr || m_yard.eventSeq() == m_lastEventSeq) {
        return;
    }
    m_lastEventSeq = m_yard.eventSeq();
    const ds::Vector<ds::Event>& events = m_yard.events();
    m_log->setUpdatesEnabled(false);
    m_log->clear();
    if (events.empty()) {
        auto* item = new QListWidgetItem(
            QStringLiteral("// 暂无事件。车辆到达、离场和告警会记录在这里。"));
        item->setFlags(Qt::NoItemFlags);
        item->setForeground(QBrush(ef::palette().ink2));
        m_log->addItem(item);
    } else {
        for (int i = 0; i < events.size(); ++i) {
            auto* item = new QListWidgetItem(
                QString("%1  │  %2")
                    .arg(events[i].time, 0, 'f', 1)
                    .arg(QString::fromUtf8(events[i].text)));
            item->setFont(ef::font(ef::FontRole::Field));
            item->setToolTip(item->text());
            m_log->addItem(item);
        }
    }
    m_log->setUpdatesEnabled(true);
    m_log->scrollToBottom();
}

void MainWindow::toggleTheme() {
    ef::setDark(!ef::isDark());
    ef::apply(*qApp);
    m_bar->themeButton()->setText(ef::isDark() ? QStringLiteral("浅色")
                                               : QStringLiteral("深色"));
    m_lastEventSeq = -1;
    refreshAll();
    setStatus(ef::isDark() ? QStringLiteral("已切换到深色主题")
                           : QStringLiteral("已切换到浅色主题"));
}

void MainWindow::setStatus(const QString& text) {
    statusBar()->showMessage(text, 8000);
}
