#include "map_canvas.h"

#include <QFont>
#include <QFontMetrics>
#include <QHash>
#include <QLineF>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QtGlobal>

#include <algorithm>
#include <cmath>
#include <utility>

#include "common/theme.h"

MapCanvas::MapCanvas(QWidget* parent) : QWidget(parent) {
    setMinimumSize(480, 300);
    setMouseTracking(true);
}

void MapCanvas::setSystem(ds::BikeSystem* system) {
    m_system = system;
    m_path.clear();
    m_targets.clear();
    m_depot = -1;
    m_hoverId = -1;
    m_truckT = -1.0;
    unsetCursor();
    update();
}

void MapCanvas::setPath(const QVector<int>& path) {
    m_path = path;
    m_truckT = -1.0;
    update();
}

void MapCanvas::setTruckProgress(double t) {
    m_truckT = t;
    update();
}

void MapCanvas::clearPath() {
    m_path.clear();
    m_truckT = -1.0;
    update();
}

void MapCanvas::setDepot(int id) {
    m_depot = id;
    update();
}

void MapCanvas::setTargets(const QVector<int>& ids) {
    m_targets = ids;
    update();
}

void MapCanvas::setThresholds(int lowPercent, int highPercent) {
    m_lowPercent = lowPercent;
    m_highPercent = highPercent;
    update();
}

void MapCanvas::setShowWeights(bool show) {
    m_showWeights = show;
    update();
}

void MapCanvas::setStationClickedHandler(std::function<void(int)> handler) {
    m_stationClicked = std::move(handler);
}

QRectF MapCanvas::contentBounds() const {
    if (m_system == nullptr || m_system->stationCount() == 0) {
        return QRectF(0, 0, 1000, 700);
    }
    double minX = 1e18;
    double minY = 1e18;
    double maxX = -1e18;
    double maxY = -1e18;
    for (int i = 0; i < m_system->stationCount(); ++i) {
        const ds::Station& s = m_system->stationAt(i);
        minX = std::min(minX, s.x);
        minY = std::min(minY, s.y);
        maxX = std::max(maxX, s.x);
        maxY = std::max(maxY, s.y);
    }
    const double pad = 70.0;
    return QRectF(minX - pad, minY - pad, (maxX - minX) + pad * 2,
                  (maxY - minY) + pad * 2);
}

void MapCanvas::updateTransform() {
    const QRectF b = contentBounds();
    const double sx = (width() - 80.0) / std::max(1.0, b.width());
    const double sy = (height() - 88.0) / std::max(1.0, b.height());
    m_scale = std::min(sx, sy);
    if (m_scale <= 0.0) {
        m_scale = 1.0;
    }
    m_offset = QPointF(width() / 2.0, height() / 2.0) -
               QPointF(b.center().x() * m_scale, b.center().y() * m_scale);
}

QPointF MapCanvas::screenPos(const ds::Station& s) const {
    return QPointF(s.x * m_scale + m_offset.x(), s.y * m_scale + m_offset.y());
}

bool MapCanvas::onPath(int u, int v) const {
    for (int i = 0; i + 1 < m_path.size(); ++i) {
        if ((m_path[i] == u && m_path[i + 1] == v) ||
            (m_path[i] == v && m_path[i + 1] == u)) {
            return true;
        }
    }
    return false;
}

bool MapCanvas::alert(const ds::Station& s) const {
    if (s.capacity <= 0) {
        return false;
    }
    const int low = s.capacity * m_lowPercent / 100;
    const int high = s.capacity * m_highPercent / 100;
    return s.bikes < low || s.bikes > high;
}

void MapCanvas::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    const ef::Palette& c = ef::palette();
    p.fillRect(rect(), c.panel);
    ef::drawContours(p, QRectF(rect()), c.ink, ef::isDark() ? 0.06 : 0.09);
    updateTransform();

    if (m_system == nullptr || m_system->stationCount() == 0) {
        const QRectF empty(36, 60, width() - 72.0, height() - 120.0);
        ef::drawHatch(p, empty, c.ink, 0.22);
        p.setPen(QPen(c.rule, 1.0));
        p.setBrush(Qt::NoBrush);
        p.drawRect(empty);
        p.setPen(c.ink);
        p.setFont(ef::font(ef::FontRole::Label));
        p.drawText(empty, Qt::AlignCenter,
                   QStringLiteral("还没有站点。先加载示例地图，或新增站点和道路。"));
        return;
    }

    // 站点方块可点击；把这一点直接写在画布上，不靠猜。
    const bool showPlates = width() >= 620 && height() >= 360;
    p.setFont(ef::font(ef::FontRole::Small));
    p.setPen(c.ink2);
    p.drawText(QRectF(18.0, 10.0, width() - 36.0, 14.0),
               Qt::AlignLeft | Qt::AlignVCenter,
               QStringLiteral("点击站点设为总站"));
    if (!showPlates) {
        p.drawText(QRectF(18.0, 10.0, width() - 36.0, 14.0),
                   Qt::AlignRight | Qt::AlignVCenter,
                   QStringLiteral("站名见右侧站点状态表"));
    }

    QHash<int, QPointF> pts;
    for (int i = 0; i < m_system->stationCount(); ++i) {
        const ds::Station& s = m_system->stationAt(i);
        pts.insert(s.id, screenPos(s));
    }

    const auto roadColor = [&]() {
        QColor color = c.ink2;
        color.setAlpha(ef::isDark() ? 120 : 95);
        return color;
    }();

    // 道路。
    for (int i = 0; i < m_system->roads().size(); ++i) {
        const ds::Road& r = m_system->roads()[i];
        if (!pts.contains(r.u) || !pts.contains(r.v)) {
            continue;
        }
        const QPointF a = pts.value(r.u);
        const QPointF b = pts.value(r.v);
        const bool active = m_system->hasActiveRoad(r.u, r.v);
        const bool highlight = onPath(r.u, r.v);

        QPen pen;
        if (highlight) {
            pen = QPen(c.trace, 4.0, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin);
        } else if (!active) {
            pen = QPen(c.ink, 1.5, Qt::DashLine);
        } else {
            pen = QPen(roadColor, 1.0);
        }
        p.setPen(pen);
        p.drawLine(a, b);

        if (!active) {
            const QPointF mid = (a + b) / 2.0;
            const QRectF tag(mid.x() - 19.0, mid.y() - 8.0, 38.0, 15.0);
            p.fillRect(tag, c.signal);
            p.setPen(c.onSignal);
            p.setFont(ef::font(ef::FontRole::Small));
            p.drawText(tag, Qt::AlignCenter, QStringLiteral("阻断"));
        } else if (m_showWeights && !highlight &&
                   QLineF(a, b).length() > 72.0) {
            const QFont f = ef::font(ef::FontRole::Field);
            const QFontMetrics fm(f);
            const QString text =
                QString("%1 m / %2 min").arg(r.dist, 0, 'f', 0).arg(r.time, 0, 'f', 0);
            const QPointF mid = (a + b) / 2.0;
            const QRectF bg(mid.x() + 4.0, mid.y() - 14.0,
                            fm.horizontalAdvance(text) + 6.0, 12.0);
            p.fillRect(bg, c.panel);
            p.setFont(f);
            p.setPen(c.ink2);
            p.drawText(bg.adjusted(3, 0, 0, 0), Qt::AlignLeft | Qt::AlignVCenter, text);
        }
    }

    // 调度路线：浅色用墨，深色用强调色。
    if (m_path.size() >= 2) {
        QPainterPath path;
        bool first = true;
        for (int id : m_path) {
            if (!pts.contains(id)) {
                continue;
            }
            const QPointF pt = pts.value(id);
            if (first) {
                path.moveTo(pt);
                first = false;
            } else {
                path.lineTo(pt);
            }
        }
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(c.panel, 7.0, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin));
        p.drawPath(path);
        p.setPen(QPen(c.trace, 4.0, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin));
        p.drawPath(path);

        // 每段路线中间画一个方向箭头，避免环线看不出行驶方向。
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(c.trace, 1.6, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin));
        for (int i = 0; i + 1 < m_path.size(); ++i) {
            if (!pts.contains(m_path[i]) || !pts.contains(m_path[i + 1])) {
                continue;
            }
            const QPointF a = pts.value(m_path[i]);
            const QPointF b = pts.value(m_path[i + 1]);
            const QLineF segment(a, b);
            if (segment.length() < 52.0) {
                continue;
            }
            const QPointF mid = segment.pointAt(0.55);
            const qreal angle = std::atan2(b.y() - a.y(), b.x() - a.x());
            const qreal size = 5.0;
            const qreal spread = 0.55;
            p.drawLine(mid, mid + QPointF(-std::cos(angle - spread) * size,
                                         -std::sin(angle - spread) * size));
            p.drawLine(mid, mid + QPointF(-std::cos(angle + spread) * size,
                                         -std::sin(angle + spread) * size));
        }
    }

    // 站点：可点击，所以切角；库存状态用克莱因蓝小旗，不用红绿。
    for (int i = 0; i < m_system->stationCount(); ++i) {
        const ds::Station& s = m_system->stationAt(i);
        const QPointF pt = screenPos(s);
        const qreal side = std::clamp(22.0 + s.capacity * 0.25, 26.0, 38.0);
        const QRectF box(pt.x() - side / 2.0, pt.y() - side / 2.0, side, side);
        const bool isDepot = s.id == m_depot;

        p.setBrush(isDepot ? c.signal : c.panel);
        p.setPen(QPen(c.ink, 1.5));
        p.drawPath(ef::cutPath(box, 7.0));

        p.setFont(ef::font(ef::FontRole::Numeral));
        p.setPen(isDepot ? c.onSignal : c.ink);
        p.drawText(box, Qt::AlignCenter, QString::number(s.id));

        if (showPlates) {
            const bool isAlert = alert(s);
            const int low = s.capacity * m_lowPercent / 100;
            const int high = s.capacity * m_highPercent / 100;
            const QString state = s.bikes < low ? QStringLiteral("偏低")
                                  : s.bikes > high ? QStringLiteral("偏高")
                                                   : QString();
            const QString line1 =
                QString("%1 %2").arg(s.id).arg(QString::fromUtf8(s.name));
            const QString line2 = QString("%1 / %2").arg(s.bikes).arg(s.capacity);
            QFont f1 = ef::font(ef::FontRole::Label);
            f1.setPixelSize(9);
            QFont f2 = ef::font(ef::FontRole::Field);
            f2.setPixelSize(8);
            const QFontMetrics fm1(f1);
            const QFontMetrics fm2(f2);
            const qreal textWidth =
                std::max(fm1.horizontalAdvance(line1),
                         fm2.horizontalAdvance(line2 + "  " + state));
            const qreal plateW = std::max<qreal>(74.0, textWidth + 12.0);
            const qreal plateH = 27.0;
            qreal plateX = box.right() + 5.0;
            if (plateX + plateW > width() - 8.0) {
                plateX = box.left() - plateW - 5.0;
            }
            const qreal plateY = box.center().y() - plateH / 2.0;
            const QRectF plate(plateX, plateY, plateW, plateH);
            p.fillRect(plate, c.panel);
            if (isAlert) {
                p.fillRect(QRectF(plate.left(), plate.top(), 3.0, plate.height()),
                           c.signal);
            }
            p.setPen(QPen(c.rule, 1.0));
            p.setBrush(Qt::NoBrush);
            p.drawRect(plate);

            const qreal textX = plate.left() + (isAlert ? 7.0 : 5.0);
            p.setFont(f1);
            p.setPen(c.ink);
            p.drawText(QPointF(textX, plate.top() + 11.0), line1);
            p.setFont(f2);
            p.setPen(c.ink2);
            p.drawText(QPointF(textX, plate.top() + 22.0),
                       state.isEmpty() ? line2
                                       : QString("%1  %2").arg(line2, state));
        }
    }

    // 目标与告警标记最后画，避免被相邻站点的标签牌盖住。
    for (int i = 0; i < m_system->stationCount(); ++i) {
        const ds::Station& s = m_system->stationAt(i);
        const QPointF pt = screenPos(s);
        const qreal side = std::clamp(22.0 + s.capacity * 0.25, 26.0, 38.0);
        const QRectF box(pt.x() - side / 2.0, pt.y() - side / 2.0, side, side);
        if (m_targets.contains(s.id)) {
            const QRectF mark(box.left() - 3.0, box.top() - 3.0, 7.0, 7.0);
            p.fillRect(mark, c.signal);
            p.setPen(QPen(c.ink, 1.0));
            p.setBrush(Qt::NoBrush);
            p.drawRect(mark);
        }
        if (alert(s)) {
            const QRectF flag(box.right() - 9.0, box.top() - 9.0, 10.0, 10.0);
            p.fillRect(flag, c.signal);
            p.setPen(QPen(c.ink, 1.0));
            p.setBrush(Qt::NoBrush);
            p.drawRect(flag);
            p.setFont(ef::font(ef::FontRole::Small));
            p.setPen(c.onSignal);
            p.drawText(flag, Qt::AlignCenter, QStringLiteral("!"));
        }
    }

    // 调度车：强调色实心块，是当前所在位置。
    if (m_truckT >= 0.0 && m_path.size() >= 2) {
        QVector<QPointF> pathPts;
        for (int id : m_path) {
            if (pts.contains(id)) {
                pathPts.push_back(pts.value(id));
            }
        }
        double totalLen = 0.0;
        QVector<double> lens;
        for (int i = 0; i + 1 < pathPts.size(); ++i) {
            const double len = QLineF(pathPts[i], pathPts[i + 1]).length();
            lens.push_back(len);
            totalLen += len;
        }
        if (totalLen > 0.0) {
            double target = std::clamp(m_truckT, 0.0, 1.0) * totalLen;
            QPointF pos = pathPts.front();
            for (int i = 0; i < lens.size(); ++i) {
                if (target <= lens[i]) {
                    const double t = lens[i] > 0.0 ? target / lens[i] : 0.0;
                    pos = pathPts[i] + (pathPts[i + 1] - pathPts[i]) * t;
                    break;
                }
                target -= lens[i];
                pos = pathPts[i + 1];
            }
            const QRectF car(pos.x() - 8.0, pos.y() - 8.0, 16.0, 16.0);
            p.fillPath(ef::cutPath(car, 4.0), c.signal);
            p.setPen(QPen(c.ink, 1.0));
            p.setBrush(Qt::NoBrush);
            p.drawPath(ef::cutPath(car, 4.0));
            p.setFont(ef::font(ef::FontRole::Small));
            p.setPen(c.ink);
            p.drawText(pos + QPointF(12, -8), QStringLiteral("调度车"));
        }
    }

    // 图例。
    const int ly = height() - 14;
    int lx = 16;
    const auto legendText = [&](const QString& text) {
        p.setPen(c.ink2);
        p.setFont(ef::font(ef::FontRole::Small));
        p.drawText(QPointF(lx + 18, ly), text);
        lx += 100;
    };
    p.fillRect(QRectF(lx, ly - 10, 12, 10), c.panel);
    p.setPen(QPen(c.ink, 1.0));
    p.setBrush(Qt::NoBrush);
    p.drawRect(QRectF(lx, ly - 10, 12, 10));
    legendText(QStringLiteral("正常"));
    p.fillRect(QRectF(lx, ly - 10, 12, 10), c.panel);
    p.setPen(QPen(c.ink, 1.0));
    p.drawRect(QRectF(lx, ly - 10, 12, 10));
    p.fillRect(QRectF(lx + 8, ly - 12, 6, 6), c.signal);
    legendText(QStringLiteral("偏低 / 偏高"));
    p.setPen(QPen(c.ink, 1.5, Qt::DashLine));
    p.drawLine(QPointF(lx, ly - 5), QPointF(lx + 12, ly - 5));
    legendText(QStringLiteral("阻断"));
    p.setPen(QPen(c.trace, 3.0));
    p.drawLine(QPointF(lx, ly - 5), QPointF(lx + 12, ly - 5));
    legendText(QStringLiteral("调度路线"));
    p.fillRect(QRectF(lx, ly - 10, 8, 8), c.signal);
    p.setPen(QPen(c.ink, 1.0));
    p.drawRect(QRectF(lx, ly - 10, 8, 8));
    legendText(QStringLiteral("目标"));
}

int MapCanvas::hitStation(const QPointF& pos) {
    if (m_system == nullptr) {
        return -1;
    }
    updateTransform();
    for (int i = 0; i < m_system->stationCount(); ++i) {
        const ds::Station& s = m_system->stationAt(i);
        const QPointF pt = screenPos(s);
        const qreal side = std::clamp(22.0 + s.capacity * 0.25, 26.0, 38.0);
        if (QLineF(pt, pos).length() <= side / 2.0 + 6.0) {
            return s.id;
        }
    }
    return -1;
}

void MapCanvas::mousePressEvent(QMouseEvent* event) {
    const int id = hitStation(event->position());
    if (id < 0) {
        return;
    }
    if (m_stationClicked) {
        m_stationClicked(id);
    }
    event->accept();
}

void MapCanvas::mouseMoveEvent(QMouseEvent* event) {
    const int id = hitStation(event->position());
    if (id != m_hoverId) {
        m_hoverId = id;
        if (id >= 0) {
            setCursor(Qt::PointingHandCursor);
        } else {
            unsetCursor();
        }
    }
    QWidget::mouseMoveEvent(event);
}
