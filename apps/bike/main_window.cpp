#include "main_window.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QBrush>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDoubleSpinBox>
#include <QFont>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTimer>
#include <QVBoxLayout>

#include "common/theme.h"
#include "common/widgets.h"
#include "map_canvas.h"

BikeWindow::BikeWindow(QWidget* parent) : QWidget(parent) {
    buildUi();
    loadDemo();
}

void BikeWindow::runDemo() {
    if (m_system.stationCount() == 0) {
        loadDemo();
    }
    m_mode->setCurrentIndex(0);
    {
        const QSignalBlocker blocker(m_targets);
        for (int i = 0; i < m_targets->count(); ++i) {
            QListWidgetItem* item = m_targets->item(i);
            const int id = item->data(Qt::UserRole).toInt();
            item->setCheckState((id == 3 || id == 5 || id == 7) ? Qt::Checked
                                                                : Qt::Unchecked);
        }
    }
    updateTargetStyles();
    updateSelectionView();
    updateMetrics();
    planRoute();
    simulateDispatch();
}

void BikeWindow::buildUi() {
    setWindowTitle("城市公共自行车 / 调度台");
    resize(1440, 900);
    setMinimumSize(1200, 760);

    auto* host = new QVBoxLayout(this);
    host->setContentsMargins(0, 0, 0, 0);
    host->setSpacing(0);
    auto* central = new QWidget(this);
    host->addWidget(central);
    auto* root = new QVBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    root->addWidget(new ef::EfTriStrip(central));
    m_bar = new ef::EfCommandBar(QStringLiteral("B"), QStringLiteral("CYCLE"),
                                 QStringLiteral("公共自行车"), QStringLiteral("DISPATCH"),
                                 QStringLiteral("调度台"), central);
    root->addWidget(m_bar);

    auto* content = new QWidget(central);
    auto* columns = new QHBoxLayout(content);
    columns->setContentsMargins(14, 14, 14, 14);
    columns->setSpacing(12);
    root->addWidget(content, 1);

    // ---------------- 左侧 ----------------
    auto* left = new QWidget;
    auto* leftLayout = new QVBoxLayout(left);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(8);

    auto* data = new ef::EfPanel(QStringLiteral("NETWORK"), QStringLiteral("路网数据"));
    auto* dataGrid = new QGridLayout;
    dataGrid->setContentsMargins(0, 0, 0, 0);
    dataGrid->setSpacing(6);
    auto* loadButton =
        new ef::EfButton(QStringLiteral("加载示例地图"), ef::EfButton::Variant::Primary);
    auto* clearButton =
        new ef::EfButton(QStringLiteral("清空路网"), ef::EfButton::Variant::Danger);
    auto* addStationButton =
        new ef::EfButton(QStringLiteral("新增站点"), ef::EfButton::Variant::Secondary);
    auto* addRoadButton =
        new ef::EfButton(QStringLiteral("新增道路"), ef::EfButton::Variant::Secondary);
    loadButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    clearButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    addStationButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    addRoadButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    dataGrid->addWidget(loadButton, 0, 0);
    dataGrid->addWidget(clearButton, 0, 1);
    dataGrid->addWidget(addStationButton, 1, 0);
    dataGrid->addWidget(addRoadButton, 1, 1);
    data->body()->addLayout(dataGrid);
    leftLayout->addWidget(data);

    auto* search = new ef::EfPanel(QStringLiteral("ROUTE"),
                                   QStringLiteral("最短路径 / 调度目标"));
    auto* searchForm = new QFormLayout;
    searchForm->setContentsMargins(0, 0, 0, 0);
    searchForm->setHorizontalSpacing(10);
    searchForm->setVerticalSpacing(6);
    m_mode = new QComboBox;
    m_mode->addItem(QStringLiteral("按距离最短"));
    m_mode->addItem(QStringLiteral("按耗时最少"));
    m_source = new QComboBox;
    m_dest = new QComboBox;
    searchForm->addRow(QStringLiteral("权重模式"), m_mode);
    searchForm->addRow(QStringLiteral("调度总站"), m_source);
    searchForm->addRow(QStringLiteral("查询终点"), m_dest);
    search->body()->addLayout(searchForm);
    auto* targetLabel = ef::makeEyebrow(QStringLiteral("TARGETS"), search);
    search->body()->addWidget(targetLabel);
    m_targets = new QListWidget(search);
    m_targets->setMinimumHeight(130);
    m_targets->setMaximumHeight(150);
    m_targets->setSelectionMode(QAbstractItemView::NoSelection);
    m_targets->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_targets->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_targets->setUniformItemSizes(true);
    m_targets->setFont(ef::font(ef::FontRole::Field));
    m_targets->setItemDelegate(new ef::EfListDelegate(m_targets));
    m_targets->setAccessibleName(QStringLiteral("调度目标站点"));
    search->body()->addWidget(m_targets);
    auto* planButton =
        new ef::EfButton(QStringLiteral("规划调度路线"), ef::EfButton::Variant::Primary);
    planButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    search->body()->addWidget(planButton);
    auto* queryButton = new ef::EfButton(QStringLiteral("查询两点最优路线"),
                                         ef::EfButton::Variant::Secondary);
    queryButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    search->body()->addWidget(queryButton);
    leftLayout->addWidget(search);

    auto* sim = new ef::EfPanel(QStringLiteral("SIMULATION"), QStringLiteral("调度仿真"));
    auto* simForm = new QFormLayout;
    simForm->setContentsMargins(0, 0, 0, 0);
    simForm->setHorizontalSpacing(10);
    simForm->setVerticalSpacing(6);
    m_truckCapacity = new QSpinBox;
    m_truckCapacity->setRange(1, 40);
    m_truckCapacity->setValue(8);
    m_truckCapacity->setSuffix(QStringLiteral(" 辆"));
    m_lowPercent = new QSpinBox;
    m_lowPercent->setRange(0, 50);
    m_lowPercent->setValue(20);
    m_lowPercent->setSuffix(QStringLiteral(" %"));
    m_lowPercent->setFixedWidth(64);
    m_highPercent = new QSpinBox;
    m_highPercent->setRange(50, 100);
    m_highPercent->setValue(80);
    m_highPercent->setSuffix(QStringLiteral(" %"));
    m_highPercent->setFixedWidth(64);
    simForm->addRow(QStringLiteral("调度车载量"), m_truckCapacity);
    sim->body()->addLayout(simForm);
    auto* thresholdRow = new QHBoxLayout;
    thresholdRow->setContentsMargins(0, 0, 0, 0);
    thresholdRow->setSpacing(6);
    thresholdRow->addWidget(ef::makeField(QStringLiteral("下限"), sim));
    thresholdRow->addWidget(m_lowPercent);
    thresholdRow->addWidget(ef::makeField(QStringLiteral("上限"), sim));
    thresholdRow->addWidget(m_highPercent);
    thresholdRow->addStretch(1);
    sim->body()->addLayout(thresholdRow);
    auto* simulateButton =
        new ef::EfButton(QStringLiteral("开始调度模拟"), ef::EfButton::Variant::Primary);
    simulateButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    sim->body()->addWidget(simulateButton);
    leftLayout->addWidget(sim);
    leftLayout->addStretch(1);

    auto* leftScroll = new QScrollArea;
    leftScroll->setWidgetResizable(true);
    leftScroll->setFrameShape(QFrame::NoFrame);
    leftScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    leftScroll->setWidget(left);
    leftScroll->setFixedWidth(300);
    columns->addWidget(leftScroll);

    // ---------------- 中间 ----------------
    auto* center = new QWidget;
    center->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto* centerLayout = new QVBoxLayout(center);
    centerLayout->setContentsMargins(0, 0, 0, 0);
    centerLayout->setSpacing(10);

    auto* metricsPanel =
        new ef::EfPanel(QStringLiteral("DISPATCH"), QStringLiteral("调度态势"));
    m_metrics = new ef::EfMetrics(4, metricsPanel);
    metricsPanel->body()->addWidget(m_metrics);
    m_contextFields = ef::makeField(QStringLiteral("权重 —  │  总站 —  │  目标 0"),
                                    metricsPanel);
    metricsPanel->body()->addWidget(m_contextFields);
    centerLayout->addWidget(metricsPanel);

    auto* mapPanel =
        new ef::EfPanel(QStringLiteral("NETWORK MAP"), QStringLiteral("校园路网"));
    m_canvas = new MapCanvas(mapPanel);
    m_canvas->setSystem(&m_system);
    m_canvas->setAccessibleName(QStringLiteral("校园路网站点与调度路线"));
    m_canvas->setMinimumSize(480, 300);
    mapPanel->body()->setContentsMargins(0, 0, 0, 0);
    mapPanel->body()->addWidget(m_canvas, 1);
    centerLayout->addWidget(mapPanel, 1);

    auto* block = new ef::EfPanel(QStringLiteral("ROAD BLOCK"), QStringLiteral("道路阻断"));
    m_edge = new QComboBox(block);
    m_edge->setAccessibleName(QStringLiteral("道路"));
    m_edge->setFont(ef::font(ef::FontRole::Field));
    m_edge->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    auto* blockRow = new QHBoxLayout;
    blockRow->setContentsMargins(0, 0, 0, 0);
    blockRow->setSpacing(6);
    auto* blockButton =
        new ef::EfButton(QStringLiteral("阻断"), ef::EfButton::Variant::Secondary);
    auto* restoreButton =
        new ef::EfButton(QStringLiteral("恢复"), ef::EfButton::Variant::Secondary);
    auto* reachButton = new ef::EfButton(QStringLiteral("检查目标可达性"),
                                         ef::EfButton::Variant::Secondary);
    blockButton->setFixedWidth(64);
    restoreButton->setFixedWidth(64);
    reachButton->setFixedWidth(126);
    m_showWeights = new QCheckBox(QStringLiteral("权重"), block);
    m_showWeights->setToolTip(QStringLiteral("在画布上显示道路距离和耗时"));
    blockRow->addWidget(m_edge, 1);
    blockRow->addWidget(m_showWeights);
    blockRow->addWidget(blockButton);
    blockRow->addWidget(restoreButton);
    blockRow->addWidget(reachButton);
    block->body()->addLayout(blockRow);
    centerLayout->addWidget(block);
    columns->addWidget(center, 1);

    // ---------------- 右侧 ----------------
    auto* right = new QWidget;
    right->setFixedWidth(360);
    auto* rightLayout = new QVBoxLayout(right);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(10);

    auto* stationPanel =
        new ef::EfPanel(QStringLiteral("STATIONS"), QStringLiteral("站点状态"));
    m_stationTable = new QTableWidget(0, 5, stationPanel);
    m_stationTable->setAccessibleName(QStringLiteral("站点状态"));
    m_stationTable->setHorizontalHeaderLabels(
        {QStringLiteral("编号"), QStringLiteral("名称"), QStringLiteral("现有"),
         QStringLiteral("容量"), QStringLiteral("状态")});
    m_stationTable->horizontalHeader()->setStretchLastSection(false);
    m_stationTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_stationTable->horizontalHeader()->setMinimumSectionSize(36);
    m_stationTable->horizontalHeader()->setFont(ef::font(ef::FontRole::Eyebrow));
    m_stationTable->verticalHeader()->setVisible(false);
    m_stationTable->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    m_stationTable->verticalHeader()->setDefaultSectionSize(22);
    m_stationTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_stationTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_stationTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_stationTable->setSelectionMode(QAbstractItemView::NoSelection);
    m_stationTable->setFocusPolicy(Qt::NoFocus);
    m_stationTable->setAlternatingRowColors(false);
    m_stationTable->setMinimumHeight(260);
    stationPanel->body()->addWidget(m_stationTable, 1);
    rightLayout->addWidget(stationPanel, 5);

    auto* routePanel =
        new ef::EfPanel(QStringLiteral("RESULT"), QStringLiteral("路线与调度结果"));
    m_routeText = new QPlainTextEdit(routePanel);
    m_routeText->setAccessibleName(QStringLiteral("路线与调度结果"));
    m_routeText->setReadOnly(true);
    m_routeText->setMaximumBlockCount(420);
    m_routeText->setFont(ef::font(ef::FontRole::Body));
    m_routeText->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    m_routeText->setPlaceholderText(
        QStringLiteral("// PLAN\n选择目标站点后点击“规划调度路线”，路线与调度记录会显示在这里。"));
    routePanel->body()->addWidget(m_routeText, 1);
    rightLayout->addWidget(routePanel, 3);

    auto* alertPanel =
        new ef::EfPanel(QStringLiteral("ALERTS"), QStringLiteral("调度告警"));
    m_alerts = new QListWidget(alertPanel);
    m_alerts->setSelectionMode(QAbstractItemView::NoSelection);
    m_alerts->setFocusPolicy(Qt::NoFocus);
    m_alerts->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_alerts->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_alerts->setUniformItemSizes(true);
    m_alerts->setMinimumHeight(40);
    m_alerts->setFont(ef::font(ef::FontRole::Field));
    m_alerts->setItemDelegate(new ef::EfListDelegate(m_alerts));
    m_alerts->setAccessibleName(QStringLiteral("调度告警"));
    alertPanel->body()->addWidget(m_alerts, 1);
    rightLayout->addWidget(alertPanel, 1);
    columns->addWidget(right);

    // ---------------- 连接 ----------------
    connect(loadButton, &QPushButton::clicked, this, [this]() { loadDemo(); });
    connect(clearButton, &QPushButton::clicked, this, [this]() { confirmClearAll(); });
    connect(addStationButton, &QPushButton::clicked, this,
            [this]() { addStationDialog(); });
    connect(addRoadButton, &QPushButton::clicked, this, [this]() { addRoadDialog(); });
    connect(planButton, &QPushButton::clicked, this, [this]() { planRoute(); });
    connect(queryButton, &QPushButton::clicked, this, [this]() { queryPair(); });
    connect(simulateButton, &QPushButton::clicked, this,
            [this]() { simulateDispatch(); });
    connect(blockButton, &QPushButton::clicked, this, [this]() { blockSelected(); });
    connect(restoreButton, &QPushButton::clicked, this,
            [this]() { restoreSelected(); });
    connect(reachButton, &QPushButton::clicked, this,
            [this]() { checkReachability(); });
    connect(m_showWeights, &QCheckBox::toggled, this, [this](bool checked) {
        m_canvas->setShowWeights(checked);
    });
    connect(m_edge, &QComboBox::currentIndexChanged, this,
            [this]() { m_edge->setToolTip(m_edge->currentText()); });
    connect(m_bar->themeButton(), &QPushButton::clicked, this,
            [this]() { toggleTheme(); });
    connect(m_targets, &QListWidget::itemChanged, this, [this]() {
        updateTargetStyles();
        invalidateRoute(QStringLiteral("目标站点已更改，请重新规划路线。"));
        updateSelectionView();
        updateMetrics();
    });
    connect(m_mode, &QComboBox::currentIndexChanged, this, [this]() {
        invalidateRoute(QStringLiteral("权重模式已更改，请重新规划路线。"));
        updateSelectionView();
        updateMetrics();
    });
    connect(m_source, &QComboBox::currentIndexChanged, this, [this]() {
        invalidateRoute(QStringLiteral("调度总站已更改，请重新规划路线。"));
        updateSelectionView();
        updateMetrics();
    });
    connect(m_lowPercent, &QSpinBox::valueChanged, this, [this]() {
        refreshStations();
        m_canvas->setThresholds(m_lowPercent->value(), m_highPercent->value());
        updateMetrics();
    });
    connect(m_highPercent, &QSpinBox::valueChanged, this, [this]() {
        refreshStations();
        m_canvas->setThresholds(m_lowPercent->value(), m_highPercent->value());
        updateMetrics();
    });
    m_canvas->setStationClickedHandler([this](int id) {
        const int idx = m_source->findData(id);
        if (idx >= 0) {
            m_source->setCurrentIndex(idx);
            m_canvas->setDepot(id);
            updateSelectionView();
            updateMetrics();
            setStatus(QString("已选择总站：%1").arg(stationLabel(id)));
        }
    });

    m_truckTimer = new QTimer(this);
    m_truckTimer->setInterval(30);
    connect(m_truckTimer, &QTimer::timeout, this, [this]() { animateTruck(); });

    m_status = new ef::EfStatusStrip(central);
    root->addWidget(m_status);

    central->setFocusPolicy(Qt::StrongFocus);
    central->setFocus(Qt::OtherFocusReason);
}

void BikeWindow::loadDemo() {
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
        m_system.addStation(s.id, s.name, s.bikes, s.capacity, s.x, s.y, err,
                            sizeof(err));
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
    m_canvas->setDepot(m_source->currentData().toInt());
    m_canvas->setThresholds(m_lowPercent->value(), m_highPercent->value());
    updateSelectionView();
    updateMetrics();
    setStatus(QStringLiteral("示例地图已加载：10 个站点、15 条道路"));
}

void BikeWindow::invalidateRoute(const QString& reason) {
    const bool hadRoute = !m_tour.isEmpty() || !m_path.isEmpty();
    m_truckTimer->stop();
    m_tour.clear();
    m_path.clear();
    if (m_canvas != nullptr) {
        m_canvas->clearPath();
        m_canvas->setTruckProgress(-1.0);
    }
    if (hadRoute && m_routeText != nullptr) {
        m_routeText->appendPlainText(QString("\n// ROUTE INVALID\n%1").arg(reason));
        m_routeText->verticalScrollBar()->setValue(
            m_routeText->verticalScrollBar()->maximum());
        setStatus(reason);
    }
}

void BikeWindow::confirmClearAll() {
    if (m_system.stationCount() == 0) {
        setStatus(QStringLiteral("路网已经是空的"));
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("清空路网"));
    dialog.setMinimumWidth(430);
    auto* layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(18, 16, 18, 16);
    layout->setSpacing(10);
    layout->addWidget(ef::makeEyebrow(QStringLiteral("CLEAR NETWORK"), &dialog));
    layout->addWidget(ef::makeTitle(QStringLiteral("清空路网"), &dialog));
    layout->addWidget(ef::makeRule(&dialog));
    auto* body = new QLabel(
        QString("将删除 %1 个站点和 %2 条道路，无法撤销。")
            .arg(m_system.stationCount())
            .arg(m_system.roads().size()),
        &dialog);
    body->setWordWrap(true);
    body->setFont(ef::font(ef::FontRole::Body));
    layout->addWidget(body);

    auto* actions = new QHBoxLayout;
    actions->addStretch(1);
    auto* cancel = new ef::EfButton(QStringLiteral("取消"), ef::EfButton::Variant::Secondary,
                                    &dialog);
    auto* clear = new ef::EfButton(QStringLiteral("清空"), ef::EfButton::Variant::Danger,
                                   &dialog);
    actions->addWidget(cancel);
    actions->addWidget(clear);
    layout->addLayout(actions);

    connect(cancel, &QPushButton::clicked, &dialog, &QDialog::reject);
    connect(clear, &QPushButton::clicked, &dialog, &QDialog::accept);
    cancel->setDefault(true);
    clear->setAutoDefault(false);
    cancel->setFocus(Qt::OtherFocusReason);

    if (dialog.exec() == QDialog::Accepted) {
        clearAll();
    }
}

void BikeWindow::clearAll() {
    m_system.clear();
    m_truckTimer->stop();
    m_tour.clear();
    m_path.clear();
    m_canvas->setSystem(&m_system);
    m_canvas->clearPath();
    m_source->clear();
    m_dest->clear();
    m_targets->clear();
    auto* emptyTarget = new QListWidgetItem(
        QStringLiteral("// 路网为空。先加载示例地图，或新增站点。"));
    emptyTarget->setFlags(Qt::NoItemFlags);
    emptyTarget->setForeground(QBrush(ef::palette().ink2));
    emptyTarget->setFont(ef::font(ef::FontRole::Field));
    m_targets->addItem(emptyTarget);
    m_stationTable->setRowCount(0);
    m_routeText->clear();
    m_alerts->clear();
    m_contextFields->setText(QStringLiteral("权重 —  │  总站 —  │  目标 0"));
    refreshEdges();
    refreshAlerts();
    updateSelectionView();
    updateMetrics();
    setStatus(QStringLiteral("路网已清空"));
}

void BikeWindow::refreshAll() {
    refreshSources();
    refreshStations();
    refreshEdges();
    refreshAlerts();
    updateTargetStyles();
    updateSelectionView();
    updateMetrics();
    m_canvas->setThresholds(m_lowPercent->value(), m_highPercent->value());
    m_canvas->update();
}

void BikeWindow::refreshSources() {
    const int current = m_source->currentData().isValid() ? m_source->currentData().toInt() : -1;
    const int destCurrent = m_dest->currentData().isValid() ? m_dest->currentData().toInt() : -1;
    {
        const QSignalBlocker blockerSource(m_source);
        const QSignalBlocker blockerDest(m_dest);
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
}

void BikeWindow::rebuildTargets() {
    QVector<int> previous;
    bool hadItems = false;
    for (int i = 0; i < m_targets->count(); ++i) {
        const QListWidgetItem* item = m_targets->item(i);
        if (!item->data(Qt::UserRole).isValid()) {
            continue;  // 跳过“路网为空”的占位行
        }
        hadItems = true;
        if (item->checkState() == Qt::Checked) {
            previous.push_back(item->data(Qt::UserRole).toInt());
        }
    }

    {
        const QSignalBlocker blocker(m_targets);
        m_targets->clear();
        for (int i = 0; i < m_system.stationCount(); ++i) {
            const ds::Station& s = m_system.stationAt(i);
            auto* item = new QListWidgetItem(
                QString("%1 %2").arg(s.id).arg(QString::fromUtf8(s.name)));
            item->setData(Qt::UserRole, s.id);
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            const bool checked = hadItems ? previous.contains(s.id)
                                          : (s.id == 3 || s.id == 5 || s.id == 7);
            item->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
            item->setFont(ef::font(ef::FontRole::Field));
            m_targets->addItem(item);
        }
    }
    updateTargetStyles();
    updateSelectionView();
}

void BikeWindow::updateTargetStyles() {
    const QSignalBlocker blocker(m_targets);
    for (int i = 0; i < m_targets->count(); ++i) {
        QListWidgetItem* item = m_targets->item(i);
        if (!item->data(Qt::UserRole).isValid()) {
            item->setForeground(QBrush(ef::palette().ink2));
            continue;
        }
        const bool checked = item->checkState() == Qt::Checked;
        QFont f = ef::font(ef::FontRole::Field);
        f.setWeight(checked ? QFont::DemiBold : QFont::Normal);
        item->setFont(f);
    }
    m_targets->viewport()->update();
}

void BikeWindow::updateSelectionView() {
    const int depot =
        m_source->currentData().isValid() ? m_source->currentData().toInt() : -1;
    {
        const QSignalBlocker blocker(m_targets);
        for (int i = 0; i < m_targets->count(); ++i) {
            QListWidgetItem* item = m_targets->item(i);
            if (!item->data(Qt::UserRole).isValid()) {
                continue;
            }
            const bool isDepot = item->data(Qt::UserRole).toInt() == depot;
            Qt::ItemFlags flags = item->flags();
            if (isDepot) {
                flags &= ~(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
                if (item->checkState() != Qt::Unchecked) {
                    item->setCheckState(Qt::Unchecked);
                }
            } else {
                flags |= Qt::ItemIsUserCheckable | Qt::ItemIsEnabled;
            }
            item->setFlags(flags);
        }
    }
    updateTargetStyles();

    QVector<int> targets;
    for (int i = 0; i < m_targets->count(); ++i) {
        const QListWidgetItem* item = m_targets->item(i);
        if (!item->data(Qt::UserRole).isValid()) {
            continue;
        }
        if (item->checkState() == Qt::Checked) {
            targets.push_back(item->data(Qt::UserRole).toInt());
        }
    }
    m_canvas->setDepot(depot);
    m_canvas->setTargets(targets);
    if (m_contextFields != nullptr) {
        m_contextFields->setText(ef::fields({
            QString("权重 %1").arg(m_mode->currentText()),
            QString("总站 %1").arg(depot > 0 ? QString::number(depot) : QStringLiteral("—")),
            QString("目标 %1").arg(targets.size()),
        }));
    }
    if (m_canvas != nullptr) {
        m_canvas->setAccessibleDescription(
            QString("站点 %1 个，道路 %2 条，总站 %3，已选目标 %4 个%5。")
                .arg(m_system.stationCount())
                .arg(m_system.roads().size())
                .arg(depot > 0 ? QString::number(depot) : QStringLiteral("未选择"))
                .arg(targets.size())
                .arg(m_path.isEmpty()
                         ? QString()
                         : QString("，当前路线经过 %1 站").arg(m_path.size())));
    }
}

void BikeWindow::updateMetrics() {
    int targetCount = 0;
    for (int i = 0; i < m_targets->count(); ++i) {
        if (m_targets->item(i)->checkState() == Qt::Checked) {
            ++targetCount;
        }
    }
    const int alertCount = m_system.alerts().size();
    if (m_metrics != nullptr) {
        m_metrics->setMetrics({
            {QStringLiteral("STATIONS"), QString::number(m_system.stationCount()),
             QStringLiteral("个"), QStringLiteral("站点"), false},
            {QStringLiteral("ROADS"), QString::number(m_system.roads().size()),
             QStringLiteral("条"), QStringLiteral("道路"), false},
            {QStringLiteral("TARGETS"), QString::number(targetCount), QStringLiteral("站"),
             QStringLiteral("调度目标"), false},
            {QStringLiteral("ALERTS"), QString::number(alertCount), QStringLiteral("条"),
             QStringLiteral("调度告警"), alertCount > 0},
        });
    }
    if (m_bar != nullptr) {
        m_bar->setFields(ef::fields({
            QString("站点 %1").arg(m_system.stationCount()),
            QString("道路 %1").arg(m_system.roads().size()),
            QString("调度告警 %1").arg(alertCount),
        }));
    }
    if (m_contextFields != nullptr) {
        const int depot =
            m_source->currentData().isValid() ? m_source->currentData().toInt() : -1;
        m_contextFields->setText(ef::fields({
            QString("权重 %1").arg(m_mode->currentText()),
            QString("总站 %1").arg(depot > 0 ? QString::number(depot) : QStringLiteral("—")),
            QString("目标 %1").arg(targetCount),
        }));
    }
}

void BikeWindow::refreshStations() {
    m_stationTable->setRowCount(m_system.stationCount());
    for (int i = 0; i < m_system.stationCount(); ++i) {
        const ds::Station& s = m_system.stationAt(i);
        auto* idItem = new QTableWidgetItem(QString::number(s.id));
        idItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        idItem->setFont(ef::font(ef::FontRole::Field));
        auto* nameItem = new QTableWidgetItem(QString::fromUtf8(s.name));
        nameItem->setFont(ef::font(ef::FontRole::Label));
        auto* bikesItem = new QTableWidgetItem(QString::number(s.bikes));
        bikesItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        bikesItem->setFont(ef::font(ef::FontRole::Field));
        auto* capItem = new QTableWidgetItem(QString::number(s.capacity));
        capItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        capItem->setFont(ef::font(ef::FontRole::Field));

        const int low = s.capacity * m_lowPercent->value() / 100;
        const int high = s.capacity * m_highPercent->value() / 100;
        const bool lowAlert = s.bikes < low;
        const bool highAlert = s.bikes > high;
        const QString statusText = lowAlert ? QStringLiteral("偏低")
                                  : highAlert ? QStringLiteral("偏高")
                                              : QStringLiteral("正常");

        m_stationTable->setItem(i, 0, idItem);
        m_stationTable->setItem(i, 1, nameItem);
        m_stationTable->setItem(i, 2, bikesItem);
        m_stationTable->setItem(i, 3, capItem);
        m_stationTable->setRowHeight(i, 22);
        m_stationTable->removeCellWidget(i, 4);
        if (lowAlert || highAlert) {
            auto* cell = new QLabel(statusText, m_stationTable);
            cell->setObjectName(QStringLiteral("efAlertCell"));
            cell->setAlignment(Qt::AlignCenter);
            QFont f = ef::font(ef::FontRole::Field);
            f.setWeight(QFont::Bold);
            cell->setFont(f);
            cell->setToolTip(lowAlert
                                 ? QString("低于下限：%1 / %2").arg(s.bikes).arg(low)
                                 : QString("高于上限：%1 / %2").arg(s.bikes).arg(high));
            m_stationTable->setCellWidget(i, 4, cell);
        } else {
            auto* statusItem = new QTableWidgetItem(statusText);
            statusItem->setTextAlignment(Qt::AlignCenter);
            statusItem->setFont(ef::font(ef::FontRole::Field));
            m_stationTable->setItem(i, 4, statusItem);
        }
    }
}

void BikeWindow::refreshEdges() {
    const QString current =
        m_edge->currentData().isValid() ? m_edge->currentData().toString() : QString();
    const QSignalBlocker blocker(m_edge);
    m_edge->clear();
    if (m_system.roads().size() == 0) {
        m_edge->addItem(QStringLiteral("// 暂无道路"));
        m_edge->setEnabled(false);
        m_edge->setToolTip(QStringLiteral("先加载示例地图或新增道路"));
        return;
    }
    m_edge->setEnabled(true);
    for (int i = 0; i < m_system.roads().size(); ++i) {
        const ds::Road& r = m_system.roads()[i];
        const bool active = m_system.hasActiveRoad(r.u, r.v);
        const QString text = QString("%1 - %2  │  %3 m / %4 min%5")
                                 .arg(r.u)
                                 .arg(r.v)
                                 .arg(r.dist, 0, 'f', 0)
                                 .arg(r.time, 0, 'f', 0)
                                 .arg(active ? QString() : QStringLiteral("  [阻断]"));
        m_edge->addItem(text, QString("%1,%2").arg(r.u).arg(r.v));
    }
    if (!current.isEmpty()) {
        const int idx = m_edge->findData(current);
        if (idx >= 0) {
            m_edge->setCurrentIndex(idx);
        }
    }
    m_edge->setToolTip(m_edge->currentText());
}

void BikeWindow::refreshAlerts() {
    const ef::Palette& c = ef::palette();
    m_alerts->clear();
    if (m_system.alerts().empty()) {
        auto* item = new QListWidgetItem(
            QStringLiteral("// 当前没有调度告警。站点库存异常显示在站点状态表。"));
        item->setFlags(Qt::NoItemFlags);
        item->setForeground(QBrush(c.ink2));
        item->setFont(ef::font(ef::FontRole::Field));
        m_alerts->addItem(item);
        return;
    }
    for (int i = 0; i < m_system.alerts().size(); ++i) {
        const ds::Alert& a = m_system.alerts()[i];
        auto* item = new QListWidgetItem(
            QString("!  [%1] %2").arg(a.stationId).arg(QString::fromUtf8(a.text)));
        item->setBackground(QBrush(c.signal));
        item->setFont(ef::font(ef::FontRole::Field));
        item->setToolTip(item->text());
        m_alerts->addItem(item);
    }
}

QString BikeWindow::stationLabel(int id) const {
    const ds::Station* s = m_system.findStation(id);
    if (s == nullptr) {
        return QString::number(id);
    }
    return QString("%1 %2").arg(id).arg(QString::fromUtf8(s->name));
}

void BikeWindow::addStationDialog() {
    QDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("新增站点"));
    dialog.setMinimumWidth(440);
    auto* layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(18, 16, 18, 16);
    layout->setSpacing(10);
    layout->addWidget(ef::makeEyebrow(QStringLiteral("ADD STATION"), &dialog));
    layout->addWidget(ef::makeTitle(QStringLiteral("新增站点"), &dialog));
    layout->addWidget(ef::makeRule(&dialog));

    int maxId = 0;
    for (int i = 0; i < m_system.stationCount(); ++i) {
        maxId = qMax(maxId, m_system.stationAt(i).id);
    }

    auto* form = new QFormLayout;
    form->setHorizontalSpacing(12);
    form->setVerticalSpacing(6);
    auto* idSpin = new QSpinBox(&dialog);
    idSpin->setRange(1, 999999);
    idSpin->setValue(maxId + 1);
    auto* nameEdit = new QLineEdit(&dialog);
    nameEdit->setText(QString("新站点%1").arg(maxId + 1));
    auto* bikesSpin = new QSpinBox(&dialog);
    bikesSpin->setRange(0, 9999);
    bikesSpin->setValue(5);
    auto* capacitySpin = new QSpinBox(&dialog);
    capacitySpin->setRange(1, 9999);
    capacitySpin->setValue(20);
    auto* xSpin = new QDoubleSpinBox(&dialog);
    xSpin->setRange(-10000, 10000);
    xSpin->setValue(500);
    auto* ySpin = new QDoubleSpinBox(&dialog);
    ySpin->setRange(-10000, 10000);
    ySpin->setValue(350);
    form->addRow(QStringLiteral("站点编号"), idSpin);
    form->addRow(QStringLiteral("站点名称"), nameEdit);
    form->addRow(QStringLiteral("现有车辆"), bikesSpin);
    form->addRow(QStringLiteral("最大容量"), capacitySpin);
    form->addRow(QStringLiteral("X 坐标"), xSpin);
    form->addRow(QStringLiteral("Y 坐标"), ySpin);
    layout->addLayout(form);

    auto* actions = new QHBoxLayout;
    actions->addStretch(1);
    auto* cancel = new ef::EfButton(QStringLiteral("取消"), ef::EfButton::Variant::Secondary,
                                    &dialog);
    auto* ok = new ef::EfButton(QStringLiteral("新增"), ef::EfButton::Variant::Primary,
                                &dialog);
    actions->addWidget(cancel);
    actions->addWidget(ok);
    layout->addLayout(actions);
    connect(cancel, &QPushButton::clicked, &dialog, &QDialog::reject);
    connect(ok, &QPushButton::clicked, &dialog, &QDialog::accept);
    ok->setDefault(true);
    cancel->setAutoDefault(false);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    char err[160] = {0};
    if (!m_system.addStation(idSpin->value(), nameEdit->text().toUtf8().constData(),
                             bikesSpin->value(), capacitySpin->value(), xSpin->value(),
                             ySpin->value(), err, sizeof(err))) {
        setStatus(QString::fromUtf8(err));
        return;
    }
    invalidateRoute(QStringLiteral("路网已变化，原路线已失效，请重新规划。"));
    m_canvas->setSystem(&m_system);
    refreshSources();
    rebuildTargets();
    refreshStations();
    refreshEdges();
    refreshAll();
    setStatus(QString("已新增站点 %1").arg(idSpin->value()));
}

void BikeWindow::addRoadDialog() {
    if (m_system.stationCount() < 2) {
        setStatus(QStringLiteral("至少需要两个站点才能新增道路"));
        return;
    }
    QDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("新增道路"));
    dialog.setMinimumWidth(440);
    auto* layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(18, 16, 18, 16);
    layout->setSpacing(10);
    layout->addWidget(ef::makeEyebrow(QStringLiteral("ADD ROAD"), &dialog));
    layout->addWidget(ef::makeTitle(QStringLiteral("新增道路"), &dialog));
    layout->addWidget(ef::makeRule(&dialog));

    auto* form = new QFormLayout;
    form->setHorizontalSpacing(12);
    form->setVerticalSpacing(6);
    auto* uCombo = new QComboBox(&dialog);
    auto* vCombo = new QComboBox(&dialog);
    for (int i = 0; i < m_system.stationCount(); ++i) {
        const ds::Station& s = m_system.stationAt(i);
        const QString label = QString("%1 %2").arg(s.id).arg(QString::fromUtf8(s.name));
        uCombo->addItem(label, s.id);
        vCombo->addItem(label, s.id);
    }
    if (vCombo->count() > 1) {
        vCombo->setCurrentIndex(1);
    }
    auto* distSpin = new QDoubleSpinBox(&dialog);
    distSpin->setRange(1.0, 1000000.0);
    distSpin->setDecimals(1);
    distSpin->setValue(500.0);
    distSpin->setSuffix(QStringLiteral(" m"));
    auto* timeSpin = new QDoubleSpinBox(&dialog);
    timeSpin->setRange(0.1, 100000.0);
    timeSpin->setDecimals(1);
    timeSpin->setValue(5.0);
    timeSpin->setSuffix(QStringLiteral(" min"));
    form->addRow(QStringLiteral("端点 A"), uCombo);
    form->addRow(QStringLiteral("端点 B"), vCombo);
    form->addRow(QStringLiteral("距离"), distSpin);
    form->addRow(QStringLiteral("通行耗时"), timeSpin);
    layout->addLayout(form);

    auto* actions = new QHBoxLayout;
    actions->addStretch(1);
    auto* cancel = new ef::EfButton(QStringLiteral("取消"), ef::EfButton::Variant::Secondary,
                                    &dialog);
    auto* ok = new ef::EfButton(QStringLiteral("新增"), ef::EfButton::Variant::Primary,
                                &dialog);
    actions->addWidget(cancel);
    actions->addWidget(ok);
    layout->addLayout(actions);
    connect(cancel, &QPushButton::clicked, &dialog, &QDialog::reject);
    connect(ok, &QPushButton::clicked, &dialog, &QDialog::accept);
    ok->setDefault(true);
    cancel->setAutoDefault(false);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    char err[160] = {0};
    if (!m_system.addRoad(uCombo->currentData().toInt(), vCombo->currentData().toInt(),
                          distSpin->value(), timeSpin->value(), err, sizeof(err))) {
        setStatus(QString::fromUtf8(err));
        return;
    }
    invalidateRoute(QStringLiteral("路网已变化，原路线已失效，请重新规划。"));
    refreshEdges();
    refreshAll();
    setStatus(QString("已新增道路 %1 - %2")
                  .arg(uCombo->currentData().toInt())
                  .arg(vCombo->currentData().toInt()));
}

void BikeWindow::queryPair() {
    if (m_system.stationCount() == 0) {
        setStatus(QStringLiteral("请先加载路网"));
        return;
    }
    const int src = m_source->currentData().toInt();
    const int dst = m_dest->currentData().toInt();
    const auto mode = m_mode->currentIndex() == 1 ? ds::WeightMode::Time
                                                  : ds::WeightMode::Distance;
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
    m_canvas->setDepot(src);
    updateSelectionView();
    m_truckTimer->stop();
    m_canvas->setTruckProgress(-1.0);

    QString pathText;
    for (int i = 0; i < path.size(); ++i) {
        if (i > 0) {
            pathText += QStringLiteral(" → ");
        }
        pathText += stationLabel(path[i]);
    }
    const QString totalText = (mode == ds::WeightMode::Time)
                                  ? QString("%1 分钟").arg(total, 0, 'f', 1)
                                  : QString("%1 m").arg(total, 0, 'f', 0);
    m_routeText->setPlainText(
        QString("// SHORTEST PATH\n%1\n\n%2\n\n%3")
            .arg(pathText)
            .arg(ef::fields({QString("权重 %1").arg(m_mode->currentText()),
                             QString("途经 %1 站").arg(path.size())}))
            .arg(ef::fields({QString("总权重 %1").arg(totalText)})));
    setStatus(QString("查询完成：%1 个途经站点").arg(path.size()));
}

void BikeWindow::checkReachability() {
    if (m_system.stationCount() == 0) {
        return;
    }
    const int depot = m_source->currentData().toInt();
    QString text = QString("// REACHABILITY\n从 %1 出发：\n").arg(stationLabel(depot));
    int count = 0;
    for (int i = 0; i < m_targets->count(); ++i) {
        const QListWidgetItem* item = m_targets->item(i);
        if (item->checkState() != Qt::Checked) {
            continue;
        }
        const int id = item->data(Qt::UserRole).toInt();
        const bool ok = m_system.connected(depot, id);
        text += QString("│ %1：%2\n").arg(stationLabel(id)).arg(ok ? QStringLiteral("可达")
                                                                  : QStringLiteral("不可达"));
        ++count;
    }
    if (count == 0) {
        text += QStringLiteral("（尚未勾选目标站点）\n");
    }
    m_routeText->appendPlainText("\n" + text);
    m_routeText->verticalScrollBar()->setValue(
        m_routeText->verticalScrollBar()->maximum());
    setStatus(QStringLiteral("可达性检查完成"));
}

void BikeWindow::planRoute() {
    if (m_system.stationCount() == 0) {
        setStatus(QStringLiteral("请先加载路网"));
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
        setStatus(QStringLiteral("请至少勾选一个目标站点"));
        return;
    }

    const auto mode = m_mode->currentIndex() == 1 ? ds::WeightMode::Time
                                                  : ds::WeightMode::Distance;
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
    m_canvas->setDepot(depot);
    updateSelectionView();

    QString order;
    for (int i = 0; i < tour.size(); ++i) {
        if (i > 0) {
            order += QStringLiteral(" → ");
        }
        order += stationLabel(tour[i]);
    }
    QString pathText;
    for (int i = 0; i < full.size(); ++i) {
        if (i > 0) {
            pathText += QStringLiteral(" → ");
        }
        pathText += QString::number(full[i]);
    }
    const QString totalText = (mode == ds::WeightMode::Time)
                                  ? QString("%1 分钟").arg(total, 0, 'f', 1)
                                  : QString("%1 m（约 %2 km）")
                                        .arg(total, 0, 'f', 0)
                                        .arg(total / 1000.0, 0, 'f', 2);
    m_routeText->setPlainText(
        QString("// PLAN\n%1\n\n// PATH\n%2\n\n%3")
            .arg(order)
            .arg(pathText)
            .arg(ef::fields({QString("总权重 %1").arg(totalText),
                             QString("目标 %1 站").arg(targets.size())})));
    m_truckTimer->stop();
    m_canvas->setTruckProgress(-1.0);
    setStatus(QString("路线规划完成：%1 个站点").arg(tour.size()));
}

void BikeWindow::simulateDispatch() {
    if (m_tour.size() < 2) {
        setStatus(QStringLiteral("请先规划调度路线"));
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
        updateMetrics();
        return;
    }

    QString summary = QString::fromUtf8(report.message);
    summary.replace(QStringLiteral("，总耗时"), QStringLiteral("\n总耗时"));
    m_routeText->appendPlainText(QString("\n\n// DISPATCH\n%1").arg(summary));
    for (int i = 0; i < report.stops.size(); ++i) {
        const ds::StopRecord& s = report.stops[i];
        m_routeText->appendPlainText(
            QString("%1  │  %2 → %3  │  卸 %4 / 装 %5  │  车载 %6%7")
                .arg(stationLabel(s.stationId))
                .arg(s.before)
                .arg(s.after)
                .arg(s.unloaded)
                .arg(s.loaded)
                .arg(s.load)
                .arg(s.alert ? QStringLiteral("  !") : QString()));
    }
    m_routeText->verticalScrollBar()->setValue(
        m_routeText->verticalScrollBar()->maximum());

    m_progress = 0.0;
    m_canvas->setTruckProgress(0.0);
    if (ef::reduceMotion()) {
        m_progress = 1.0;
        m_canvas->setTruckProgress(1.0);
    } else {
        m_truckTimer->start();
    }
    refreshStations();
    refreshAlerts();
    updateMetrics();
    setStatus(QString::fromUtf8(report.message));
}

void BikeWindow::blockSelected() {
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
        setStatus(QStringLiteral("该道路已阻断或不存在"));
        return;
    }
    m_tour.clear();
    m_path.clear();
    m_canvas->clearPath();
    m_truckTimer->stop();
    refreshEdges();
    refreshAll();
    m_routeText->appendPlainText(QStringLiteral("\n// ROAD BLOCK\n道路已阻断，请重新规划路线。"));
    checkReachability();
    setStatus(QString("已阻断道路 %1 - %2").arg(u).arg(v));
}

void BikeWindow::restoreSelected() {
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
        setStatus(QStringLiteral("该道路不存在"));
        return;
    }
    refreshEdges();
    refreshAll();
    checkReachability();
    setStatus(QString("已恢复道路 %1 - %2").arg(u).arg(v));
}

void BikeWindow::animateTruck() {
    m_progress += 0.006;
    if (m_progress >= 1.0) {
        m_progress = 1.0;
        m_truckTimer->stop();
    }
    m_canvas->setTruckProgress(m_progress);
}

void BikeWindow::toggleTheme() {
    ef::setDark(!ef::isDark());
    ef::apply(*qApp);
    refreshTheme();
    setStatus(ef::isDark() ? QStringLiteral("已切换到深色主题")
                           : QStringLiteral("已切换到浅色主题"));
}

void BikeWindow::refreshTheme() {
    if (m_bar != nullptr) {
        m_bar->themeButton()->setText(ef::isDark() ? QStringLiteral("浅色")
                                                   : QStringLiteral("深色"));
    }
    refreshAll();
    update();
}

void BikeWindow::suspend() {
    if (m_truckTimer != nullptr) {
        m_truckTimer->stop();
    }
}

void BikeWindow::setStatus(const QString& text) {
    if (m_status != nullptr) {
        m_status->showMessage(text);
    }
}
