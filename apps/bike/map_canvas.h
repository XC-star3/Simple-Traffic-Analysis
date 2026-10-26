#pragma once

#include <functional>

#include <QPointF>
#include <QRectF>
#include <QVector>
#include <QWidget>

#include "ds/bike_system.h"

class QMouseEvent;
class QPaintEvent;

class MapCanvas : public QWidget {
public:
    explicit MapCanvas(QWidget* parent = nullptr);

    void setSystem(ds::BikeSystem* system);
    void setPath(const QVector<int>& path);
    void setTruckProgress(double t);  // 0..1，<0 隐藏
    void clearPath();
    void setDepot(int id);
    void setTargets(const QVector<int>& ids);
    void setThresholds(int lowPercent, int highPercent);
    void setShowWeights(bool show);
    void setStationClickedHandler(std::function<void(int)> handler);

    QSize sizeHint() const override { return QSize(900, 700); }

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    QPointF screenPos(const ds::Station& s) const;
    QRectF contentBounds() const;
    void updateTransform();
    int hitStation(const QPointF& pos);
    bool onPath(int u, int v) const;
    bool alert(const ds::Station& s) const;

    ds::BikeSystem* m_system = nullptr;
    QVector<int> m_path;
    QVector<int> m_targets;
    int m_depot = -1;
    int m_lowPercent = 20;
    int m_highPercent = 80;
    bool m_showWeights = false;
    int m_hoverId = -1;
    double m_truckT = -1.0;
    double m_scale = 1.0;
    QPointF m_offset;
    std::function<void(int)> m_stationClicked;
};
