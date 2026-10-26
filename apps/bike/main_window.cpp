#include "main_window.h"

#include <QAbstractItemView>
#include <QColor>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFont>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QStatusBar>
#include <QStringList>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTimer>
#include <QVBoxLayout>

#include <cstring>

#include "map_canvas.h"

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    buildUi();
    loadDemo();
}

void MainWindow::runDemo() {
    if (m_system.stationCount() == 0) {
        loadDemo();
    }
    m_mode->setCurrentIndex(0);
    for (int i = 0; i < m_targets->count(); ++i) {
        QListWidgetItem* item = m_targets->item(i);
        const int id = item->data(Qt::UserRole).toInt();
        item->setCheckState((id == 3 || id == 5 || id == 7) ? Qt::Checked : Qt::Unchecked);
    }
    planRoute();
    simulateDispatch();
}

void MainWindow::buildUi() {
    setWindowTitle("城市公共自行车调度路径规划系统");
    resize(1600, 940);

    auto* central = new QWidget(this);
    setCentralWidget(central);
    auto* root = new QHBoxLayout(central);
    root->setContentsMargins(12, 12, 12, 12);
    root->setSpacing(12);

    // ---------------- 左侧 ----------------
    auto* left = new QWidget;
    left->setFixedWidth(330);
    auto* leftLayout = new QVBoxLayout(left);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(10);

    auto* dataGroup = new QGroupBox("路网数据");
    auto* dataLayout = new QVBoxLayout(dataGroup);
    auto* loadButton = new QPushButton("加载示例地图");
    auto* clearButton = new QPushButton("清空路网");
    auto* addStationButton = new QPushButton("新增站点");
    auto* addRoadButton = new QPushButton("新增道路");
    dataLayout->addWidget(loadButton);
    dataLayout->addWidget(clearButton);
    dataLayout->addWidget(addStationButton);
    dataLayout->addWidget(addRoadButton);
    leftLayout->addWidget(dataGroup);

    auto* searchGroup = new QGroupBox("最短路径 / 调度目标");
    auto* searchForm = new QFormLayout(searchGroup);
    m_mode = new QComboBox;
    m_mode->addItem("按距离最短");
    m_mode->addItem("按耗时最少");
    m_source = new QComboBox;
    m_dest = new QComboBox;
    searchForm->addRow("权重模式", m_mode);
    searchForm->addRow("调度总站", m_source);
    searchForm->addRow("查询终点", m_dest);
    m_targets = new QListWidget;
    m_targets->setMinimumHeight(150);
    searchForm->addRow("目标站点", m_targets);
    auto* planButton = new QPushButton("规划调度路线");
    searchForm->addRow(planButton);
    auto* queryButton = new QPushButton("查询两点最优路线");
    searchForm->addRow(queryButton);
    leftLayout->addWidget(searchGroup);

    auto* simGroup = new QGroupBox("调度仿真");
    auto* simForm = new QFormLayout(simGroup);
    m_truckCapacity = new QSpinBox;
    m_truckCapacity->setRange(1, 40);
    m_truckCapacity->setValue(8);
    m_truckCapacity->setSuffix(" 辆");
    m_lowPercent = new QSpinBox;
    m_lowPercent->setRange(0, 50);
    m_lowPercent->setValue(20);
    m_lowPercent->setSuffix(" %");
    m_highPercent = new QSpinBox;
    m_highPercent->setRange(50, 100);
    m_highPercent->setValue(80);
    m_highPercent->setSuffix(" %");
    simForm->addRow("调度车载量", m_truckCapacity);
    simForm->addRow("低库存阈值", m_lowPercent);
    simForm->addRow("高库存阈值", m_highPercent);
    auto* simulateButton = new QPushButton("开始调度模拟");
    simForm->addRow(simulateButton);
    leftLayout->addWidget(simGroup);

    auto* blockGroup = new QGroupBox("道路阻断");
    auto* blockLayout = new QVBoxLayout(blockGroup);
    m_edge = new QComboBox;
    blockLayout->addWidget(m_edge);
    auto* blockRow = new QHBoxLayout;
    auto* blockButton = new QPushButton("阻断");
    auto* restoreButton = new QPushButton("恢复");
    blockRow->addWidget(blockButton);
    blockRow->addWidget(restoreButton);
    blockLayout->addLayout(blockRow);
    auto* reachButton = new QPushButton("检查目标可达性");
    blockLayout->addWidget(reachButton);
    leftLayout->addWidget(blockGroup);
    leftLayout->addStretch();

    root->addWidget(left);

    // ---------------- 中间 ----------------
    auto* center = new QWidget;
    auto* centerLayout = new QVBoxLayout(center);
    centerLayout->setContentsMargins(0, 0, 0, 0);
    m_summary = new QLabel("尚未加载数据");
    m_summary->setStyleSheet("font-size:13px;color:#374151;font-weight:600;");
    m_canvas = new MapCanvas;
    m_canvas->setSystem(&m_system);
    centerLayout->addWidget(m_summary);
    centerLayout->addWidget(m_canvas, 1);
    root->addWidget(center, 1);

    // ---------------- 右侧 ----------------
    auto* right = new QWidget;
    right->setFixedWidth(430);
    auto* rightLayout = new QVBoxLayout(right);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(10);

    auto* stationGroup = new QGroupBox("站点状态");
    auto* stationLayout = new QVBoxLayout(stationGroup);
    m_stationTable = new QTableWidget(0, 5);
    m_stationTable->setHorizontalHeaderLabels({"编号", "名称", "现有", "容量", "状态"});
    m_stationTable->horizontalHeader()->setStretchLastSection(false);
    m_stationTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_stationTable->setColumnWidth(0, 44);
    m_stationTable->setColumnWidth(2, 50);
    m_stationTable->setColumnWidth(3, 50);
    m_stationTable->setColumnWidth(4, 56);
    m_stationTable->verticalHeader()->setVisible(false);
    m_stationTable->verticalHeader()->setDefaultSectionSize(26);
    m_stationTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_stationTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_stationTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    stationLayout->addWidget(m_stationTable);
    rightLayout->addWidget(stationGroup, 1);

    auto* routeGroup = new QGroupBox("路线与调度结果");
    auto* routeLayout = new QVBoxLayout(routeGroup);
    m_routeText = new QPlainTextEdit;
    m_routeText->setReadOnly(true);
    m_routeText->setMaximumBlockCount(300);
    m_routeText->setFont(QFont("Consolas", 9));
    routeLayout->addWidget(m_routeText);
    rightLayout->addWidget(routeGroup, 1);

    auto* alertGroup = new QGroupBox("告警");
    auto* alertLayout = new QVBoxLayout(alertGroup);
    m_alerts = new QListWidget;
    alertLayout->addWidget(m_alerts);
    rightLayout->addWidget(alertGroup);

    root->addWidget(right);

    connect(loadButton, &QPushButton::clicked, this, [this]() { loadDemo(); });
    connect(clearButton, &QPushButton::clicked, this, [this]() { clearAll(); });
    connect(addStationButton, &QPushButton::clicked, this, [this]() { addStationDialog(); });
    connect(addRoadButton, &QPushButton::clicked, this, [this]() { addRoadDialog(); });
    connect(planButton, &QPushButton::clicked, this, [this]() { planRoute(); });
    connect(queryButton, &QPushButton::clicked, this, [this]() { queryPair(); });
    connect(simulateButton, &QPushButton::clicked, this, [this]() { simulateDispatch(); });
    connect(blockButton, &QPushButton::clicked, this, [this]() { blockSelected(); });
    connect(restoreButton, &QPushButton::clicked, this, [this]() { restoreSelected(); });
    connect(reachButton, &QPushButton::clicked, this, [this]() { checkReachability(); });
    connect(m_canvas, &MapCanvas::stationClicked, this, [this](int id) {
        const int idx = m_source->findData(id);
        if (idx >= 0) {
            m_source->setCurrentIndex(idx);
            setStatus(QString("已选择总站：%1").arg(stationLabel(id)));
        }
    });

    m_truckTimer = new QTimer(this);
    m_truckTimer->setInterval(30);
    connect(m_truckTimer, &QTimer::timeout, this, [this]() { animateTruck(); });

    statusBar()->showMessage("就绪");
}

void MainWindow::loadDemo() {
    m_system.clear();
    m_truckTimer->stop();
    m_tour.clear();
    m_path.clear();
    m_canvas->clearPath();
    m_routeText->clear();
    m_alerts->clear();

    struct StationData {
        int id;
        const char* name;
        int bikes;
        int capacity;
        double x;
        double y;
    };
    const StationData stations[] = {
        {1, "火车站", 12, 20, 120, 160},
        {2, "图书馆", 3, 15, 300, 100},
        {3, "体育馆", 8, 25, 520, 140},
        {4, "食堂", 2, 18, 250, 320},
        {5, "宿舍区", 16, 30, 470, 300},
        {6, "教学楼", 5, 22, 700, 260},
        {7, "医院", 1, 12, 150, 500},
        {8, "公园", 9, 16, 400, 470},
        {9, "地铁站", 14, 24, 640, 470},
        {10, "调度总站", 20, 40, 860, 560},
    };
    char err[160] = {0};
    for (const auto& s : stations) {
        m_system.addStation(s.id, s.name, s.bikes, s.capacity, s.x, s.y, err, sizeof(err));
    }

    struct RoadData {
        int u;
        int v;
        double dist;
        double time;
    };
    const RoadData roads[] = {
        {1, 2, 800, 6},   {2, 3, 900, 7},   {1, 4, 600, 5},   {2, 5, 1100, 9},
        {3, 6, 700, 6},   {4, 5, 500, 4},   {4, 7, 900, 8},   {5, 6, 600, 5},
        {5, 8, 800, 7},   {6, 9, 700, 6},   {7, 8, 1000, 9},  {8, 9, 600, 5},
        {9, 10, 900, 8},  {3, 10, 1200, 10}, {6, 10, 1000, 8},
    };
    for (const auto& r : roads) {
        m_system.addRoad(r.u, r.v, r.dist, r.time, err, sizeof(err));
    }

    m_canvas->setSystem(&m_system);
    refreshSources();
    rebuildTargets();
    refreshStations();
    refreshEdges();
    refreshAll();

    const int depot = m_source->findData(10);
    if (depot >= 0) {
        m_source->setCurrentIndex(depot);
    }
    const int dest = m_dest->findData(9);
    if (dest >= 0) {
        m_dest->setCurrentIndex(dest);
    }
    setStatus("示例地图已加载：10 个站点、15 条道路");
}

void MainWindow::clearAll() {
    m_system.clear();
    m_truckTimer->stop();
    m_tour.clear();
    m_path.clear();
    m_canvas->clearPath();
    m_source->clear();
    m_dest->clear();
    m_targets->clear();
    m_edge->clear();
    m_stationTable->setRowCount(0);
    m_routeText->clear();
    m_alerts->clear();
    m_summary->setText("尚未加载数据");
    setStatus("路网已清空");
}

void MainWindow::refreshAll() {
    refreshSources();
    refreshStations();
    refreshEdges();
    refreshAlerts();
    m_canvas->update();
    m_summary->setText(QString("站点 %1 个 · 道路 %2 条 · 告警 %3 条")
                           .arg(m_system.stationCount())
                           .arg(m_system.roads().size())
                           .arg(m_system.alerts().size()));
}

void MainWindow::refreshSources() {
    const int current = m_source->currentData().isValid() ? m_source->currentData().toInt() : -1;
    const int destCurrent = m_dest->currentData().isValid() ? m_dest->currentData().toInt() : -1;
    m_source->clear();
    m_dest->clear();
    for (int i = 0; i < m_system.stationCount(); ++i) {
        const ds::Station& s = m_system.stationAt(i);
        const QString label = QString("%1 %2").arg(s.id).arg(QString::fromUtf8(s.name));
        m_source->addItem(label, s.id);
        m_dest->addItem(label, s.id);
    }
    if (current >= 0) {
        const int idx = m_source->findData(current);
        if (idx >= 0) {
            m_source->setCurrentIndex(idx);
        }
    }
    if (destCurrent >= 0) {
        const int idx = m_dest->findData(destCurrent);
        if (idx >= 0) {
            m_dest->setCurrentIndex(idx);
        }
    }
}

void MainWindow::rebuildTargets() {
    QVector<int> previous;
    for (int i = 0; i < m_targets->count(); ++i) {
        const QListWidgetItem* item = m_targets->item(i);
        if (item->checkState() == Qt::Checked) {
            previous.push_back(item->data(Qt::UserRole).toInt());
        }
    }
    const bool hadItems = m_targets->count() > 0;

    m_targets->clear();
    for (int i = 0; i < m_system.stationCount(); ++i) {
        const ds::Station& s = m_system.stationAt(i);
        auto* item = new QListWidgetItem(QString("%1 %2").arg(s.id).arg(QString::fromUtf8(s.name)));
        item->setData(Qt::UserRole, s.id);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        bool checked = false;
        if (hadItems) {
            checked = previous.contains(s.id);
        } else {
            checked = (s.id == 3 || s.id == 5 || s.id == 7);
        }
        item->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
        m_targets->addItem(item);
    }
}

void MainWindow::refreshStations() {
    m_stationTable->setRowCount(m_system.stationCount());
    for (int i = 0; i < m_system.stationCount(); ++i) {
        const ds::Station& s = m_system.stationAt(i);
        m_stationTable->setItem(i, 0, new QTableWidgetItem(QString::number(s.id)));
        m_stationTable->setItem(i, 1, new QTableWidgetItem(QString::fromUtf8(s.name)));
        m_stationTable->setItem(i, 2, new QTableWidgetItem(QString::number(s.bikes)));
        m_stationTable->setItem(i, 3, new QTableWidgetItem(QString::number(s.capacity)));

        const int low = s.capacity * m_lowPercent->value() / 100;
        const int high = s.capacity * m_highPercent->value() / 100;
        QString status = "正常";
        QColor color(0x27, 0xAE, 0x60);
        if (s.bikes < low) {
            status = "偏低";
            color = QColor(0xEB, 0x57, 0x57);
        } else if (s.bikes > high) {
            status = "偏高";
            color = QColor(0xF2, 0x99, 0x4A);
        }
        auto* statusItem = new QTableWidgetItem(status);
        statusItem->setForeground(color);
        m_stationTable->setItem(i, 4, statusItem);
    }
}

void MainWindow::refreshEdges() {
    const QString current = m_edge->currentData().isValid() ? m_edge->currentData().toString() : QString();
    m_edge->clear();
    for (int i = 0; i < m_system.roads().size(); ++i) {
        const ds::Road& r = m_system.roads()[i];
        const bool active = m_system.hasActiveRoad(r.u, r.v);
        const QString text = QString("%1 - %2  %3m / %4min%5")
                                 .arg(r.u)
                                 .arg(r.v)
                                 .arg(r.dist, 0, 'f', 0)
                                 .arg(r.time, 0, 'f', 0)
                                 .arg(active ? "" : "  [已阻断]");
        m_edge->addItem(text, QString("%1,%2").arg(r.u).arg(r.v));
    }
    if (!current.isEmpty()) {
        const int idx = m_edge->findData(current);
        if (idx >= 0) {
            m_edge->setCurrentIndex(idx);
        }
    }
}

void MainWindow::refreshAlerts() {
    m_alerts->clear();
    if (m_system.alerts().empty()) {
        m_alerts->addItem("无告警");
        return;
    }
    for (int i = 0; i < m_system.alerts().size(); ++i) {
        const ds::Alert& a = m_system.alerts()[i];
        m_alerts->addItem(QString("[%1] %2").arg(a.stationId).arg(QString::fromUtf8(a.text)));
    }
}

QString MainWindow::stationLabel(int id) const {
    const ds::Station* s = m_system.findStation(id);
    if (s == nullptr) {
        return QString::number(id);
    }
    return QString("%1 %2").arg(id).arg(QString::fromUtf8(s->name));
}

void MainWindow::addStationDialog() {
    QDialog dialog(this);
    dialog.setWindowTitle("新增站点");
    auto* layout = new QVBoxLayout(&dialog);
    auto* form = new QFormLayout;

    int maxId = 0;
    for (int i = 0; i < m_system.stationCount(); ++i) {
        maxId = qMax(maxId, m_system.stationAt(i).id);
    }

    QSpinBox idSpin;
    idSpin.setRange(1, 999999);
    idSpin.setValue(maxId + 1);
    QLineEdit nameEdit;
    nameEdit.setText(QString("新站点%1").arg(maxId + 1));
    QSpinBox bikesSpin;
    bikesSpin.setRange(0, 9999);
    bikesSpin.setValue(5);
    QSpinBox capacitySpin;
    capacitySpin.setRange(1, 9999);
    capacitySpin.setValue(20);
    QDoubleSpinBox xSpin;
    xSpin.setRange(-10000, 10000);
    xSpin.setValue(500);
    QDoubleSpinBox ySpin;
    ySpin.setRange(-10000, 10000);
    ySpin.setValue(350);
    form->addRow("站点编号", &idSpin);
    form->addRow("站点名称", &nameEdit);
    form->addRow("现有车辆", &bikesSpin);
    form->addRow("最大容量", &capacitySpin);
    form->addRow("X 坐标", &xSpin);
    form->addRow("Y 坐标", &ySpin);
    layout->addLayout(form);

    QDialogButtonBox buttons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(&buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(&buttons);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    char err[160] = {0};
    if (!m_system.addStation(idSpin.value(), nameEdit.text().toUtf8().constData(),
                             bikesSpin.value(), capacitySpin.value(), xSpin.value(),
                             ySpin.value(), err, sizeof(err))) {
        setStatus(QString::fromUtf8(err));
        return;
    }
    m_canvas->setSystem(&m_system);
    refreshSources();
    rebuildTargets();
    refreshStations();
    refreshEdges();
    refreshAll();
    setStatus(QString("已新增站点 %1").arg(idSpin.value()));
}

void MainWindow::addRoadDialog() {
    if (m_system.stationCount() < 2) {
        setStatus("至少需要两个站点才能新增道路");
        return;
    }
    QDialog dialog(this);
    dialog.setWindowTitle("新增道路");
    auto* layout = new QVBoxLayout(&dialog);
    auto* form = new QFormLayout;

    QComboBox uCombo;
    QComboBox vCombo;
    for (int i = 0; i < m_system.stationCount(); ++i) {
        const ds::Station& s = m_system.stationAt(i);
        const QString label = QString("%1 %2").arg(s.id).arg(QString::fromUtf8(s.name));
        uCombo.addItem(label, s.id);
        vCombo.addItem(label, s.id);
    }
    if (vCombo.count() > 1) {
        vCombo.setCurrentIndex(1);
    }
    QDoubleSpinBox distSpin;
    distSpin.setRange(1.0, 1000000.0);
    distSpin.setDecimals(1);
    distSpin.setValue(500.0);
    distSpin.setSuffix(" m");
    QDoubleSpinBox timeSpin;
    timeSpin.setRange(0.1, 100000.0);
    timeSpin.setDecimals(1);
    timeSpin.setValue(5.0);
    timeSpin.setSuffix(" min");
    form->addRow("端点 A", &uCombo);
    form->addRow("端点 B", &vCombo);
    form->addRow("距离", &distSpin);
    form->addRow("通行耗时", &timeSpin);
    layout->addLayout(form);

    QDialogButtonBox buttons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(&buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(&buttons);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    char err[160] = {0};
    if (!m_system.addRoad(uCombo.currentData().toInt(), vCombo.currentData().toInt(),
                          distSpin.value(), timeSpin.value(), err, sizeof(err))) {
        setStatus(QString::fromUtf8(err));
        return;
    }
    refreshEdges();
    refreshAll();
    setStatus(QString("已新增道路 %1 - %2")
                  .arg(uCombo.currentData().toInt())
                  .arg(vCombo.currentData().toInt()));
}

void MainWindow::queryPair() {
    if (m_system.stationCount() == 0) {
        setStatus("请先加载路网");
        return;
    }
    const int src = m_source->currentData().toInt();
    const int dst = m_dest->currentData().toInt();
    const auto mode = m_mode->currentIndex() == 1 ? ds::WeightMode::Time : ds::WeightMode::Distance;
    ds::Vector<int> path;
    double total = 0.0;
    if (!m_system.shortest(src, dst, mode, path, total)) {
        setStatus(QString("站点 %1 到 %2 不可达").arg(src).arg(dst));
        return;
    }

    m_tour.clear();
    m_path.clear();
    for (int i = 0; i < path.size(); ++i) {
        m_path.push_back(path[i]);
    }
    m_canvas->setPath(m_path);
    m_truckTimer->stop();
    m_canvas->setTruckProgress(-1.0);

    QString pathText;
    for (int i = 0; i < path.size(); ++i) {
        if (i > 0) {
            pathText += " → ";
        }
        pathText += stationLabel(path[i]);
    }
    const QString totalText = (mode == ds::WeightMode::Time)
                                  ? QString("%1 分钟").arg(total, 0, 'f', 1)
                                  : QString("%1 m").arg(total, 0, 'f', 0);
    m_routeText->setPlainText(QString("两点最优路线（%1）：\n%2\n\n总权重：%3")
                                  .arg(m_mode->currentText())
                                  .arg(pathText)
                                  .arg(totalText));
    setStatus(QString("查询完成：%1 个途经站点，总权重 %2").arg(path.size()).arg(totalText));
}

void MainWindow::checkReachability() {
    if (m_system.stationCount() == 0) {
        return;
    }
    const int depot = m_source->currentData().toInt();
    QString text = QString("从 %1 出发的可达性：\n").arg(stationLabel(depot));
    int count = 0;
    for (int i = 0; i < m_targets->count(); ++i) {
        const QListWidgetItem* item = m_targets->item(i);
        if (item->checkState() != Qt::Checked) {
            continue;
        }
        const int id = item->data(Qt::UserRole).toInt();
        const bool ok = m_system.connected(depot, id);
        text += QString("· %1：%2\n").arg(stationLabel(id)).arg(ok ? "可达" : "不可达");
        ++count;
    }
    if (count == 0) {
        text += "（尚未勾选目标站点）\n";
    }
    m_routeText->appendPlainText("\n" + text);
    setStatus("可达性检查完成");
}

void MainWindow::planRoute() {
    if (m_system.stationCount() == 0) {
        setStatus("请先加载路网");
        return;
    }
    const int depot = m_source->currentData().toInt();
    ds::Vector<int> targets;
    for (int i = 0; i < m_targets->count(); ++i) {
        const QListWidgetItem* item = m_targets->item(i);
        if (item->checkState() == Qt::Checked) {
            targets.push_back(item->data(Qt::UserRole).toInt());
        }
    }
    if (targets.empty()) {
        setStatus("请至少勾选一个目标站点");
        return;
    }

    const auto mode = m_mode->currentIndex() == 1 ? ds::WeightMode::Time : ds::WeightMode::Distance;
    ds::Vector<int> tour;
    ds::Vector<int> full;
    double total = 0.0;
    char err[160] = {0};
    if (!m_system.planRoute(depot, targets, mode, tour, full, total, err, sizeof(err))) {
        setStatus(QString::fromUtf8(err));
        return;
    }

    m_tour.clear();
    m_path.clear();
    for (int i = 0; i < tour.size(); ++i) {
        m_tour.push_back(tour[i]);
    }
    for (int i = 0; i < full.size(); ++i) {
        m_path.push_back(full[i]);
    }
    m_canvas->setPath(m_path);

    QString order;
    for (int i = 0; i < tour.size(); ++i) {
        if (i > 0) {
            order += " → ";
        }
        order += stationLabel(tour[i]);
    }
    QString pathText;
    for (int i = 0; i < full.size(); ++i) {
        if (i > 0) {
            pathText += " → ";
        }
        pathText += QString::number(full[i]);
    }
    const QString totalText = (mode == ds::WeightMode::Time)
                                  ? QString("%1 分钟").arg(total, 0, 'f', 1)
                                  : QString("%1 m（约 %2 km）")
                                        .arg(total, 0, 'f', 0)
                                        .arg(total / 1000.0, 0, 'f', 2);
    m_routeText->setPlainText(QString("调度顺序：\n%1\n\n完整途经站点：\n%2\n\n总权重：%3")
                                  .arg(order)
                                  .arg(pathText)
                                  .arg(totalText));
    m_truckTimer->stop();
    m_canvas->setTruckProgress(-1.0);
    setStatus(QString("路线规划完成：%1 个站点，总权重 %2").arg(tour.size()).arg(totalText));
}

void MainWindow::simulateDispatch() {
    if (m_tour.size() < 2) {
        setStatus("请先规划调度路线");
        return;
    }
    ds::Vector<int> tour;
    for (int id : m_tour) {
        tour.push_back(id);
    }
    ds::DispatchReport report;
    if (!m_system.simulate(tour, m_truckCapacity->value(), m_lowPercent->value(),
                           m_highPercent->value(), report)) {
        setStatus(QString::fromUtf8(report.message));
        refreshStations();
        refreshAlerts();
        return;
    }

    m_routeText->appendPlainText(QString("\n\n—— 调度模拟 ——\n%1").arg(QString::fromUtf8(report.message)));
    for (int i = 0; i < report.stops.size(); ++i) {
        const ds::StopRecord& s = report.stops[i];
        m_routeText->appendPlainText(
            QString("%1：%2 → %3，卸 %4 / 装 %5，车载 %6%7")
                .arg(stationLabel(s.stationId))
                .arg(s.before)
                .arg(s.after)
                .arg(s.unloaded)
                .arg(s.loaded)
                .arg(s.load)
                .arg(s.alert ? "  ⚠" : ""));
    }

    m_progress = 0.0;
    m_canvas->setTruckProgress(0.0);
    m_truckTimer->start();
    refreshStations();
    refreshAlerts();
    m_summary->setText(QString("站点 %1 个 · 道路 %2 条 · 告警 %3 条")
                           .arg(m_system.stationCount())
                           .arg(m_system.roads().size())
                           .arg(m_system.alerts().size()));
    setStatus(QString::fromUtf8(report.message));
}

void MainWindow::blockSelected() {
    const QString data = m_edge->currentData().toString();
    if (data.isEmpty()) {
        return;
    }
    const QStringList parts = data.split(',');
    if (parts.size() != 2) {
        return;
    }
    const int u = parts[0].toInt();
    const int v = parts[1].toInt();
    if (!m_system.blockRoad(u, v)) {
        setStatus("该道路已阻断或不存在");
        return;
    }
    m_tour.clear();
    m_path.clear();
    m_canvas->clearPath();
    m_truckTimer->stop();
    refreshEdges();
    refreshAll();
    m_routeText->appendPlainText("\n道路已阻断，请重新规划路线。");
    checkReachability();
    setStatus(QString("已阻断道路 %1 - %2").arg(u).arg(v));
}

void MainWindow::restoreSelected() {
    const QString data = m_edge->currentData().toString();
    if (data.isEmpty()) {
        return;
    }
    const QStringList parts = data.split(',');
    if (parts.size() != 2) {
        return;
    }
    const int u = parts[0].toInt();
    const int v = parts[1].toInt();
    if (!m_system.restoreRoad(u, v)) {
        setStatus("该道路不存在");
        return;
    }
    refreshEdges();
    refreshAll();
    checkReachability();
    setStatus(QString("已恢复道路 %1 - %2").arg(u).arg(v));
}

void MainWindow::animateTruck() {
    m_progress += 0.006;
    if (m_progress >= 1.0) {
        m_progress = 1.0;
        m_truckTimer->stop();
    }
    m_canvas->setTruckProgress(m_progress);
}

void MainWindow::setStatus(const QString& text) {
    statusBar()->showMessage(text, 8000);
}
