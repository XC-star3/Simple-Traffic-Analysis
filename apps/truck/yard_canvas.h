#pragma once

#include <QHash>
#include <QPointF>
#include <QWidget>

#include "ds/truck_yard.h"

class QPainter;
class QTimer;

// 题 2 的场景画布：主通道（栈）、临时区（栈）、三条优先级队列。
class YardCanvas : public QWidget {
public:
    explicit YardCanvas(QWidget* parent = nullptr);

    void setYard(ds::TruckYard* yard);
    void resetAnimation();

    QSize sizeHint() const override { return QSize(900, 560); }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QPointF targetFor(int index) const;
    int verticalShift() const;
    int channelX() const { return 248; }
    int channelY() const { return 50 + verticalShift(); }
    int slotH() const { return 44; }
    int gap() const { return 6; }
    int slotW() const;
    int tempY() const { return 140 + verticalShift(); }
    int laneY(int type) const { return 190 + verticalShift() + type * 74; }
    int laneH() const { return 70; }
    void paintTruck(QPainter& p, const QPointF& pos, const ds::Truck& t, bool small);

    ds::TruckYard* m_yard = nullptr;
    QHash<int, QPointF> m_pos;
    QTimer* m_timer = nullptr;
};
