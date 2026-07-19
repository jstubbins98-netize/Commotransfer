#include "DropZone.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QMimeData>
#include <QPainter>
#include <QPainterPath>
#include <QUrl>

DropZone::DropZone(QWidget *parent)
    : QLabel(parent)
{
    setAcceptDrops(true);
    setAlignment(Qt::AlignCenter);
    setMinimumSize(400, 140);
    setCursor(Qt::PointingHandCursor);
    updateAppearance();
}

// ── Public API ────────────────────────────────────────────────────────────────

void DropZone::setFile(const QString &path)
{
    m_currentFile = path;
    updateAppearance();
    emit fileSelected(path);
}

void DropZone::clearFile()
{
    m_currentFile.clear();
    updateAppearance();
}

// ── Drag & drop ───────────────────────────────────────────────────────────────

bool DropZone::acceptableMimeData(const QMimeData *data) const
{
    if (!data->hasUrls()) return false;
    for (const QUrl &u : data->urls()) {
        if (!u.isLocalFile()) return false;
        if (!u.toLocalFile().endsWith(".prg", Qt::CaseInsensitive)) return false;
    }
    return !data->urls().isEmpty();
}

void DropZone::dragEnterEvent(QDragEnterEvent *event)
{
    if (acceptableMimeData(event->mimeData())) {
        event->acceptProposedAction();
        m_dragOver = true;
        update();
    } else {
        event->ignore();
    }
}

void DropZone::dragLeaveEvent(QDragLeaveEvent *event)
{
    m_dragOver = false;
    update();
    QLabel::dragLeaveEvent(event);
}

void DropZone::dropEvent(QDropEvent *event)
{
    m_dragOver = false;
    if (!acceptableMimeData(event->mimeData())) { event->ignore(); return; }
    event->acceptProposedAction();
    // Use the first .prg file dropped
    QString path = event->mimeData()->urls().first().toLocalFile();
    setFile(path);
}

// ── Click → file picker ───────────────────────────────────────────────────────

void DropZone::mousePressEvent(QMouseEvent *event)
{
    QLabel::mousePressEvent(event);
    QString path = QFileDialog::getOpenFileName(
        this,
        tr("Select a .prg file"),
        QDir::homePath(),
        tr("Commodore PRG files (*.prg);;All files (*)")
    );
    if (!path.isEmpty())
        setFile(path);
}

// ── Painting ──────────────────────────────────────────────────────────────────

void DropZone::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QColor bg, border, textColor;
    bool isDark = palette().window().color().lightness() < 128;

    if (m_dragOver) {
        bg     = isDark ? QColor(30, 80, 160, 60) : QColor(0, 100, 220, 30);
        border = QColor(0, 120, 255);
    } else if (!m_currentFile.isEmpty()) {
        bg     = isDark ? QColor(20, 80, 40, 60) : QColor(0, 160, 60, 20);
        border = isDark ? QColor(40, 180, 80)     : QColor(0, 140, 50);
    } else {
        bg     = isDark ? QColor(255, 255, 255, 10) : QColor(0, 0, 0, 8);
        border = isDark ? QColor(255, 255, 255, 60) : QColor(0, 0, 0, 50);
    }

    textColor = isDark ? QColor(230, 230, 230) : QColor(40, 40, 40);

    // Background
    QPainterPath path;
    path.addRoundedRect(rect().adjusted(2, 2, -2, -2), 12, 12);
    p.fillPath(path, bg);

    // Dashed border
    QPen pen(border, 1.8, Qt::DashLine);
    pen.setDashPattern({6, 4});
    p.setPen(pen);
    p.drawPath(path);

    // Text
    p.setPen(textColor);

    if (!m_currentFile.isEmpty()) {
        QFont boldFont = font();
        boldFont.setPointSize(boldFont.pointSize() + 1);
        boldFont.setBold(true);
        p.setFont(boldFont);
        p.drawText(rect().adjusted(12, 0, -12, -18), Qt::AlignCenter,
                   QFileInfo(m_currentFile).fileName());

        QFont smallFont = font();
        smallFont.setPointSize(smallFont.pointSize() - 1);
        p.setFont(smallFont);
        p.setPen(border);
        p.drawText(rect().adjusted(12, 20, -12, 0), Qt::AlignCenter,
                   QFileInfo(m_currentFile).absolutePath());
    } else {
        QFont iconFont = font();
        iconFont.setPointSize(iconFont.pointSize() + 8);
        p.setFont(iconFont);
        QColor dimmed = textColor;
        dimmed.setAlpha(120);
        p.setPen(dimmed);
        p.drawText(rect().adjusted(0, 0, 0, -30), Qt::AlignCenter, "⬇");

        QFont labelFont = font();
        p.setFont(labelFont);
        p.setPen(textColor);
        p.drawText(rect().adjusted(0, 30, 0, 0), Qt::AlignCenter,
                   m_dragOver ? "Release to load file" : "Drop a .prg file here\nor click to browse");
    }
}

void DropZone::updateAppearance()
{
    update(); // triggers paintEvent
}
