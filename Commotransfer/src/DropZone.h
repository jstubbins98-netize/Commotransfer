#pragma once

#include <QLabel>
#include <QStringList>

/**
 * DropZone
 *
 * A styled QLabel that accepts drag-and-drop of .prg files and also lets the
 * user click to open a file picker.
 */
class DropZone : public QLabel
{
    Q_OBJECT

public:
    explicit DropZone(QWidget *parent = nullptr);

    /** Currently selected file path, or empty string. */
    QString currentFile() const { return m_currentFile; }

    /** Programmatically set the file (e.g. from a menu action). */
    void setFile(const QString &path);

    /** Clear the selection back to the placeholder state. */
    void clearFile();

signals:
    void fileSelected(const QString &path);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    void updateAppearance();
    bool acceptableMimeData(const class QMimeData *data) const;

    QString m_currentFile;
    bool    m_dragOver = false;
};
