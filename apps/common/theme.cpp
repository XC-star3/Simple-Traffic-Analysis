#include "common/theme.h"

#include <QApplication>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QLabel>
#include <QPainter>
#include <QPalette>
#include <QStyleFactory>
#include <QWidget>

#include <algorithm>
#include <cmath>

#ifdef Q_OS_WIN
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#endif

namespace ef {
namespace {

bool g_dark = false;
bool g_fontsLoaded = false;
bool g_styleSet = false;

QString uiFamily() {
    static const QString family = [] {
        const QStringList families = QFontDatabase::families();
        if (families.contains(QStringLiteral("Microsoft YaHei UI"))) {
            return QStringLiteral("Microsoft YaHei UI");
        }
        if (families.contains(QStringLiteral("Microsoft YaHei"))) {
            return QStringLiteral("Microsoft YaHei");
        }
        return QFontDatabase::systemFont(QFontDatabase::GeneralFont).family();
    }();
    return family;
}

QString sairaFamily(const char* wanted) {
    static const QStringList families = QFontDatabase::families();
    const QString name = QString::fromLatin1(wanted);
    if (families.contains(name)) {
        return name;
    }
    if (families.contains(QStringLiteral("Saira"))) {
        return QStringLiteral("Saira");
    }
    return uiFamily();
}

void loadFonts() {
    if (g_fontsLoaded) {
        return;
    }
    g_fontsLoaded = true;
    const char* files[] = {
        ":/fonts/Saira-Wide-SemiBold.ttf",
        ":/fonts/Saira-Field.ttf",
        ":/fonts/Saira-Bold.ttf",
        ":/fonts/Saira-Wide-ExtraBold.ttf",
    };
    for (const char* file : files) {
        const int id = QFontDatabase::addApplicationFont(QString::fromLatin1(file));
        if (id < 0) {
            qWarning("ef: cannot load font %s", file);
        }
    }
}

QString styleSheet() {
    const Palette& c = palette();
    QString css = QStringLiteral(R"CSS(
* { outline: none; }
QMainWindow, QDialog { background: @paper; }
QWidget { color: @ink; font-size: 13px; }
QLabel { color: @ink2; background: transparent; font-size: 12px; }
QLabel#efEyebrow { color: @ink2; font-size: 10px; font-weight: 600; }
QLabel#efPanelTitle { color: @ink; font-size: 14px; font-weight: 700; }
QLabel#efField { color: @ink2; font-size: 12px; }
QLabel#efHint { color: @ink2; font-size: 11px; }
QLabel#efBarField { color: @commandText; font-size: 11px; }
QLabel#efBrand { color: @commandText; font-size: 15px; font-weight: 700; }
QLabel#efBrandSub { color: @commandMuted; font-size: 10px; }
QLabel#efDisplay { color: @ink; font-size: 40px; font-weight: 800; }
QLabel#efNumeral { color: @signal; font-size: 44px; font-weight: 800; }
QLabel#efError { color: @onSignal; background: @signal; padding: 6px 8px; border-left: 4px solid @ink; }
QLabel#efAlertCell { color: @onSignal; background: @signal; font-weight: 600; }
QFrame#efRule { background: @rule; border: none; }
QFrame#efBarRule { background: @commandRule; border: none; }

QLineEdit, QSpinBox, QDoubleSpinBox, QComboBox, QPlainTextEdit {
    background: @panel;
    color: @ink;
    border: 1px solid @rule;
    border-radius: 0;
    padding: 5px 7px;
    selection-background-color: @signal;
    selection-color: @onSignal;
}
QLineEdit:hover, QSpinBox:hover, QDoubleSpinBox:hover, QComboBox:hover,
QPlainTextEdit:hover { border-color: @ink2; }
QLineEdit:focus, QSpinBox:focus, QDoubleSpinBox:focus, QComboBox:focus,
QPlainTextEdit:focus { border: 2px solid @ink; padding: 4px 6px; }
QLineEdit:disabled, QSpinBox:disabled, QDoubleSpinBox:disabled,
QComboBox:disabled { color: @ink2; background: @chip; }

QSpinBox::up-button, QSpinBox::down-button,
QDoubleSpinBox::up-button, QDoubleSpinBox::down-button {
    width: 18px;
    background: @chip;
    border-left: 1px solid @rule;
}
QSpinBox::up-button:hover, QSpinBox::down-button:hover,
QDoubleSpinBox::up-button:hover, QDoubleSpinBox::down-button:hover {
    background: @rule;
}
QComboBox::drop-down {
    width: 22px;
    background: @chip;
    border-left: 1px solid @rule;
}
QComboBox::drop-down:hover { background: @rule; }
QComboBox QAbstractItemView {
    background: @panel;
    color: @ink;
    border: 1px solid @ink;
    selection-background-color: @signal;
    selection-color: @onSignal;
    padding: 2px;
}
QComboBox QAbstractItemView::item { min-height: 28px; padding: 2px 8px; }

QListWidget, QTableWidget {
    background: @panel;
    color: @ink;
    border: 1px solid @rule;
    border-radius: 0;
    gridline-color: @rule;
    alternate-background-color: @panel;
    selection-background-color: @signal;
    selection-color: @onSignal;
}
QTableWidget::item { border-bottom: 1px solid @rule; padding: 1px 4px; }
QTableWidget::item:selected { background: @signal; color: @onSignal; }
QHeaderView::section {
    background: @panel;
    color: @ink2;
    border: none;
    border-bottom: 1px solid @ink;
    padding: 5px 6px;
    font-size: 10px;
    font-weight: 600;
}
QTableCornerButton::section { background: @panel; border: none; }

QCheckBox { color: @ink; spacing: 8px; }
QCheckBox::indicator { width: 14px; height: 14px; }

QScrollBar:vertical { background: @paper; width: 10px; margin: 0; }
QScrollBar::handle:vertical { background: @ink2; min-height: 24px; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
QScrollBar:horizontal { background: @paper; height: 10px; margin: 0; }
QScrollBar::handle:horizontal { background: @ink2; min-width: 24px; }
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }
QScrollBar::add-page, QScrollBar::sub-page { background: transparent; }

QStatusBar { background: @panel; color: @ink2; border-top: 1px solid @rule; }
QStatusBar::item { border: none; }
QFrame#efStatusBar { background: @panel; border-top: 1px solid @rule; }
QLabel#efStatusText { color: @ink2; font-size: 11px; }
QToolTip {
    background: @commandBar;
    color: @commandText;
    border: 1px solid @rule;
    padding: 4px 6px;
}
QSplitter::handle { background: @rule; }
QSplitter::handle:hover { background: @ink2; }
QScrollArea { border: none; background: transparent; }
QScrollArea > QWidget > QWidget { background: transparent; }
QDialog QLabel { color: @ink2; }
QPushButton {
    border-radius: 0;
    padding: 6px 10px;
}
)CSS");

    css.replace(QStringLiteral("@paper"), c.paper.name());
    css.replace(QStringLiteral("@panel"), c.panel.name());
    css.replace(QStringLiteral("@ink2"), c.ink2.name());
    css.replace(QStringLiteral("@ink"), c.ink.name());
    css.replace(QStringLiteral("@rule"), c.rule.name());
    css.replace(QStringLiteral("@chip"), c.chip.name());
    css.replace(QStringLiteral("@signal"), c.signal.name());
    css.replace(QStringLiteral("@onSignal"), c.onSignal.name());
    css.replace(QStringLiteral("@danger"), c.danger.name());
    css.replace(QStringLiteral("@commandBar"), c.commandBar.name());
    css.replace(QStringLiteral("@commandText"), c.commandBarText.name());
    css.replace(QStringLiteral("@commandMuted"), c.commandBarMuted.name());
    css.replace(QStringLiteral("@commandRule"), c.commandBarRule.name());
    return css;
}

qreal bezierComponent(qreal t, qreal a, qreal b) {
    const qreal mt = 1.0 - t;
    return 3.0 * mt * mt * t * a + 3.0 * mt * t * t * b + t * t * t;
}

qreal solveX(qreal x, qreal x1, qreal x2) {
    qreal lo = 0.0;
    qreal hi = 1.0;
    for (int i = 0; i < 22; ++i) {
        const qreal mid = (lo + hi) * 0.5;
        if (bezierComponent(mid, x1, x2) < x) {
            lo = mid;
        } else {
            hi = mid;
        }
    }
    return (lo + hi) * 0.5;
}

qreal cubic(qreal x, qreal x1, qreal y1, qreal x2, qreal y2) {
    const qreal t = solveX(x, x1, x2);
    return bezierComponent(t, y1, y2);
}

qreal snapEase(qreal x) {
    return cubic(x, 0.16, 1.0, 0.3, 1.0);
}

qreal mechEase(qreal x) {
    return cubic(x, 0.7, 0.0, 0.2, 1.0);
}

}  // namespace

const Palette& palette() {
    // 克莱因蓝变体：冷蓝纸 + 深海军蓝墨 + 单一强调色 #002FA7。
    // 深蓝底上用纸白字；深色主题把强调色提亮为 #7FA0FF 以保持对比度。
    static const Palette light = {
        QColor(0xED, 0xF2, 0xFA), QColor(0xFF, 0xFF, 0xFF), QColor(0x0E, 0x1A, 0x30),
        QColor(0x4B, 0x5B, 0x76), QColor(0xCF, 0xD9, 0xE8), QColor(0xE2, 0xE9, 0xF3),
        QColor(0x00, 0x2F, 0xA7), QColor(0xF7, 0xF9, 0xFF), QColor(0x0E, 0x1A, 0x30),
        QColor(0x0E, 0x1A, 0x30), QColor(0xFF, 0xFF, 0xFF), QColor(0xB3, 0x26, 0x1E),
        QColor(0x0A, 0x17, 0x30), QColor(0xF2, 0xF6, 0xFC), QColor(0xA9, 0xB8, 0xD6),
        QColor(0x26, 0x38, 0x58), QColor(0xFF, 0x3D, 0x9A), QColor(0x00, 0x2F, 0xA7),
        QColor(0x00, 0xC2, 0xA8)};
    static const Palette dark = {
        QColor(0x0C, 0x14, 0x24), QColor(0x13, 0x1D, 0x31), QColor(0xE7, 0xED, 0xFA),
        QColor(0x93, 0xA2, 0xC4), QColor(0x2A, 0x38, 0x50), QColor(0x1B, 0x28, 0x42),
        QColor(0x7F, 0xA0, 0xFF), QColor(0x08, 0x10, 0x20), QColor(0x7F, 0xA0, 0xFF),
        QColor(0x1B, 0x28, 0x42), QColor(0xE7, 0xED, 0xFA), QColor(0xFF, 0x8A, 0x7A),
        QColor(0x08, 0x0F, 0x1E), QColor(0xE7, 0xED, 0xFA), QColor(0x87, 0x97, 0xB8),
        QColor(0x24, 0x30, 0x4A), QColor(0xFF, 0x3D, 0x9A), QColor(0x7F, 0xA0, 0xFF),
        QColor(0x00, 0xC2, 0xA8)};
    return g_dark ? dark : light;
}

bool isDark() {
    return g_dark;
}

void setDark(bool dark) {
    g_dark = dark;
}

void toggleMode() {
    setDark(!isDark());
}

void apply(QApplication& app) {
    loadFonts();
    if (!g_styleSet) {
        app.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
        g_styleSet = true;
    }

    const Palette& c = palette();
    QPalette pal = app.palette();
    pal.setColor(QPalette::Window, c.paper);
    pal.setColor(QPalette::WindowText, c.ink);
    pal.setColor(QPalette::Base, c.panel);
    pal.setColor(QPalette::AlternateBase, c.chip);
    pal.setColor(QPalette::Text, c.ink);
    pal.setColor(QPalette::Button, c.panel);
    pal.setColor(QPalette::ButtonText, c.ink);
    pal.setColor(QPalette::BrightText, c.onSignal);
    pal.setColor(QPalette::Highlight, c.signal);
    pal.setColor(QPalette::HighlightedText, c.onSignal);
    pal.setColor(QPalette::ToolTipBase, c.commandBar);
    pal.setColor(QPalette::ToolTipText, c.commandBarText);
    pal.setColor(QPalette::PlaceholderText, c.ink2);
    pal.setColor(QPalette::Link, c.ink);
    pal.setColor(QPalette::LinkVisited, c.ink2);
    pal.setColor(QPalette::Light, c.panel);
    pal.setColor(QPalette::Midlight, c.chip);
    pal.setColor(QPalette::Mid, c.rule);
    pal.setColor(QPalette::Dark, c.ink2);
    pal.setColor(QPalette::Shadow, c.ink);
    pal.setColor(QPalette::Disabled, QPalette::Text, c.ink2);
    pal.setColor(QPalette::Disabled, QPalette::WindowText, c.ink2);
    pal.setColor(QPalette::Disabled, QPalette::ButtonText, c.ink2);
    app.setPalette(pal);
    app.setFont(font(FontRole::Body));
    app.setStyleSheet(styleSheet());
    repaintAll();
}

void repaintAll() {
    for (QWidget* w : QApplication::allWidgets()) {
        w->update();
    }
}

QFont font(FontRole role) {
    loadFonts();
    const QString cjk = uiFamily();
    const QString semi = sairaFamily("Saira Wide SemiBold");
    const QString field = sairaFamily("Saira Field");
    const QString bold = sairaFamily("Saira Bold");
    const QString extra = sairaFamily("Saira Wide ExtraBold");

    QFont f;
    switch (role) {
        case FontRole::Eyebrow:
            f.setFamilies({semi, cjk});
            f.setPixelSize(10);
            f.setWeight(QFont::DemiBold);
            f.setCapitalization(QFont::AllUppercase);
            f.setLetterSpacing(QFont::PercentageSpacing, 114);
            break;
        case FontRole::Field:
            f.setFamilies({field, cjk});
            f.setPixelSize(12);
            f.setWeight(QFont::Normal);
            f.setFeature(QFont::Tag("tnum"), 1);
            break;
        case FontRole::Label:
            f.setFamilies({semi, cjk});
            f.setPixelSize(12);
            f.setWeight(QFont::DemiBold);
            break;
        case FontRole::Body:
            f.setFamilies({field, cjk});
            f.setPixelSize(13);
            f.setWeight(QFont::Normal);
            break;
        case FontRole::Heading:
            f.setFamilies({extra, cjk});
            f.setPixelSize(16);
            f.setWeight(QFont::Bold);
            break;
        case FontRole::Display:
            f.setFamilies({extra, cjk});
            f.setPixelSize(30);
            f.setWeight(QFont::Black);
            break;
        case FontRole::Button:
            f.setFamilies({bold, cjk});
            f.setPixelSize(13);
            f.setWeight(QFont::Bold);
            f.setFeature(QFont::Tag("tnum"), 1);
            break;
        case FontRole::Numeral:
            f.setFamilies({extra, cjk});
            f.setPixelSize(27);
            f.setWeight(QFont::Black);
            f.setFeature(QFont::Tag("tnum"), 1);
            break;
        case FontRole::Small:
            f.setFamilies({field, cjk});
            f.setPixelSize(10);
            f.setWeight(QFont::Normal);
            break;
    }
    f.setStyleStrategy(QFont::PreferAntialias);
    return f;
}

QString fields(const QStringList& parts) {
    QStringList cleaned;
    for (const QString& part : parts) {
        if (!part.isEmpty()) {
            cleaned.append(part);
        }
    }
    return cleaned.join(QStringLiteral("  \u2502  "));
}

QLabel* makeEyebrow(const QString& text, QWidget* parent) {
    auto* label = new QLabel(text.startsWith(QStringLiteral("//"))
                                 ? text
                                 : QStringLiteral("// %1").arg(text.toUpper()),
                             parent);
    label->setObjectName(QStringLiteral("efEyebrow"));
    label->setFont(font(FontRole::Eyebrow));
    label->setTextInteractionFlags(Qt::NoTextInteraction);
    return label;
}

QLabel* makeTitle(const QString& text, QWidget* parent) {
    auto* label = new QLabel(text, parent);
    label->setObjectName(QStringLiteral("efPanelTitle"));
    label->setFont(font(FontRole::Heading));
    return label;
}

QLabel* makeField(const QString& text, QWidget* parent) {
    auto* label = new QLabel(text, parent);
    label->setObjectName(QStringLiteral("efField"));
    label->setFont(font(FontRole::Field));
    return label;
}

QLabel* makeHint(const QString& text, QWidget* parent) {
    auto* label = new QLabel(text, parent);
    label->setObjectName(QStringLiteral("efHint"));
    label->setFont(font(FontRole::Small));
    label->setWordWrap(true);
    return label;
}

QPainterPath cutPath(const QRectF& rect, qreal cut) {
    QPainterPath path;
    const qreal c = std::max<qreal>(0.0, std::min(cut, std::min(rect.width(), rect.height()) / 2.0));
    path.moveTo(rect.left(), rect.top());
    path.lineTo(rect.right(), rect.top());
    path.lineTo(rect.right(), rect.bottom() - c);
    path.lineTo(rect.right() - c, rect.bottom());
    path.lineTo(rect.left(), rect.bottom());
    path.closeSubpath();
    return path;
}

void drawCutRect(QPainter& p, const QRectF& rect, qreal cut, const QBrush& fill,
                 const QPen& pen) {
    const QPainterPath path = cutPath(rect, cut);
    p.setPen(Qt::NoPen);
    p.setBrush(fill);
    p.drawPath(path);
    p.setBrush(Qt::NoBrush);
    p.setPen(pen);
    p.drawPath(path);
}

void drawHatch(QPainter& p, const QRectF& rect, const QColor& ink, qreal opacity,
               qreal period) {
    if (rect.isEmpty()) {
        return;
    }
    QColor line = ink;
    line.setAlphaF(static_cast<float>(std::clamp(opacity, 0.0, 1.0)));
    p.save();
    p.setClipRect(rect);
    QPen pen(line);
    pen.setWidthF(1.5);
    p.setPen(pen);
    const qreal step = period * std::sqrt(2.0);
    for (qreal x = rect.left() - rect.height(); x < rect.right() + rect.height();
         x += step) {
        p.drawLine(QPointF(x, rect.bottom()), QPointF(x + rect.height(), rect.top()));
    }
    p.restore();
}

void drawContours(QPainter& p, const QRectF& rect, const QColor& ink, qreal opacity) {
    if (rect.isEmpty()) {
        return;
    }
    QColor line = ink;
    line.setAlphaF(static_cast<float>(std::clamp(opacity, 0.0, 1.0)));
    QPen pen(line);
    pen.setWidthF(1.0);
    p.save();
    p.setClipRect(rect);
    p.setBrush(Qt::NoBrush);
    p.setPen(pen);

    // 右上角
    const qreal w = rect.width();
    for (int i = 0; i < 4; ++i) {
        const qreal o = i * 16.0;
        QPainterPath path;
        path.moveTo(rect.right() - 6.0 - o, rect.top() + 18.0 + o * 0.35);
        path.cubicTo(rect.right() - w * 0.22, rect.top() + 40.0 + o,
                     rect.right() - w * 0.10, rect.top() + 8.0 + o,
                     rect.right() - 4.0, rect.top() + 74.0 + o);
        p.drawPath(path);
    }
    // 左下角
    for (int i = 0; i < 3; ++i) {
        const qreal o = i * 18.0;
        QPainterPath path;
        path.moveTo(rect.left() + 8.0, rect.bottom() - 18.0 - o);
        path.cubicTo(rect.left() + w * 0.08, rect.bottom() - 46.0 - o,
                     rect.left() + w * 0.18, rect.bottom() - 20.0 - o,
                     rect.left() + w * 0.30, rect.bottom() - 4.0 - o * 0.4);
        p.drawPath(path);
    }
    p.restore();
}

void drawTriStrip(QPainter& p, const QRectF& rect) {
    const qreal third = rect.width() / 3.0;
    p.save();
    p.setPen(Qt::NoPen);
    p.setBrush(palette().triA);
    p.drawRect(QRectF(rect.left(), rect.top(), third, rect.height()));
    p.setBrush(palette().triB);
    p.drawRect(QRectF(rect.left() + third, rect.top(), third, rect.height()));
    p.setBrush(palette().triC);
    p.drawRect(QRectF(rect.left() + third * 2.0, rect.top(), third, rect.height()));
    p.restore();
}

QEasingCurve easingSnap() {
    QEasingCurve curve(QEasingCurve::Custom);
    curve.setCustomType(&snapEase);
    return curve;
}

QEasingCurve easingMech() {
    QEasingCurve curve(QEasingCurve::Custom);
    curve.setCustomType(&mechEase);
    return curve;
}

bool reduceMotion() {
    static const bool reduce = [] {
        if (qEnvironmentVariableIsSet("DS_REDUCE_MOTION")) {
            return qEnvironmentVariableIntValue("DS_REDUCE_MOTION") > 0;
        }
#ifdef Q_OS_WIN
        BOOL animations = TRUE;
        if (SystemParametersInfoW(SPI_GETCLIENTAREAANIMATION, 0, &animations, 0)) {
            return animations == FALSE;
        }
#endif
        return false;
    }();
    return reduce;
}

}  // namespace ef
