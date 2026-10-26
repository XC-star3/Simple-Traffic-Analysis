#pragma once

#include <QFrame>
#include <QPushButton>
#include <QStyledItemDelegate>
#include <QVariantAnimation>
#include <QVector>
#include <QWidget>

#include "common/theme.h"

class QAbstractItemModel;
class QEnterEvent;
class QEvent;
class QFocusEvent;
class QLabel;
class QModelIndex;
class QMouseEvent;
class QPaintEvent;
class QPainter;
class QStyleOptionViewItem;
class QVBoxLayout;

namespace ef {

class EfButton : public QPushButton {
public:
    enum class Variant { Primary, Secondary, Danger, OnDark };

    explicit EfButton(const QString& text, Variant variant = Variant::Secondary,
                      QWidget* parent = nullptr);

    void setVariant(Variant variant);
    Variant variant() const { return m_variant; }

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
    bool hitButton(const QPoint& pos) const override;

private:
    void animateTo(qreal target, int duration);

    Variant m_variant = Variant::Secondary;
    qreal m_wipe = 0.0;
    bool m_hover = false;
    QVariantAnimation* m_anim = nullptr;
};

// QListWidget / QListView 的统一行绘制：1px 分隔线、可选勾选块、强调色实心行。
// 让 item 的勾选态和告警底色不再依赖样式表的 ::item 规则（那一层会吞掉
// setBackground 画刷），切换深浅主题时也不会留下旧颜色。
class EfListDelegate : public QStyledItemDelegate {
public:
    explicit EfListDelegate(QObject* parent = nullptr);

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option,
                   const QModelIndex& index) const override;
    bool editorEvent(QEvent* event, QAbstractItemModel* model,
                     const QStyleOptionViewItem& option,
                     const QModelIndex& index) override;
};

class EfPanel : public QFrame {
public:
    EfPanel(const QString& eyebrow, const QString& title, QWidget* parent = nullptr);

    QVBoxLayout* body() const { return m_body; }

protected:
    void paintEvent(QPaintEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    void animateTabTo(qreal target);

    QVBoxLayout* m_body = nullptr;
    QWidget* m_header = nullptr;
    qreal m_tab = 0.0;
    QVariantAnimation* m_anim = nullptr;
};

struct Metric {
    QString eyebrow;
    QString value;
    QString unit;
    QString label;
    bool alert = false;
};

class EfMetrics : public QWidget {
public:
    explicit EfMetrics(int columns = 3, QWidget* parent = nullptr);

    void setMetrics(const QVector<Metric>& metrics);
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QVector<Metric> m_metrics;
    int m_columns = 3;
};

class EfTriStrip : public QWidget {
public:
    explicit EfTriStrip(QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override;
};

class EfCommandBar : public QFrame {
public:
    EfCommandBar(const QString& seal, const QString& brand, const QString& brandSub,
                 const QString& sectionEn, const QString& sectionZh,
                 QWidget* parent = nullptr);

    void setFields(const QString& text);
    EfButton* themeButton() const { return m_theme; }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QWidget* m_seal = nullptr;
    QWidget* m_section = nullptr;
    QLabel* m_fields = nullptr;
    EfButton* m_theme = nullptr;
};

QFrame* makeRule(QWidget* parent = nullptr);

}  // namespace ef
