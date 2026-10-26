#pragma once

#include <QMainWindow>
#include <QVector>

#include "ds/bike_system.h"

class QComboBox;
class QCheckBox;
class QLabel;
class QListWidget;
class QPlainTextEdit;
class QTableWidget;
class QPushButton;
class QSpinBox;
class QTimer;

namespace ef {
class EfCommandBar;
class EfMetrics;
}  // namespace ef

class MapCanvas;

class MainWindow : public QMainWindow {
public:
    explicit MainWindow(QWidget* parent = nullptr);
    void runDemo();

private:
    void buildUi();
    void loadDemo();
    void confirmClearAll();
    void clearAll();
    void invalidateRoute(const QString& reason);
    void addStationDialog();
    void addRoadDialog();
    void refreshAll();
    void refreshSources();
    void rebuildTargets();
    void refreshStations();
    void refreshEdges();
    void refreshAlerts();
    void updateMetrics();
    void updateSelectionView();
    void updateTargetStyles();
    void planRoute();
    void queryPair();
    void checkReachability();
    void simulateDispatch();
    void blockSelected();
    void restoreSelected();
    void animateTruck();
    void toggleTheme();
    void setStatus(const QString& text);
    QString stationLabel(int id) const;

    ds::BikeSystem m_system;
    MapCanvas* m_canvas = nullptr;
    ef::EfCommandBar* m_bar = nullptr;
    ef::EfMetrics* m_metrics = nullptr;
    QLabel* m_contextFields = nullptr;

    QComboBox* m_mode = nullptr;
    QComboBox* m_source = nullptr;
    QComboBox* m_dest = nullptr;
    QListWidget* m_targets = nullptr;
    QComboBox* m_edge = nullptr;
    QCheckBox* m_showWeights = nullptr;
    QTableWidget* m_stationTable = nullptr;
    QPlainTextEdit* m_routeText = nullptr;
    QListWidget* m_alerts = nullptr;
    QSpinBox* m_truckCapacity = nullptr;
    QSpinBox* m_lowPercent = nullptr;
    QSpinBox* m_highPercent = nullptr;

    QVector<int> m_tour;
    QVector<int> m_path;
    QTimer* m_truckTimer = nullptr;
    double m_progress = 0.0;
};
