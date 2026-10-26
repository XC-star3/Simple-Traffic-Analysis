#pragma once

#include <QMainWindow>

#include "ds/truck_yard.h"

class QLabel;
class QPlainTextEdit;
class QSpinBox;
class QDoubleSpinBox;
class QComboBox;
class QPushButton;
class QTimer;
class YardCanvas;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    void runDemo();

private:
    void buildUi();
    void applyConfig();
    void doArrive();
    void doDepart();
    void autoTick();
    void toggleAuto();
    void resetAll();
    void refreshAll();
    void syncEvents();
    void setStatus(const QString& text);

    ds::TruckYard m_yard;
    YardCanvas* m_canvas = nullptr;

    QSpinBox* m_capacity = nullptr;
    QSpinBox* m_tempCapacity = nullptr;
    QDoubleSpinBox* m_fee = nullptr;
    QSpinBox* m_arriveId = nullptr;
    QComboBox* m_type = nullptr;
    QSpinBox* m_departId = nullptr;
    QDoubleSpinBox* m_nowSpin = nullptr;
    QSpinBox* m_interval = nullptr;
    QPushButton* m_autoButton = nullptr;
    QPlainTextEdit* m_log = nullptr;

    QLabel* m_statRevenue = nullptr;
    QLabel* m_statChannel = nullptr;
    QLabel* m_statTemp = nullptr;
    QLabel* m_statQueues = nullptr;
    QLabel* m_statCounts = nullptr;
    QLabel* m_statPeak = nullptr;

    QTimer* m_autoTimer = nullptr;
    double m_now = 0.0;
    int m_nextId = 1001;
    int m_lastEvents = 0;
};
