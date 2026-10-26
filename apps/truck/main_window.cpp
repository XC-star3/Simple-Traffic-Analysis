#include "main_window.h"

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFont>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRandomGenerator>
#include <QSpinBox>
#include <QStatusBar>
#include <QTimer>
#include <QVBoxLayout>
#include <QVector>

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
    m_yard.depart(1002, m_now, &fee);
    m_nowSpin->setValue(m_now);
    m_nextId = 1011;
    m_arriveId->setValue(m_nextId);
    m_departId->setValue(1003);
    refreshAll();
    setStatus("演示数据：10 辆车已到达，T1002 已完成让路离场");
}

void MainWindow::buildUi() {
    setWindowTitle("快递中转栈-队列混合调度模拟");
    resize(1500, 900);

    auto* central = new QWidget(this);
    setCentralWidget(central);
    auto* root = new QHBoxLayout(central);
    root->setContentsMargins(12, 12, 12, 12);
    root->setSpacing(12);

    // ---------------- 左侧控制面板 ----------------
    auto* left = new QWidget;
    left->setFixedWidth(315);
    auto* leftLayout = new QVBoxLayout(left);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(10);

    auto* configGroup = new QGroupBox("场地参数");
    auto* configForm = new QFormLayout(configGroup);
    m_capacity = new QSpinBox;
    m_capacity->setRange(1, 12);
    m_capacity->setValue(8);
    m_capacity->setSuffix(" 辆");
    m_tempCapacity = new QSpinBox;
    m_tempCapacity->setRange(0, 12);
    m_tempCapacity->setValue(7);
    m_tempCapacity->setSuffix(" 辆");
    m_fee = new QDoubleSpinBox;
    m_fee->setRange(0.0, 10000.0);
    m_fee->setDecimals(2);
    m_fee->setValue(5.0);
    m_fee->setSuffix(" 元/小时");
    configForm->addRow("主通道容量", m_capacity);
    configForm->addRow("临时区容量", m_tempCapacity);
    configForm->addRow("停车费率", m_fee);
    auto* applyButton = new QPushButton("应用参数并重置");
    configForm->addRow(applyButton);
    leftLayout->addWidget(configGroup);

    auto* arriveGroup = new QGroupBox("货车到达");
    auto* arriveForm = new QFormLayout(arriveGroup);
    m_arriveId = new QSpinBox;
    m_arriveId->setRange(1, 999999);
    m_arriveId->setValue(1001);
    m_type = new QComboBox;
    m_type->addItem("生鲜件（优先级最高）");
    m_type->addItem("普通件");
    m_type->addItem("大件货（优先级最低）");
    arriveForm->addRow("货车编号", m_arriveId);
    arriveForm->addRow("货物类型", m_type);
    auto* arriveButton = new QPushButton("到达并排队");
    arriveForm->addRow(arriveButton);
    auto* hint = new QLabel("到达时刻取下方“当前时刻”");
    hint->setStyleSheet("color:#6B7280;font-size:11px;");
    arriveForm->addRow(hint);
    leftLayout->addWidget(arriveGroup);

    auto* departGroup = new QGroupBox("货车离场");
    auto* departForm = new QFormLayout(departGroup);
    m_departId = new QSpinBox;
    m_departId->setRange(1, 999999);
    m_departId->setValue(1001);
    departForm->addRow("货车编号", m_departId);
    auto* departButton = new QPushButton("离场（自动让路）");
    departForm->addRow(departButton);
    leftLayout->addWidget(departGroup);

    auto* simGroup = new QGroupBox("仿真时钟 / 自动模拟");
    auto* simForm = new QFormLayout(simGroup);
    m_nowSpin = new QDoubleSpinBox;
    m_nowSpin->setRange(0.0, 100000.0);
    m_nowSpin->setDecimals(1);
    m_nowSpin->setValue(0.0);
    m_nowSpin->setSuffix(" 分钟");
    m_interval = new QSpinBox;
    m_interval->setRange(200, 3000);
    m_interval->setSingleStep(100);
    m_interval->setValue(700);
    m_interval->setSuffix(" ms");
    simForm->addRow("当前时刻", m_nowSpin);
    simForm->addRow("事件间隔", m_interval);
    m_autoButton = new QPushButton("开始自动模拟");
    simForm->addRow(m_autoButton);
    auto* stepButton = new QPushButton("单步推进 2 分钟");
    simForm->addRow(stepButton);
    auto* resetButton = new QPushButton("重置系统");
    simForm->addRow(resetButton);
    leftLayout->addWidget(simGroup);
    leftLayout->addStretch();

    root->addWidget(left);

    // ---------------- 中间画布 ----------------
    auto* center = new QWidget;
    auto* centerLayout = new QVBoxLayout(center);
    centerLayout->setContentsMargins(0, 0, 0, 0);
    auto* canvasTitle = new QLabel("运行态势");
    canvasTitle->setStyleSheet("font-size:16px;font-weight:600;color:#1F2A37;");
    m_canvas = new YardCanvas;
    m_canvas->setYard(&m_yard);
    centerLayout->addWidget(canvasTitle);
    centerLayout->addWidget(m_canvas, 1);
    root->addWidget(center, 1);

    // ---------------- 右侧统计与日志 ----------------
    auto* right = new QWidget;
    right->setFixedWidth(380);
    auto* rightLayout = new QVBoxLayout(right);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(10);

    auto* statsGroup = new QGroupBox("统计");
    auto* statsGrid = new QGridLayout(statsGroup);
    statsGrid->setColumnStretch(1, 1);
    auto addStat = [&](int row, const QString& name, QLabel*& out) {
        auto* key = new QLabel(name);
        key->setStyleSheet("color:#6B7280;");
        out = new QLabel("-");
        out->setStyleSheet("font-weight:600;color:#1F2A37;");
        statsGrid->addWidget(key, row, 0);
        statsGrid->addWidget(out, row, 1);
    };
    addStat(0, "累计收入", m_statRevenue);
    addStat(1, "主通道占用", m_statChannel);
    addStat(2, "临时区占用", m_statTemp);
    addStat(3, "队列积压", m_statQueues);
    addStat(4, "到达 / 服务", m_statCounts);
    addStat(5, "临时区峰值", m_statPeak);
    rightLayout->addWidget(statsGroup);

    auto* logGroup = new QGroupBox("事件日志");
    auto* logLayout = new QVBoxLayout(logGroup);
    m_log = new QPlainTextEdit;
    m_log->setReadOnly(true);
    m_log->setMaximumBlockCount(600);
    QFont logFont("Consolas", 9);
    m_log->setFont(logFont);
    logLayout->addWidget(m_log);
    rightLayout->addWidget(logGroup, 1);

    root->addWidget(right);

    // ---------------- 连接 ----------------
    connect(applyButton, &QPushButton::clicked, this, [this]() { applyConfig(); });
    connect(arriveButton, &QPushButton::clicked, this, [this]() { doArrive(); });
    connect(departButton, &QPushButton::clicked, this, [this]() { doDepart(); });
    connect(m_autoButton, &QPushButton::clicked, this, [this]() { toggleAuto(); });
    connect(stepButton, &QPushButton::clicked, this, [this]() { autoTick(); });
    connect(resetButton, &QPushButton::clicked, this, [this]() { resetAll(); });

    m_autoTimer = new QTimer(this);
    connect(m_autoTimer, &QTimer::timeout, this, [this]() { autoTick(); });

    statusBar()->showMessage("就绪");
}

void MainWindow::applyConfig() {
    if (!m_yard.configure(m_capacity->value(), m_tempCapacity->value(), m_fee->value())) {
        setStatus("参数非法，请检查容量与费率");
        return;
    }
    m_now = 0.0;
    m_nextId = 1001;
    m_arriveId->setValue(m_nextId);
    m_departId->setValue(m_nextId);
    m_nowSpin->setValue(0.0);
    m_lastEvents = 0;
    m_log->clear();
    m_canvas->resetAnimation();
    refreshAll();
    setStatus("参数已应用，系统已重置");
}

void MainWindow::doArrive() {
    if (!m_yard.configured()) {
        setStatus("系统尚未初始化");
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
        setStatus("系统尚未初始化");
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

void MainWindow::autoTick() {
    if (!m_yard.configured()) {
        return;
    }
    m_now += 2.0;
    m_nowSpin->setValue(m_now);

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
        m_autoButton->setText("开始自动模拟");
    } else {
        m_autoTimer->setInterval(m_interval->value());
        m_autoTimer->start();
        m_autoButton->setText("暂停自动模拟");
    }
}

void MainWindow::resetAll() {
    m_autoTimer->stop();
    m_autoButton->setText("开始自动模拟");
    applyConfig();
}

void MainWindow::refreshAll() {
    m_canvas->update();
    const ds::YardStats s = m_yard.stats();
    if (m_statRevenue != nullptr) {
        m_statRevenue->setText(QString("%1 元").arg(s.revenue, 0, 'f', 2));
        m_statChannel->setText(QString("%1 / %2 辆").arg(s.inChannel).arg(m_yard.capacity()));
        m_statTemp->setText(QString("%1 / %2 辆").arg(s.inTemp).arg(m_yard.tempCapacity()));
        m_statQueues->setText(QString("生鲜 %1 · 普通 %2 · 大件 %3")
                                  .arg(s.waiting[0])
                                  .arg(s.waiting[1])
                                  .arg(s.waiting[2]));
        m_statCounts->setText(QString("%1 / %2 辆").arg(s.arrived).arg(s.served));
        m_statPeak->setText(QString("%1 辆").arg(s.maxTempUsed));
    }
    syncEvents();
}

void MainWindow::syncEvents() {
    if (m_log == nullptr) {
        return;
    }
    const ds::Vector<ds::Event>& events = m_yard.events();
    if (events.size() < m_lastEvents) {
        m_log->clear();
        m_lastEvents = 0;
    }
    for (int i = m_lastEvents; i < events.size(); ++i) {
        m_log->appendPlainText(QString("[%1 min] %2")
                                   .arg(events[i].time, 0, 'f', 1)
                                   .arg(QString::fromUtf8(events[i].text)));
    }
    m_lastEvents = events.size();
}

void MainWindow::setStatus(const QString& text) {
    statusBar()->showMessage(text, 8000);
}
