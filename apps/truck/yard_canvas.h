#pragma once

#include <QHash>
#include <QPointF>
#include <QWidget>

#include "ds/truck_yard.h"

class QPainter;
class QTimer;

// 题 2 的场景画布：主通道（栈）、临时区（栈）、三条优先级队列。
class YardCanvas : public QWidget {
    Q_OBJECT
public:
    explicit YardCanvas(QWidget* parent = nullptr);

    void setYard(ds::TruckYard* yard);
    void resetAnimation();

    QSize sizeHint() const override { return QSize(900, 560); }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QPointF targetFor(int index) const;
    int channelX() const { return 294; }
    int channelY() const { return 96; }
    int slotH() const { return 54; }
    int gap() const { return 8; }
    int slotW() const;
    int tempY() const { return channelY() + slotH() + 80; }
    int laneY(int type) const { return 250 + type * 100; }
    void paintTruck(QPainter& p, const QPointF& pos, const ds::Truck& t, bool small);

    ds::TruckYard* m_yard = nullptr;
    QHash<int, QPointF> m_pos;
    QTimer* m_timer = nullptr;
};
