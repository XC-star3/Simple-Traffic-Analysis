#pragma once

#include <QMainWindow>

#include "ds/truck_yard.h"

class QLabel;
class QListWidget;
class QSpinBox;
class QDoubleSpinBox;
class QComboBox;
class QPushButton;
class QTimer;

namespace ef {
class EfCommandBar;
class EfMetrics;
}  // namespace ef

class YardCanvas;

class MainWindow : public QMainWindow {
public:
    explicit MainWindow(QWidget* parent = nullptr);
    void runDemo();

private:
    void buildUi();
    void applyConfig();
    void doArrive();
    void doDepart();
    void autoTick(bool randomEvent = true);
    void toggleAuto();
    void resetAll();
    void refreshAll();
    void syncEvents();
    void toggleTheme();
    void setStatus(const QString& text);

    ds::TruckYard m_yard;
    YardCanvas* m_canvas = nullptr;
    ef::EfCommandBar* m_bar = nullptr;
    ef::EfMetrics* m_metrics = nullptr;
    QLabel* m_queueFields = nullptr;

    QSpinBox* m_capacity = nullptr;
    QSpinBox* m_tempCapacity = nullptr;
    QDoubleSpinBox* m_fee = nullptr;
    QSpinBox* m_arriveId = nullptr;
    QComboBox* m_type = nullptr;
    QSpinBox* m_departId = nullptr;
    QDoubleSpinBox* m_nowSpin = nullptr;
    QSpinBox* m_interval = nullptr;
    QPushButton* m_autoButton = nullptr;
    QListWidget* m_log = nullptr;

    QTimer* m_autoTimer = nullptr;
    double m_now = 0.0;
    int m_nextId = 1001;
    long long m_lastEventSeq = -1;
};
