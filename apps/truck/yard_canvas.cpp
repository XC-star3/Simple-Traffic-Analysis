#include "yard_canvas.h"

#include <QFont>
#include <QFontMetrics>
#include <QPainter>
#include <QTimer>

#include <algorithm>

#include "common/theme.h"

namespace {

QString typeEn(int type) {
    switch (type) {
        case 0: return QStringLiteral("FRESH");
        case 1: return QStringLiteral("NORMAL");
        default: return QStringLiteral("BULK");
    }
}

}  // namespace

YardCanvas::YardCanvas(QWidget* parent) : QWidget(parent) {
    setMinimumSize(480, 438);
    setAutoFillBackground(false);
    m_timer = new QTimer(this);
    m_timer->setInterval(33);
    connect(m_timer, &QTimer::timeout, this, [this]() { update(); });
}

int YardCanvas::verticalShift() const {
    return std::clamp((height() - 500) / 4, 0, 48);
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
    w = std::clamp(w, 16, 64);
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
                return QPointF(14 + 8 + col * 50 + 21,
                               laneY(type) + 33 + row * 18 + 9);
            }
        }
    }
    return QPointF(width() + 80.0, channelY() + sh / 2.0);
}

void YardCanvas::paintTruck(QPainter& p, const QPointF& pos, const ds::Truck& t,
                            bool small) {
    const qreal w = small ? 42.0 : std::min(52.0, std::max(16.0, slotW() - 2.0));
    const qreal h = small ? 18.0 : 30.0;
    const QRectF r(pos.x() - w / 2.0, pos.y() - h / 2.0, w, h);
    if (!r.intersects(QRectF(0, 0, width(), height()))) {
        return;
    }

    const ef::Palette& c = ef::palette();
    QColor fill = c.panel;
    QColor text = c.ink;
    bool hatch = false;
    if (t.type == ds::CargoType::Fresh) {
        fill = c.ink;
        text = c.paper;
    } else if (t.type == ds::CargoType::Bulk) {
        hatch = true;
    }

    p.setBrush(fill);
    p.setPen(QPen(c.ink, 1.0));
    p.drawRect(r);
    if (hatch) {
        ef::drawHatch(p, r.adjusted(1, 1, -1, -1), c.ink, 0.22);
    }

    QFont f = ef::font(ef::FontRole::Field);
    // 窄窗口里主通道每格只有 20 多像素，标签必须跟着缩一档，否则会被裁掉。
    f.setPixelSize(small ? 7 : (w < 34.0 ? 6 : 9));
    // 小字号下不要合成粗体：合成后的横画会糊成一团，T1001 反而不好认。
    f.setWeight(QFont::Normal);
    p.setFont(f);
    p.setPen(text);
    const QString label = w < 34.0 ? QString::number(t.id) : QString("T%1").arg(t.id);
    // Saira 的行盒比字形高，AlignCenter 会把字顶高；按字形包围盒手动居中。
    const QFontMetrics fm(f);
    const QRect bounds = fm.boundingRect(label);
    const QPointF baseline(r.center().x() - bounds.width() / 2.0 - bounds.left(),
                           r.center().y() - bounds.center().y());
    p.drawText(baseline, label);
}

void YardCanvas::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    const ef::Palette& c = ef::palette();
    p.fillRect(rect(), c.panel);
    ef::drawContours(p, QRectF(rect()), c.ink, ef::isDark() ? 0.06 : 0.09);

    if (m_yard == nullptr || !m_yard->configured()) {
        if (m_timer->isActive()) {
            m_timer->stop();
        }
        const QRectF empty(36, 90, width() - 72.0, height() - 180.0);
        ef::drawHatch(p, empty, c.ink, 0.22);
        p.setPen(QPen(c.rule, 1.0));
        p.setBrush(Qt::NoBrush);
        p.drawRect(empty);
        p.setPen(c.ink);
        p.setFont(ef::font(ef::FontRole::Label));
        p.drawText(empty, Qt::AlignCenter,
                   QStringLiteral("请先在左侧设置场地参数并应用。"));
        return;
    }

    const int cap = m_yard->capacity();
    const int tempCap = m_yard->tempCapacity();
    const int sw = slotW();
    const int sh = slotH();
    const int gy = gap();

    // 顶部字段：都是真实数据。
    p.setFont(ef::font(ef::FontRole::Field));
    p.setPen(c.ink2);
    p.drawText(QRectF(16, 14, width() - 32, 18), Qt::AlignLeft | Qt::AlignVCenter,
               ef::fields({QString("主通道 %1 位").arg(cap),
                           QString("临时区 %1 位").arg(tempCap),
                           QString("费率 %1 元/小时").arg(m_yard->feePerHour(), 0, 'f', 2)}));

    // 主通道。
    p.setFont(ef::font(ef::FontRole::Label));
    p.setPen(c.ink);
    p.drawText(QPointF(channelX(), channelY() - 10.0), QStringLiteral("主装卸通道"));
    p.setFont(ef::font(ef::FontRole::Small));
    p.setPen(c.ink2);
    p.drawText(QRectF(width() - 90.0, channelY() - 22.0, 74.0, 14.0),
               Qt::AlignRight | Qt::AlignVCenter, QStringLiteral("入口"));

    for (int i = 0; i < cap; ++i) {
        const QRectF r(channelX() + i * (sw + gy), channelY(), sw, sh);
        p.fillRect(r, c.panel);
        ef::drawHatch(p, r.adjusted(1, 1, -1, -1), c.ink, 0.10);
        p.setPen(QPen(c.rule, 1.0));
        p.setBrush(Qt::NoBrush);
        p.drawRect(r);
        p.setFont(ef::font(ef::FontRole::Small));
        p.setPen(c.ink2);
        p.drawText(r.adjusted(3, 0, -3, -3), Qt::AlignLeft | Qt::AlignBottom,
                   QString("%1").arg(i + 1, 2, 10, QChar('0')));
    }
    p.setPen(Qt::NoPen);
    p.setBrush(c.trace);
    p.drawRect(QRectF(channelX(), channelY() + sh + 3.0,
                      cap * (sw + gy) - gy, 3.0));

    // 临时区。
    p.setFont(ef::font(ef::FontRole::Label));
    p.setPen(c.ink);
    p.drawText(QPointF(channelX(), tempY() - 10.0), QStringLiteral("临时等候区"));
    p.setFont(ef::font(ef::FontRole::Field));
    p.setPen(c.ink2);
    p.drawText(QPointF(channelX() + 108.0, tempY() - 12.0),
               QString("%1 / %2 辆")
                   .arg(m_yard->temp().size())
                   .arg(tempCap));
    for (int i = 0; i < tempCap; ++i) {
        const QRectF r(channelX() + i * (sw + gy), tempY(), sw, sh);
        p.fillRect(r, c.panel);
        ef::drawHatch(p, r.adjusted(1, 1, -1, -1), c.ink, 0.16);
        p.setPen(QPen(c.rule, 1.0));
        p.setBrush(Qt::NoBrush);
        p.drawRect(r);
    }
    if (tempCap == 0) {
        p.setPen(c.ink2);
        p.setFont(ef::font(ef::FontRole::Small));
        p.drawText(QPointF(channelX() + 4, tempY() + sh / 2 + 4),
                   QStringLiteral("未设置临时区"));
    }

    // 三条优先级队列。
    for (int k = 0; k < 3; ++k) {
        const ds::CargoType type = static_cast<ds::CargoType>(k);
        const int y = laneY(k);
        const QRectF lane(14, y, 220, laneH());
        p.fillRect(lane, c.panel);
        p.setPen(QPen(c.rule, 1.0));
        p.setBrush(Qt::NoBrush);
        p.drawRect(lane);

        p.setFont(ef::font(ef::FontRole::Eyebrow));
        p.setPen(c.ink2);
        p.drawText(QRectF(lane.left() + 8, lane.top() + 5, lane.width() - 16, 12),
                   Qt::AlignLeft | Qt::AlignVCenter,
                   QString("// %1").arg(typeEn(k)));
        p.setFont(ef::font(ef::FontRole::Label));
        p.setPen(c.ink);
        p.drawText(QPointF(lane.left() + 8, lane.top() + 26),
                   QString::fromUtf8(ds::cargoName(type)));
        p.setFont(ef::font(ef::FontRole::Field));
        p.setPen(c.ink2);
        const int queued = m_yard->queue(type).size();
        p.drawText(QRectF(lane.left() + 8, lane.top() + 5, lane.width() - 16, 12),
                   Qt::AlignRight | Qt::AlignVCenter,
                   queued > 8 ? QString("%1 辆（+%2）").arg(queued).arg(queued - 8)
                              : QString("%1 辆").arg(queued));

        p.setPen(QPen(c.rule, 1.0));
        p.drawLine(QPointF(lane.left() + 8, lane.top() + 30),
                   QPointF(lane.right() - 8, lane.top() + 30));

        const ds::Queue<int>& q = m_yard->queue(type);
        if (q.empty()) {
            const QRectF empty(lane.left() + 8, lane.top() + 33, lane.width() - 16, 31);
            ef::drawHatch(p, empty, c.ink, 0.16);
            p.setPen(c.ink2);
            p.setFont(ef::font(ef::FontRole::Small));
            p.drawText(empty, Qt::AlignCenter, QStringLiteral("暂无排队车辆。"));
        } else {
            const int shown = std::min(q.size(), 8);
            for (int i = 0; i < shown; ++i) {
                const int col = i % 4;
                const int row = i / 4;
                const QPointF pos(lane.left() + 8 + col * 50 + 21,
                                  lane.top() + 33 + row * 18 + 9);
                paintTruck(p, pos, m_yard->truckAt(q.at(i)), true);
            }
        }
    }

    // 平滑移动：只处理通道、临时区和已离场的车辆。
    // 只有确实存在位移时才启动 33ms 定时器，静止后立刻停下，避免空转重绘。
    bool moving = false;
    const ds::Vector<ds::Truck>& trucks = m_yard->trucks();
    for (int i = 0; i < trucks.size(); ++i) {
        const ds::Truck& t = trucks[i];
        if (t.state == ds::TruckState::Waiting) {
            continue;
        }
        const QPointF target = targetFor(i);
        if (t.state == ds::TruckState::Left) {
            if (!m_pos.contains(i)) {
                continue;
            }
            QPointF cur = m_pos.value(i);
            cur += (target - cur) * 0.30;
            m_pos[i] = cur;
            if (cur.x() > width() + 40.0) {
                m_pos.remove(i);
                continue;
            }
            moving = true;
            paintTruck(p, cur, t, false);
            continue;
        }

        if (!m_pos.contains(i)) {
            m_pos.insert(i, target);
        }
        QPointF cur = m_pos.value(i);
        if (ef::reduceMotion()) {
            cur = target;
        } else {
            cur += (target - cur) * 0.30;
        }
        if ((target - cur).manhattanLength() < 0.5) {
            cur = target;
        } else {
            moving = true;
        }
        m_pos[i] = cur;
        paintTruck(p, cur, t, false);
    }

    if (moving) {
        if (!m_timer->isActive()) {
            m_timer->start();
        }
    } else if (m_timer->isActive()) {
        m_timer->stop();
    }

    // 图例：类型用墨色、描边和斜线纹区分。
    const int ly = height() - 14;
    int lx = 16;
    const char* labels[3] = {"生鲜件", "普通件", "大件货"};
    for (int k = 0; k < 3; ++k) {
        const QRectF swatch(lx, ly - 10, 14, 10);
        if (k == 0) {
            p.fillRect(swatch, c.ink);
        } else {
            p.fillRect(swatch, c.panel);
            p.setPen(QPen(c.ink, 1.0));
            p.setBrush(Qt::NoBrush);
            p.drawRect(swatch);
            if (k == 2) {
                ef::drawHatch(p, swatch.adjusted(1, 1, -1, -1), c.ink, 0.22);
            }
        }
        p.setPen(c.ink2);
        p.setFont(ef::font(ef::FontRole::Small));
        p.drawText(QPointF(lx + 20, ly), QString::fromUtf8(labels[k]));
        lx += 92;
    }
    p.setPen(QPen(c.trace, 3.0));
    p.drawLine(QPointF(width() - 180, ly - 5), QPointF(width() - 150, ly - 5));
    p.setPen(c.ink2);
    p.drawText(QPointF(width() - 142, ly), QStringLiteral("栈底线"));
}
