#pragma once

#include <QApplication>
#include <QColor>
#include <QFont>
#include <QPalette>

// 统一浅色主题：避免 Windows 深色模式导致表格/日志黑底黑字。
inline void applyAppTheme(QApplication& app) {
    app.setStyle(QStringLiteral("Fusion"));

    QPalette pal = app.palette();
    pal.setColor(QPalette::Window, QColor(0xF5, 0xF7, 0xFA));
    pal.setColor(QPalette::WindowText, QColor(0x1F, 0x2A, 0x37));
    pal.setColor(QPalette::Base, QColor(0xFF, 0xFF, 0xFF));
    pal.setColor(QPalette::AlternateBase, QColor(0xF3, 0xF6, 0xFA));
    pal.setColor(QPalette::Text, QColor(0x1F, 0x2A, 0x37));
    pal.setColor(QPalette::Button, QColor(0xFF, 0xFF, 0xFF));
    pal.setColor(QPalette::ButtonText, QColor(0x1F, 0x2A, 0x37));
    pal.setColor(QPalette::Highlight, QColor(0x2F, 0x80, 0xED));
    pal.setColor(QPalette::HighlightedText, QColor(0xFF, 0xFF, 0xFF));
    pal.setColor(QPalette::ToolTipBase, QColor(0xFF, 0xFF, 0xFF));
    pal.setColor(QPalette::ToolTipText, QColor(0x1F, 0x2A, 0x37));
    app.setPalette(pal);

    app.setStyleSheet(QStringLiteral(R"(
QGroupBox {
    background-color: #FFFFFF;
    border: 1px solid #DCE3EC;
    border-radius: 8px;
    margin-top: 12px;
    padding-top: 6px;
}
QGroupBox::title {
    subcontrol-origin: margin;
    subcontrol-position: top left;
    left: 12px;
    padding: 0 4px;
    color: #334155;
    font-weight: 600;
}
QLineEdit, QSpinBox, QDoubleSpinBox, QComboBox, QPlainTextEdit,
QListWidget, QTableWidget {
    background-color: #FFFFFF;
    color: #1F2A37;
    border: 1px solid #CBD5E1;
    border-radius: 4px;
    selection-background-color: #2F80ED;
    selection-color: #FFFFFF;
}
QPushButton {
    background-color: #2F80ED;
    color: #FFFFFF;
    border: none;
    border-radius: 5px;
    padding: 6px 10px;
}
QPushButton:hover { background-color: #2563EB; }
QPushButton:pressed { background-color: #1D4ED8; }
QHeaderView::section {
    background-color: #EEF2F7;
    color: #334155;
    border: none;
    padding: 4px;
}
QTableWidget { gridline-color: #E2E8F0; }
QStatusBar { background-color: #EEF2F7; color: #334155; }
QScrollBar:vertical { background: #F1F5F9; width: 10px; }
QScrollBar::handle:vertical { background: #CBD5E1; border-radius: 5px; }
)"));
}
