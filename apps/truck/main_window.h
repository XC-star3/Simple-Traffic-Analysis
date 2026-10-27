#pragma once

#include <QWidget>

#include "ds/truck_yard.h"

class QLabel;
class QListWidget;
class QSpinBox;
class QDoubleSpinBox;
class QComboBox;
class QPushButton;
class QTimer;

namespace ef {
class EfAlertBar;
class EfCommandBar;
class EfMetrics;
class EfStatusStrip;
}  // namespace ef

class YardCanvas;

class TruckWindow : public QWidget {
public:
    explicit TruckWindow(QWidget* parent = nullptr);
    void runDemo();
    void runWarningDemo();
    void runLetWayDemo();

    ef::EfCommandBar* commandBar() const { return m_bar; }
    void suspend();
    void refreshTheme();

private:
    void buildUi();
    void applyConfig();
    void doArrive();
    void doDepart(bool fromAuto = false);
    void autoTick(bool randomEvent = true);
    void toggleAuto();
    void resetAll();
    void refreshAll();
    void syncEvents();
    void toggleTheme();
    void setStatus(const QString& text);
    void showTempWarning(int id);
    void showConfigWarning();
    void finishLetWay();
    void setBusy(bool busy);

    ds::TruckYard m_yard;
    YardCanvas* m_canvas = nullptr;
    ef::EfCommandBar* m_bar = nullptr;
    ef::EfMetrics* m_metrics = nullptr;
    ef::EfAlertBar* m_alert = nullptr;
    ef::EfStatusStrip* m_status = nullptr;
    QLabel* m_queueFields = nullptr;

    QSpinBox* m_capacity = nullptr;
    QSpinBox* m_tempCapacity = nullptr;
    QDoubleSpinBox* m_fee = nullptr;
    QSpinBox* m_arriveId = nullptr;
    QComboBox* m_type = nullptr;
    QDoubleSpinBox* m_arriveTime = nullptr;
    QSpinBox* m_departId = nullptr;
    QDoubleSpinBox* m_nowSpin = nullptr;
    QSpinBox* m_interval = nullptr;
    QPushButton* m_autoButton = nullptr;
    QListWidget* m_log = nullptr;

    QPushButton* m_arriveButton = nullptr;
    QPushButton* m_departButton = nullptr;
    QPushButton* m_stepButton = nullptr;

    QTimer* m_autoTimer = nullptr;
    QTimer* m_departTimer = nullptr;
    int m_pendingId = 0;
    int m_departDelayMs = 520;
    double m_pendingFee = 0.0;
    double m_pendingDwell = 0.0;
    double m_now = 0.0;
    int m_nextId = 1001;
    long long m_lastEventSeq = -1;
};
