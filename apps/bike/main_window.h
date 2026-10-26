#pragma once

#include <QMainWindow>
#include <QVector>

#include "ds/bike_system.h"

class QComboBox;
class QListWidget;
class QTableWidget;
class QPlainTextEdit;
class QPushButton;
class QLabel;
class QSpinBox;
class QTimer;
class MapCanvas;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    void runDemo();

private:
    void buildUi();
    void loadDemo();
    void clearAll();
    void addStationDialog();
    void addRoadDialog();
    void refreshAll();
    void refreshSources();
    void rebuildTargets();
    void refreshStations();
    void refreshEdges();
    void refreshAlerts();
    void planRoute();
    void queryPair();
    void checkReachability();
    void simulateDispatch();
    void blockSelected();
    void restoreSelected();
    void animateTruck();
    void setStatus(const QString& text);
    QString stationLabel(int id) const;

    ds::BikeSystem m_system;
    MapCanvas* m_canvas = nullptr;

    QComboBox* m_mode = nullptr;
    QComboBox* m_source = nullptr;
    QComboBox* m_dest = nullptr;
    QListWidget* m_targets = nullptr;
    QComboBox* m_edge = nullptr;
    QTableWidget* m_stationTable = nullptr;
    QPlainTextEdit* m_routeText = nullptr;
    QListWidget* m_alerts = nullptr;
    QLabel* m_summary = nullptr;
    QSpinBox* m_truckCapacity = nullptr;
    QSpinBox* m_lowPercent = nullptr;
    QSpinBox* m_highPercent = nullptr;

    QVector<int> m_tour;
    QVector<int> m_path;
    QTimer* m_truckTimer = nullptr;
    double m_progress = 0.0;
};
