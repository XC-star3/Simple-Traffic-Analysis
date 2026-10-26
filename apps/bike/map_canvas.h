#pragma once

#include <QPointF>
#include <QRectF>
#include <QVector>
#include <QWidget>

#include "ds/bike_system.h"

class QMouseEvent;
class QPaintEvent;

class MapCanvas : public QWidget {
    Q_OBJECT
public:
    explicit MapCanvas(QWidget* parent = nullptr);

    void setSystem(ds::BikeSystem* system);
    void setPath(const QVector<int>& path);
    void setTruckProgress(double t);  // 0..1，<0 隐藏
    void clearPath();

    QSize sizeHint() const override { return QSize(900, 700); }

signals:
    void stationClicked(int id);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private:
    QPointF screenPos(const ds::Station& s) const;
    QRectF contentBounds() const;
    void updateTransform();
    bool onPath(int u, int v) const;

    ds::BikeSystem* m_system = nullptr;
    QVector<int> m_path;
    double m_truckT = -1.0;
    double m_scale = 1.0;
    QPointF m_offset;
};
