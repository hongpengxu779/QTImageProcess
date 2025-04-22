#include "ProcessingWidget.h"
#include <QMouseEvent>
#include <QPainter>
#include <QDebug>

void ProcessingWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_currentImage.isNull()) {
        QPoint pos = event->pos();
        if (pos.x() >= 0 && pos.x() < m_currentImage.width() &&
            pos.y() >= 0 && pos.y() < m_currentImage.height()) {
            QRgb pixel = m_currentImage.pixel(pos);
            int grayValue = qGray(pixel);
            emit mouseMoved(pos, grayValue);
        }
    }
    QWidget::mouseMoveEvent(event);
}

void ProcessingWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.fillRect(rect(), Qt::black);
    
    if (!m_currentImage.isNull()) {
        QRect targetRect = m_currentImage.rect();
        targetRect.moveCenter(rect().center());
        painter.drawImage(targetRect, m_currentImage);
    }
}

void ProcessingWidget::updateImageStats()
{
    if (m_currentImage.isNull()) {
        return;
    }

    double sum = 0;
    int count = 0;
    
    for (int y = 0; y < m_currentImage.height(); ++y) {
        for (int x = 0; x < m_currentImage.width(); ++x) {
            QRgb pixel = m_currentImage.pixel(x, y);
            sum += qGray(pixel);
            ++count;
        }
    }
    
    if (count > 0) {
        double meanValue = sum / count;
        emit imageStatsUpdated(meanValue);
    }
} 