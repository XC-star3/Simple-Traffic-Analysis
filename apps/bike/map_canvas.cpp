#include "map_canvas.h"

#include <QFont>
#include <QFontMetrics>
#include <QLineF>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QtGlobal>

namespace {

QColor stationColor(int bikes, int capacity) {
    if (capacity <= 0) {
        return QColor(0x2F, 0x80, 0xED);
    }
    const double ratio = static_cast<double>(bikes) / capacity;
    if (ratio < 0.2) {
        return QColor(0xEB, 0x57, 0x57);
    }
    if (ratio > 0.85) {
        return QColor(0xF2, 0x99, 0x4A);
    }
    return QColor(0x2F, 0x80, 0xED);
}

}  // namespace

MapCanvas::MapCanvas(QWidget* parent) : QWidget(parent) {
    setMinimumSize(640, 520);
    setMouseTracking(true);
}

void MapCanvas::setSystem(ds::BikeSystem* system) {
    m_system = system;
    m_path.clear();
    m_truckT = -1.0;
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
        minX = qMin(minX, s.x);
        minY = qMin(minY, s.y);
        maxX = qMax(maxX, s.x);
        maxY = qMax(maxY, s.y);
    }
    const double pad = 60.0;
    return QRectF(minX - pad, minY - pad, (maxX - minX) + pad * 2, (maxY - minY) + pad * 2);
}

void MapCanvas::updateTransform() {
    const QRectF b = contentBounds();
    const double sx = (width() - 80.0) / qMax(1.0, b.width());
    const double sy = (height() - 100.0) / qMax(1.0, b.height());
    m_scale = qMin(sx, sy);
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

void MapCanvas::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.fillRect(rect(), QColor(0xF7, 0xF9, 0xFC));
    updateTransform();

    QFont title = p.font();
    title.setPointSize(12);
    title.setBold(true);
    p.setFont(title);
    p.setPen(QColor(0x1F, 0x2A, 0x37));
    p.drawText(20, 30, "城市公共自行车调度 · 路网与调度路线");

    if (m_system == nullptr || m_system->stationCount() == 0) {
        p.setPen(QColor(0x6B, 0x72, 0x80));
        p.drawText(rect(), Qt::AlignCenter, "请加载站点与道路数据");
        return;
    }

    // 道路
    for (int i = 0; i < m_system->roads().size(); ++i) {
        const ds::Road& r = m_system->roads()[i];
        const ds::Station* su = m_system->findStation(r.u);
        const ds::Station* sv = m_system->findStation(r.v);
        if (su == nullptr || sv == nullptr) {
            continue;
        }
        const QPointF a = screenPos(*su);
        const QPointF b = screenPos(*sv);
        const bool active = m_system->hasActiveRoad(r.u, r.v);
        const bool highlight = onPath(r.u, r.v);

        QPen pen;
        if (highlight) {
            pen = QPen(QColor(0xF2, 0xC9, 0x4C), 6.0, Qt::SolidLine, Qt::RoundCap);
        } else if (!active) {
            pen = QPen(QColor(0xEB, 0x57, 0x57), 2.0, Qt::DashLine);
        } else {
            pen = QPen(QColor(0xB8, 0xC2, 0xCF), 2.0);
        }
        p.setPen(pen);
        p.drawLine(a, b);

        if (active && !highlight) {
            QFont f = p.font();
            f.setPointSize(7);
            p.setFont(f);
            p.setPen(QColor(0x9A, 0xA3, 0xAF));
            const QPointF mid = (a + b) / 2.0;
            p.drawText(mid + QPointF(4, -4),
                       QString("%1m / %2min").arg(r.dist, 0, 'f', 0).arg(r.time, 0, 'f', 0));
        }
    }

    // 调度路线（在道路上方、站点下方）
    if (m_path.size() >= 2) {
        QPainterPath path;
        bool first = true;
        for (int id : m_path) {
            const ds::Station* s = m_system->findStation(id);
            if (s == nullptr) {
                continue;
            }
            const QPointF pt = screenPos(*s);
            if (first) {
                path.moveTo(pt);
                first = false;
            } else {
                path.lineTo(pt);
            }
        }
        p.setPen(QPen(QColor(0xF2, 0xC9, 0x4C, 120), 10.0, Qt::SolidLine, Qt::RoundCap,
                      Qt::RoundJoin));
        p.setBrush(Qt::NoBrush);
        p.drawPath(path);
    }

    // 站点
    for (int i = 0; i < m_system->stationCount(); ++i) {
        const ds::Station& s = m_system->stationAt(i);
        const QPointF pt = screenPos(s);
        const double radius = qBound(14.0, 12.0 + s.capacity * 0.3, 26.0);

        p.setBrush(stationColor(s.bikes, s.capacity));
        p.setPen(QPen(QColor(0xFF, 0xFF, 0xFF), 2.0));
        p.drawEllipse(pt, radius, radius);
        p.setPen(QColor(0x1F, 0x2A, 0x37));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(pt, radius, radius);

        QFont f = p.font();
        f.setBold(true);
        f.setPointSize(8);
        p.setFont(f);
        p.setPen(Qt::white);
        p.drawText(QRectF(pt.x() - radius, pt.y() - radius, radius * 2, radius * 2),
                   Qt::AlignCenter, QString::number(s.bikes));

        f.setBold(false);
        f.setPointSize(8);
        p.setFont(f);
        const QString line1 = QString("%1 %2").arg(s.id).arg(QString::fromUtf8(s.name));
        const QString line2 = QString("%1/%2").arg(s.bikes).arg(s.capacity);
        const QFontMetrics fm(f);
        const int textWidth = qMax(fm.horizontalAdvance(line1), fm.horizontalAdvance(line2));
        const QRectF plate(pt.x() + radius + 3, pt.y() - 17, textWidth + 8, 34);
        p.setBrush(QColor(0xFF, 0xFF, 0xFF, 215));
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(plate, 4, 4);
        p.setPen(QColor(0x37, 0x41, 0x51));
        p.drawText(QPointF(plate.x() + 4, plate.y() + 14), line1);
        p.setPen(QColor(0x6B, 0x72, 0x80));
        p.drawText(QPointF(plate.x() + 4, plate.y() + 28), line2);
    }

    // 调度车
    if (m_truckT >= 0.0 && m_path.size() >= 2) {
        double totalLen = 0.0;
        QVector<QPointF> pts;
        QVector<double> lens;
        for (int id : m_path) {
            const ds::Station* s = m_system->findStation(id);
            if (s == nullptr) {
                continue;
            }
            pts.push_back(screenPos(*s));
        }
        for (int i = 0; i + 1 < pts.size(); ++i) {
            const double len = QLineF(pts[i], pts[i + 1]).length();
            lens.push_back(len);
            totalLen += len;
        }
        if (totalLen > 0.0) {
            double target = qBound(0.0, m_truckT, 1.0) * totalLen;
            QPointF pos = pts.front();
            for (int i = 0; i < lens.size(); ++i) {
                if (target <= lens[i]) {
                    const double t = lens[i] > 0.0 ? target / lens[i] : 0.0;
                    pos = pts[i] + (pts[i + 1] - pts[i]) * t;
                    break;
                }
                target -= lens[i];
                pos = pts[i + 1];
            }
            p.setBrush(QColor(0xF2, 0x99, 0x4A));
            p.setPen(QPen(Qt::white, 2.0));
            p.drawEllipse(pos, 10.0, 10.0);
            p.setPen(QColor(0x7A, 0x4A, 0x12));
            QFont f = p.font();
            f.setBold(true);
            f.setPointSize(8);
            p.setFont(f);
            p.drawText(pos + QPointF(12, -10), "调度车");
        }
    }

    // 图例
    QFont f = p.font();
    f.setPointSize(8);
    f.setBold(false);
    p.setFont(f);
    int x = 20;
    const int y = height() - 16;
    struct Legend {
        QColor color;
        const char* text;
    };
    const Legend legends[] = {
        {QColor(0x2F, 0x80, 0xED), "正常"},
        {QColor(0xF2, 0x99, 0x4A), "偏高"},
        {QColor(0xEB, 0x57, 0x57), "偏低/阻断"},
        {QColor(0xF2, 0xC9, 0x4C), "调度路线"},
    };
    for (const auto& item : legends) {
        p.setBrush(item.color);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(QRectF(x, y - 9, 12, 10), 2, 2);
        p.setPen(QColor(0x4B, 0x55, 0x63));
        p.drawText(x + 17, y, QString::fromUtf8(item.text));
        x += 92;
    }
}

void MapCanvas::mousePressEvent(QMouseEvent* event) {
    if (m_system == nullptr) {
        return;
    }
    updateTransform();
    double best = 22.0;
    int bestId = -1;
    for (int i = 0; i < m_system->stationCount(); ++i) {
        const ds::Station& s = m_system->stationAt(i);
        const QPointF pt = screenPos(s);
        const double d = QLineF(pt, event->position()).length();
        if (d < best) {
            best = d;
            bestId = s.id;
        }
    }
    if (bestId >= 0) {
        emit stationClicked(bestId);
    }
}
