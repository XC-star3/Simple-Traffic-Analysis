#include "common/widgets.h"

#include <QAbstractItemModel>
#include <QApplication>
#include <QBrush>
#include <QEnterEvent>
#include <QFocusEvent>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPalette>
#include <QPen>
#include <QStyle>
#include <QStyledItemDelegate>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>

namespace ef {
namespace {

class Seal final : public QWidget {
public:
    Seal(const QString& letter, QWidget* parent) : QWidget(parent), m_letter(letter) {
        setFixedSize(30, 30);
        setFocusPolicy(Qt::NoFocus);
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        const QRectF r = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
        p.setPen(Qt::NoPen);
        p.setBrush(ef::palette().signal);
        p.drawPath(cutPath(r, 6.0));
        p.setPen(ef::palette().onSignal);
        QFont f = ef::font(FontRole::Heading);
        f.setPixelSize(16);
        f.setWeight(QFont::Black);
        p.setFont(f);
        p.drawText(r, Qt::AlignCenter, m_letter.toUpper());
    }

private:
    QString m_letter;
};

class SectionTag final : public QWidget {
public:
    SectionTag(const QString& en, const QString& zh, QWidget* parent)
        : QWidget(parent), m_en(en.toUpper()), m_zh(zh) {
        setFixedSize(116, 34);
        setFocusPolicy(Qt::NoFocus);
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        const QRectF r = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
        p.fillRect(r, ef::palette().signal);
        p.setPen(ef::palette().onSignal);
        QFont f = ef::font(FontRole::Eyebrow);
        f.setPixelSize(8);
        p.setFont(f);
        p.drawText(QRectF(r.left() + 8, r.top() + 4, r.width() - 16, 10),
                   Qt::AlignLeft | Qt::AlignVCenter, m_en);
        f = ef::font(FontRole::Label);
        f.setPixelSize(12);
        f.setWeight(QFont::Bold);
        p.setFont(f);
        p.drawText(QRectF(r.left() + 8, r.top() + 15, r.width() - 16, 16),
                   Qt::AlignLeft | Qt::AlignVCenter, m_zh);
    }

private:
    QString m_en;
    QString m_zh;
};

QColor alphaColor(QColor c, int alpha) {
    c.setAlpha(alpha);
    return c;
}

}  // namespace

EfButton::EfButton(const QString& text, Variant variant, QWidget* parent)
    : QPushButton(text, parent), m_variant(variant) {
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_Hover, true);
    setMinimumHeight(34);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    m_anim = new QVariantAnimation(this);
    m_anim->setEasingCurve(easingMech());
    connect(m_anim, &QVariantAnimation::valueChanged, this, [this](const QVariant& value) {
        m_wipe = value.toReal();
        update();
    });
    setFont(ef::font(FontRole::Button));
}

void EfButton::setVariant(Variant variant) {
    m_variant = variant;
    update();
}

QSize EfButton::sizeHint() const {
    const QFontMetrics fm(ef::font(FontRole::Button));
    const int textWidth = fm.horizontalAdvance(text());
    return QSize(std::max(84, textWidth + 12 * 2 + 12), 34);
}

QSize EfButton::minimumSizeHint() const {
    return sizeHint();
}

void EfButton::animateTo(qreal target, int duration) {
    if (reduceMotion()) {
        m_wipe = target;
        update();
        return;
    }
    m_anim->stop();
    m_anim->setStartValue(m_wipe);
    m_anim->setEndValue(target);
    m_anim->setDuration(duration);
    m_anim->start();
}

void EfButton::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const Palette& c = ef::palette();
    const bool dark = isDark();
    QColor base = c.chip;
    QColor border = c.rule;
    QColor textColor = c.ink;
    QColor wipeColor = c.signal;
    QColor wipeText = c.onSignal;

    switch (m_variant) {
        case Variant::Primary:
            base = c.ink;
            border = c.ink;
            textColor = c.paper;
            wipeColor = c.signal;
            wipeText = c.onSignal;
            break;
        case Variant::Secondary:
            base = c.chip;
            border = c.rule;
            textColor = c.ink;
            wipeColor = c.signal;
            wipeText = c.onSignal;
            break;
        case Variant::Danger:
            base = c.panel;
            border = c.danger;
            textColor = c.danger;
            wipeColor = c.danger;
            wipeText = dark ? c.onSignal : QColor(0xFF, 0xFF, 0xFF);
            break;
        case Variant::OnDark:
            base = c.commandBar;
            border = alphaColor(c.commandBarText, 90);
            textColor = c.commandBarText;
            wipeColor = c.signal;
            wipeText = c.onSignal;
            break;
    }

    if (!isEnabled()) {
        base = c.chip;
        border = c.rule;
        textColor = c.ink2;
    }

    const QRectF r = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    const qreal cut = 8.0;
    const QPainterPath path = cutPath(r, cut);
    p.fillPath(path, base);

    if (m_wipe > 0.001 && isEnabled()) {
        p.save();
        p.setClipPath(path);
        p.fillRect(QRectF(r.left(), r.top(), r.width() * m_wipe, r.height()),
                   wipeColor);
        p.restore();
    }

    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(border, 1.0));
    p.drawPath(path);

    if (!isEnabled()) {
        drawHatch(p, r.adjusted(1, 1, -1, -1), c.ink, 0.22);
    }

    if (hasFocus()) {
        QColor ring = c.ink;
        if (m_variant == Variant::Primary) {
            ring = c.paper;
        } else if (m_variant == Variant::OnDark) {
            ring = c.commandBarText;
        } else if (m_variant == Variant::Danger) {
            ring = c.danger;
        }
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(ring, 1.0, Qt::DashLine));
        p.drawPath(cutPath(r.adjusted(3, 3, -3, -3), std::max<qreal>(0.0, cut - 2.0)));
    }

    QRectF textRect = r.adjusted(12, 0, -(cut + 12), 0);
    if (isDown()) {
        textRect.translate(0, 1);
    }
    p.setFont(ef::font(FontRole::Button));

    auto drawClipped = [&](const QColor& color, const QRectF& clip) {
        p.save();
        p.setClipRect(clip);
        p.setPen(color);
        p.drawText(textRect, Qt::AlignCenter, text());
        p.restore();
    };

    if (m_wipe > 0.001 && isEnabled()) {
        drawClipped(wipeText,
                    QRectF(r.left(), r.top(), r.width() * m_wipe, r.height()));
        drawClipped(textColor,
                    QRectF(r.left() + r.width() * m_wipe, r.top(),
                           r.width() * (1.0 - m_wipe), r.height()));
    } else {
        drawClipped(textColor, r);
    }
}

void EfButton::enterEvent(QEnterEvent* event) {
    m_hover = true;
    if (isEnabled()) {
        animateTo(1.0, kWipeMs);
    }
    QPushButton::enterEvent(event);
}

void EfButton::leaveEvent(QEvent* event) {
    m_hover = false;
    if (isEnabled()) {
        animateTo(0.0, kExitMs);
    }
    QPushButton::leaveEvent(event);
}

void EfButton::mousePressEvent(QMouseEvent* event) {
    if (isEnabled()) {
        animateTo(1.0, kPressMs);
    }
    QPushButton::mousePressEvent(event);
}

void EfButton::mouseReleaseEvent(QMouseEvent* event) {
    QPushButton::mouseReleaseEvent(event);
    if (isEnabled()) {
        animateTo(underMouse() ? 1.0 : 0.0, kExitMs);
    }
}

void EfButton::focusInEvent(QFocusEvent* event) {
    update();
    QPushButton::focusInEvent(event);
}

void EfButton::focusOutEvent(QFocusEvent* event) {
    update();
    QPushButton::focusOutEvent(event);
}

bool EfButton::hitButton(const QPoint& pos) const {
    return cutPath(QRectF(rect()), 8.0).contains(pos);
}

EfListDelegate::EfListDelegate(QObject* parent) : QStyledItemDelegate(parent) {}

QSize EfListDelegate::sizeHint(const QStyleOptionViewItem&, const QModelIndex&) const {
    return QSize(0, 27);
}

void EfListDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                           const QModelIndex& index) const {
    QStyleOptionViewItem opt(option);
    initStyleOption(&opt, index);

    const Palette& c = ef::palette();
    const bool checkable = (opt.features & QStyleOptionViewItem::HasCheckIndicator) != 0;
    const bool checked = opt.checkState == Qt::Checked;
    const QVariant bgData = index.data(Qt::BackgroundRole);
    const bool hasFill = bgData.canConvert<QBrush>() &&
                         qvariant_cast<QBrush>(bgData).style() != Qt::NoBrush;
    const bool filled = checked || hasFill;
    const bool enabled = (opt.state & QStyle::State_Enabled) != 0;
    const bool hover = checkable && (opt.state & QStyle::State_MouseOver) != 0 && !filled;

    painter->save();
    painter->fillRect(opt.rect, filled ? c.signal : (hover ? c.paper : c.panel));
    if (hover) {
        painter->fillRect(QRectF(opt.rect.left(), opt.rect.top(), 3.0,
                                 opt.rect.height()),
                          c.trace);
    }
    painter->fillRect(QRect(opt.rect.left(), opt.rect.bottom(), opt.rect.width(), 1),
                      c.rule);

    int textLeft = opt.rect.left() + 10;
    if (checkable) {
        const QRectF box(opt.rect.left() + 10.0, opt.rect.center().y() - 6.5, 13.0,
                         13.0);
        painter->setBrush(checked ? Qt::NoBrush : QBrush(c.panel));
        painter->setPen(QPen(checked ? c.onSignal : c.ink2, 1.0));
        painter->drawRect(box);
        if (checked) {
            QPainterPath tick;
            tick.moveTo(box.left() + 3.0, box.center().y());
            tick.lineTo(box.left() + 5.7, box.bottom() - 4.0);
            tick.lineTo(box.right() - 2.8, box.top() + 3.6);
            painter->setPen(QPen(c.onSignal, 1.6, Qt::SolidLine, Qt::SquareCap,
                                 Qt::MiterJoin));
            painter->setBrush(Qt::NoBrush);
            painter->drawPath(tick);
        }
        textLeft = static_cast<int>(box.right()) + 9;
    }

    QColor fg = filled ? c.onSignal : opt.palette.color(QPalette::Text);
    if (!filled) {
        const QVariant fgData = index.data(Qt::ForegroundRole);
        if (fgData.canConvert<QBrush>()) {
            fg = qvariant_cast<QBrush>(fgData).color();
        }
    }
    if (!enabled) {
        fg = c.ink2;
    }

    QFont textFont = opt.font;
    painter->setFont(textFont);
    painter->setPen(fg);
    const int rightPad = checkable ? 10 : 12;
    const QRect textRect(textLeft, opt.rect.top(),
                         std::max(0, opt.rect.right() - textLeft - rightPad),
                         opt.rect.height());
    const QString elided =
        QFontMetrics(textFont).elidedText(opt.text, Qt::ElideRight, textRect.width());
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, elided);

    if (opt.state & QStyle::State_HasFocus) {
        painter->setBrush(Qt::NoBrush);
        painter->setPen(QPen(filled ? c.onSignal : c.ink, 1.0, Qt::DashLine));
        painter->drawRect(opt.rect.adjusted(2, 2, -2, -2));
    }
    painter->restore();
}

bool EfListDelegate::editorEvent(QEvent* event, QAbstractItemModel* model,
                                 const QStyleOptionViewItem& option,
                                 const QModelIndex& index) {
    if (event->type() == QEvent::MouseButtonRelease &&
        (option.state & QStyle::State_Enabled) != 0) {
        const QVariant checkState = index.data(Qt::CheckStateRole);
        if (checkState.isValid() && (index.flags() & Qt::ItemIsUserCheckable) != 0) {
            const Qt::CheckState state = static_cast<Qt::CheckState>(checkState.toInt());
            return model->setData(index,
                                  state == Qt::Checked ? Qt::Unchecked : Qt::Checked,
                                  Qt::CheckStateRole);
        }
    }
    return QStyledItemDelegate::editorEvent(event, model, option, index);
}

EfPanel::EfPanel(const QString& eyebrow, const QString& title, QWidget* parent)
    : QFrame(parent) {
    setFrameShape(QFrame::NoFrame);
    setAttribute(Qt::WA_Hover, true);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    m_header = new QWidget(this);
    auto* hv = new QVBoxLayout(m_header);
    hv->setContentsMargins(12, 12, 12, 6);
    hv->setSpacing(1);
    hv->addWidget(makeEyebrow(eyebrow, m_header));
    hv->addWidget(makeTitle(title, m_header));
    outer->addWidget(m_header);

    auto* lineWrap = new QWidget(this);
    auto* lw = new QVBoxLayout(lineWrap);
    lw->setContentsMargins(12, 0, 12, 0);
    lw->setSpacing(0);
    lw->addWidget(makeRule(lineWrap));
    outer->addWidget(lineWrap);

    m_body = new QVBoxLayout;
    m_body->setContentsMargins(12, 8, 12, 10);
    m_body->setSpacing(6);
    outer->addLayout(m_body, 1);

    m_anim = new QVariantAnimation(this);
    m_anim->setEasingCurve(easingMech());
    connect(m_anim, &QVariantAnimation::valueChanged, this, [this](const QVariant& value) {
        m_tab = value.toReal();
        update();
    });
}

void EfPanel::animateTabTo(qreal target) {
    if (reduceMotion()) {
        m_tab = target;
        update();
        return;
    }
    m_anim->stop();
    m_anim->setStartValue(m_tab);
    m_anim->setEndValue(target);
    m_anim->setDuration(kTabMs);
    m_anim->start();
}

void EfPanel::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    const Palette& c = ef::palette();
    const QRectF r = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    p.fillRect(r, c.panel);
    p.setPen(QPen(c.rule, 1.0));
    p.setBrush(Qt::NoBrush);
    p.drawRect(r);

    const qreal tabWidth = 36.0 + (r.width() - 36.0) * m_tab;
    p.setPen(Qt::NoPen);
    p.setBrush(c.ink);
    p.drawRect(QRectF(r.left(), r.top(), std::max<qreal>(36.0, tabWidth), 5.0));
}

void EfPanel::enterEvent(QEnterEvent* event) {
    animateTabTo(1.0);
    QFrame::enterEvent(event);
}

void EfPanel::leaveEvent(QEvent* event) {
    animateTabTo(0.0);
    QFrame::leaveEvent(event);
}

EfMetrics::EfMetrics(int columns, QWidget* parent)
    : QWidget(parent), m_columns(std::max(1, columns)) {
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setFocusPolicy(Qt::NoFocus);
}

void EfMetrics::setMetrics(const QVector<Metric>& metrics) {
    m_metrics = metrics;
    QStringList accessible;
    for (const Metric& m : m_metrics) {
        accessible.append(QStringLiteral("%1 %2 %3%4")
                              .arg(m.label, m.value, m.unit,
                                   m.alert ? QStringLiteral("，需要注意") : QString()));
    }
    setAccessibleName(accessible.join(QStringLiteral("，")));
    updateGeometry();
    update();
}

QSize EfMetrics::sizeHint() const {
    const int count = static_cast<int>(m_metrics.size());
    const int rows = std::max(1, (count + m_columns - 1) / m_columns);
    return QSize(m_columns * 108, rows * 60 + 6);
}

QSize EfMetrics::minimumSizeHint() const {
    return sizeHint();
}

void EfMetrics::paintEvent(QPaintEvent*) {
    if (m_metrics.isEmpty()) {
        return;
    }
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    const Palette& c = ef::palette();
    const int count = static_cast<int>(m_metrics.size());
    const int rows = std::max(1, (count + m_columns - 1) / m_columns);
    const qreal cellW = width() / static_cast<qreal>(m_columns);
    const qreal cellH = (height() - 6.0) / static_cast<qreal>(rows);
    const qreal top = 3.0;

    for (int i = 0; i < count; ++i) {
        const int col = i % m_columns;
        const int row = i / m_columns;
        const qreal x = col * cellW;
        const qreal y = top + row * cellH;
        const Metric& m = m_metrics[i];

        if (col > 0) {
            p.setPen(QPen(c.rule, 1.0));
            p.drawLine(QPointF(x, y + 8.0), QPointF(x, y + cellH - 8.0));
        }
        if (row > 0) {
            p.setPen(QPen(c.rule, 1.0));
            p.drawLine(QPointF(x + 10.0, y), QPointF(x + cellW - 10.0, y));
        }

        p.setPen(c.ink2);
        QFont f = ef::font(FontRole::Eyebrow);
        f.setPixelSize(8);
        p.setFont(f);
        p.drawText(QRectF(x + 10.0, y + 5.0, cellW - 20.0, 9.0),
                   Qt::AlignLeft | Qt::AlignVCenter, m.eyebrow.toUpper());

        QFont valueFont = ef::font(FontRole::Numeral);
        int valuePx = 24;
        valueFont.setPixelSize(valuePx);
        while (valuePx > 15 &&
               QFontMetrics(valueFont).horizontalAdvance(m.value) > cellW - 26.0) {
            valuePx -= 2;
            valueFont.setPixelSize(valuePx);
        }
        p.setFont(valueFont);
        const QFontMetrics valueMetrics(valueFont);
        const int valueWidth = valueMetrics.horizontalAdvance(m.value);
        const QRectF valueRect(x + 10.0, y + 16.0, valueWidth + 8.0, 25.0);
        if (m.alert) {
            p.fillRect(valueRect, c.signal);
        }
        p.setPen(m.alert ? c.onSignal : c.ink);
        p.drawText(QRectF(x + 14.0, y + 16.0, valueWidth + 2.0, 25.0),
                   Qt::AlignLeft | Qt::AlignVCenter, m.value);

        const QFont smallFont = ef::font(FontRole::Small);
        p.setFont(smallFont);
        p.setPen(c.ink2);
        p.drawText(QRectF(x + 14.0 + valueWidth + 4.0, y + 22.0, cellW, 14.0),
                   Qt::AlignLeft | Qt::AlignVCenter, m.unit);

        p.setPen(c.ink2);
        const QString label = QFontMetrics(smallFont).elidedText(
            m.label, Qt::ElideRight, static_cast<int>(cellW - 20.0));
        p.drawText(QRectF(x + 10.0, y + 44.0, cellW - 20.0, 12.0),
                   Qt::AlignLeft | Qt::AlignVCenter, label);
    }
}

EfTriStrip::EfTriStrip(QWidget* parent) : QWidget(parent) {
    setFixedHeight(3);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setFocusPolicy(Qt::NoFocus);
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
}

void EfTriStrip::paintEvent(QPaintEvent*) {
    QPainter p(this);
    drawTriStrip(p, QRectF(rect()));
}

EfCommandBar::EfCommandBar(const QString& seal, const QString& brand,
                           const QString& brandSub, const QString& sectionEn,
                           const QString& sectionZh, QWidget* parent)
    : QFrame(parent) {
    setFrameShape(QFrame::NoFrame);
    setFixedHeight(54);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(16, 0, 16, 0);
    layout->setSpacing(12);

    m_seal = new Seal(seal, this);
    layout->addWidget(m_seal);

    auto* brandBox = new QWidget(this);
    auto* brandLayout = new QVBoxLayout(brandBox);
    brandLayout->setContentsMargins(0, 0, 0, 0);
    brandLayout->setSpacing(0);
    auto* brandLabel = new QLabel(brand.toUpper(), brandBox);
    brandLabel->setObjectName(QStringLiteral("efBrand"));
    brandLabel->setFont(ef::font(FontRole::Button));
    auto* subLabel = new QLabel(brandSub, brandBox);
    subLabel->setObjectName(QStringLiteral("efBrandSub"));
    subLabel->setFont(ef::font(FontRole::Small));
    brandLayout->addWidget(brandLabel);
    brandLayout->addWidget(subLabel);
    layout->addWidget(brandBox);

    m_section = new SectionTag(sectionEn, sectionZh, this);
    layout->addWidget(m_section);
    layout->addStretch(1);

    m_fields = new QLabel(this);
    m_fields->setObjectName(QStringLiteral("efBarField"));
    m_fields->setFont(ef::font(FontRole::Field));
    m_fields->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    layout->addWidget(m_fields);

    m_theme = new EfButton(isDark() ? QStringLiteral("浅色") : QStringLiteral("深色"),
                           EfButton::Variant::OnDark, this);
    m_theme->setFixedSize(64, 28);
    m_theme->setMinimumHeight(28);
    layout->addWidget(m_theme);
}

void EfCommandBar::setFields(const QString& text) {
    if (m_fields != nullptr) {
        m_fields->setText(text);
        m_fields->setAccessibleName(text);
    }
}

void EfCommandBar::paintEvent(QPaintEvent*) {
    QPainter p(this);
    const Palette& c = ef::palette();
    p.fillRect(rect(), c.commandBar);
    p.setPen(QPen(c.commandBarRule, 1.0));
    p.drawLine(0, height() - 1, width(), height() - 1);
}

QFrame* makeRule(QWidget* parent) {
    auto* rule = new QFrame(parent);
    rule->setObjectName(QStringLiteral("efRule"));
    rule->setFixedHeight(1);
    rule->setAttribute(Qt::WA_StyledBackground, true);
    rule->setFocusPolicy(Qt::NoFocus);
    return rule;
}

}  // namespace ef
