#include "yard_canvas.h"

#include <QFont>
#include <QPainter>
#include <QTimer>

#include <utility>

namespace {

QColor colorFor(ds::CargoType type) {
    switch (type) {
        case ds::CargoType::Fresh: return QColor(0x27, 0xAE, 0x60);
        case ds::CargoType::Normal: return QColor(0x2F, 0x80, 0xED);
        case ds::CargoType::Bulk: return QColor(0xF2, 0x99, 0x4A);
    }
    return QColor(0x88, 0x88, 0x88);
}

QColor darkFor(ds::CargoType type) {
    switch (type) {
        case ds::CargoType::Fresh: return QColor(0x1E, 0x84, 0x49);
        case ds::CargoType::Normal: return QColor(0x1C, 0x5F, 0xB8);
        case ds::CargoType::Bulk: return QColor(0xB9, 0x6B, 0x22);
    }
    return QColor(0x55, 0x55, 0x55);
}

}  // namespace

YardCanvas::YardCanvas(QWidget* parent) : QWidget(parent) {
    setMinimumSize(680, 520);
    setAutoFillBackground(false);
    m_timer = new QTimer(this);
    m_timer->setInterval(30);
    connect(m_timer, &QTimer::timeout, this, [this]() { update(); });
    m_timer->start();
}

void YardCanvas::setYard(ds::TruckYard* yard) {
    m_yard = yard;
    resetAnimation();
    update();
}

void YardCanvas::resetAnimation() {
    m_pos.clear();
    update();
}

int YardCanvas::slotW() const {
    const int cap = m_yard != nullptr ? m_yard->capacity() : 8;
    const int avail = width() - channelX() - 20;
    if (cap <= 0) {
        return 60;
    }
    int w = (avail - gap() * (cap - 1)) / cap;
    if (w > 76) {
        w = 76;
    }
    if (w < 12) {
        w = 12;
    }
    return w;
}

QPointF YardCanvas::targetFor(int index) const {
    if (m_yard == nullptr) {
        return QPointF(0, 0);
    }
    const ds::Truck& t = m_yard->truckAt(index);
    const int sw = slotW();
    const int sh = slotH();
    const int gy = gap();

    if (t.state == ds::TruckState::Channel) {
        const ds::Stack<int>& ch = m_yard->channel();
        for (int i = 0; i < ch.size(); ++i) {
            if (ch.at(i) == index) {
                return QPointF(channelX() + i * (sw + gy) + sw / 2.0,
                               channelY() + sh / 2.0);
            }
        }
    }
    if (t.state == ds::TruckState::Temp) {
        const ds::Stack<int>& tp = m_yard->temp();
        for (int i = 0; i < tp.size(); ++i) {
            if (tp.at(i) == index) {
                return QPointF(channelX() + i * (sw + gy) + sw / 2.0,
                               tempY() + sh / 2.0);
            }
        }
    }
    if (t.state == ds::TruckState::Waiting) {
        const int type = static_cast<int>(t.type);
        const ds::Queue<int>& q = m_yard->queue(t.type);
        for (int i = 0; i < q.size(); ++i) {
            if (q.at(i) == index) {
                const int col = i % 4;
                const int row = (i / 4) % 2;
                return QPointF(30 + col * 56 + 24, laneY(type) + 30 + row * 28 + 11);
            }
        }
    }
    return QPointF(width() + 120.0, channelY() + sh / 2.0);
}

void YardCanvas::paintTruck(QPainter& p, const QPointF& pos, const ds::Truck& t,
                            bool small) {
    const qreal w = small ? 48.0 : qMin(58.0, qMax(18.0, slotW() - 4.0));
    const qreal h = small ? 22.0 : 36.0;
    QRectF r(pos.x() - w / 2.0, pos.y() - h / 2.0, w, h);
    if (!r.intersects(QRectF(0, 0, width(), height()))) {
        return;
    }
    p.setBrush(colorFor(t.type));
    p.setPen(QPen(darkFor(t.type), small ? 1.0 : 2.0));
    p.drawRoundedRect(r, small ? 4.0 : 7.0, small ? 4.0 : 7.0);

    QFont f = p.font();
    f.setBold(true);
    f.setPointSize(small ? 7 : (w < 40.0 ? 6 : 9));
    p.setFont(f);
    p.setPen(Qt::white);
    p.drawText(r, Qt::AlignCenter,
               w < 26.0 ? QString::number(t.id) : QString("T%1").arg(t.id));
}

void YardCanvas::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.fillRect(rect(), QColor(0xF5, 0xF7, 0xFA));

    QFont base = p.font();
    base.setPointSize(9);
    p.setFont(base);

    if (m_yard == nullptr || !m_yard->configured()) {
        p.setPen(QColor(0x6B, 0x72, 0x80));
        p.drawText(rect(), Qt::AlignCenter, "请先设置场地参数");
        return;
    }

    const int cap = m_yard->capacity();
    const int tempCap = m_yard->tempCapacity();
    const int sw = slotW();
    const int sh = slotH();
    const int gy = gap();

    // 标题
    QFont title = p.font();
    title.setPointSize(12);
    title.setBold(true);
    p.setFont(title);
    p.setPen(QColor(0x1F, 0x2A, 0x37));
    p.drawText(24, 34, "快递中转场 · 栈 + 优先级队列混合调度");

    QFont sub = p.font();
    sub.setPointSize(9);
    sub.setBold(false);
    p.setFont(sub);
    p.setPen(QColor(0x6B, 0x72, 0x80));
    p.drawText(24, 56, QString("主通道容量 %1 辆   临时区容量 %2 辆   费率 %3 元/小时")
                           .arg(cap)
                           .arg(tempCap)
                           .arg(m_yard->feePerHour(), 0, 'f', 2));

    // 通道槽位
    p.setPen(QColor(0x9A, 0xA3, 0xAF));
    p.setBrush(QColor(0xE9, 0xEE, 0xF5));
    for (int i = 0; i < cap; ++i) {
        QRectF r(channelX() + i * (sw + gy), channelY(), sw, sh);
        p.drawRoundedRect(r, 5, 5);
        p.setPen(QColor(0xC5, 0xCE, 0xDA));
        p.drawText(r.adjusted(0, 0, 0, -4), Qt::AlignBottom | Qt::AlignHCenter,
                   QString::number(i + 1));
        p.setPen(QColor(0x9A, 0xA3, 0xAF));
    }
    p.setPen(QColor(0x6B, 0x72, 0x80));
    p.drawText(channelX(), channelY() - 10, "主装卸通道（栈，左为栈底 / 右为入口）");
    p.drawText(channelX() + cap * (sw + gy) + 4, channelY() + sh / 2 + 5, "入口 →");

    // 临时区槽位
    p.drawText(channelX(), tempY() - 10, "临时等候区（栈，让路时暂存）");
    for (int i = 0; i < tempCap; ++i) {
        QRectF r(channelX() + i * (sw + gy), tempY(), sw, sh);
        p.setPen(QColor(0xC0, 0x9A, 0x7A));
        p.setBrush(QColor(0xFA, 0xF1, 0xE8));
        p.drawRoundedRect(r, 5, 5);
    }
    if (tempCap == 0) {
        p.setPen(QColor(0x6B, 0x72, 0x80));
        p.drawText(channelX() + 4, tempY() + sh / 2 + 5, "（未设置临时区）");
    }

    // 队列
    for (int k = 0; k < 3; ++k) {
        const ds::CargoType type = static_cast<ds::CargoType>(k);
        const int y = laneY(k);
        QRectF lane(20, y, 250, 86);
        p.setPen(QColor(0xD5, 0xDD, 0xE8));
        p.setBrush(QColor(0xFF, 0xFF, 0xFF));
        p.drawRoundedRect(lane, 8, 8);

        p.setPen(colorFor(type));
        QFont f = p.font();
        f.setBold(true);
        p.setFont(f);
        p.drawText(30, y + 20, QString("%1队列").arg(ds::cargoName(type)));
        p.setPen(QColor(0x6B, 0x72, 0x80));
        f.setBold(false);
        p.setFont(f);
        p.drawText(210, y + 20, QString("积压 %1").arg(m_yard->queue(type).size()));

        const int shown = m_yard->queue(type).size() < 8 ? m_yard->queue(type).size() : 8;
        for (int i = 0; i < shown; ++i) {
            const int col = i % 4;
            const int row = i / 4;
            const QPointF pos(30 + col * 56 + 24, y + 30 + row * 28 + 11);
            const int index = m_yard->queue(type).at(i);
            paintTruck(p, pos, m_yard->truckAt(index), true);
        }
        if (m_yard->queue(type).size() > 8) {
            p.setPen(QColor(0x6B, 0x72, 0x80));
            p.drawText(30, y + 82, QString("+%1 辆").arg(m_yard->queue(type).size() - 8));
        }
    }

    // 动画位置：向目标位置平滑靠拢。
    const ds::Vector<ds::Truck>& trucks = m_yard->trucks();
    for (int i = 0; i < trucks.size(); ++i) {
        const QPointF target = targetFor(i);
        if (!m_pos.contains(i)) {
            m_pos.insert(i, target);
        } else {
            QPointF cur = m_pos.value(i);
            cur += (target - cur) * 0.28;
            m_pos[i] = cur;
        }
    }
    for (int i = 0; i < trucks.size(); ++i) {
        const ds::Truck& t = trucks[i];
        if (t.state == ds::TruckState::Waiting) {
            continue;  // 队列里的车已经按槽位画过
        }
        paintTruck(p, m_pos.value(i), t, false);
    }

    // 图例
    int lx = 24;
    const int ly = height() - 22;
    const char* labels[3] = {"生鲜件", "普通件", "大件货"};
    for (int k = 0; k < 3; ++k) {
        p.setBrush(colorFor(static_cast<ds::CargoType>(k)));
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(QRectF(lx, ly - 10, 14, 12), 3, 3);
        p.setPen(QColor(0x4B, 0x55, 0x63));
        p.drawText(lx + 20, ly, QString::fromUtf8(labels[k]));
        lx += 86;
    }
}
