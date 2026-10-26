#pragma once

// 界面外观（Endfield-flavoured console）。
// 变体：Klein blue（accent #002FA7 / cool blue paper / standard cut / contours /
// standard density / full motion）。深色主题把强调色提亮为 #7FA0FF。
// 主视觉：题 2 为主通道 + 临时栈 + 三条队列；题 7 为校园路网与调度路线。
// 页面类型：控制台。
//
// 颜色、字体、切角、间距和动效集中在这里；两个程序共用。
// 拉丁字体 Saira（SIL OFL 1.1）已切成静态实例，许可证见 fonts/Saira-OFL.txt。

#include <QColor>
#include <QEasingCurve>
#include <QFont>
#include <QPainterPath>
#include <QRectF>
#include <QString>
#include <QStringList>
#include <QVector>

class QApplication;
class QLabel;
class QPainter;
class QWidget;

namespace ef {

enum class Mode { Light, Dark };

struct Palette {
    QColor paper;
    QColor panel;
    QColor ink;
    QColor ink2;
    QColor rule;
    QColor chip;
    QColor signal;
    QColor onSignal;
    QColor trace;
    QColor tagBg;
    QColor tagFg;
    QColor danger;
    QColor commandBar;
    QColor commandBarText;
    QColor commandBarMuted;
    QColor commandBarRule;
    QColor triA;
    QColor triB;
    QColor triC;
};

enum class FontRole {
    Eyebrow,   // 宽体大写英文小字
    Field,     // 字段、编号、计数
    Label,     // 中文/英文界面标签
    Body,      // 正文
    Heading,   // 面板标题
    Display,   // 大字标
    Button,    // 按钮文字
    Numeral,   // 大数字
    Small      // 说明、图例
};

const Palette& palette();
bool isDark();
void setDark(bool dark);
void toggleMode();

// 加载字体、设置调色板和全局样式表；两个 main() 在创建窗口前调用一次。
void apply(QApplication& app);

// 重新绘制所有控件（主题切换后调用）。
void repaintAll();

QFont font(FontRole role);
QString fields(const QStringList& parts);

QLabel* makeEyebrow(const QString& text, QWidget* parent = nullptr);
QLabel* makeTitle(const QString& text, QWidget* parent = nullptr);
QLabel* makeField(const QString& text, QWidget* parent = nullptr);
QLabel* makeHint(const QString& text, QWidget* parent = nullptr);

QPainterPath cutPath(const QRectF& rect, qreal cut);
void drawCutRect(QPainter& p, const QRectF& rect, qreal cut, const QBrush& fill,
                 const QPen& pen);
void drawHatch(QPainter& p, const QRectF& rect, const QColor& ink, qreal opacity = 0.22,
               qreal period = 6.5);
void drawContours(QPainter& p, const QRectF& rect, const QColor& ink,
                  qreal opacity = 0.09);
void drawTriStrip(QPainter& p, const QRectF& rect);

QEasingCurve easingSnap();
QEasingCurve easingMech();
bool reduceMotion();

// 与 tokens.json 对应的常用时长。
constexpr int kPressMs = 100;
constexpr int kWipeMs = 280;
constexpr int kExitMs = 180;
constexpr int kTabMs = 400;
constexpr int kRowMs = 200;

}  // namespace ef
