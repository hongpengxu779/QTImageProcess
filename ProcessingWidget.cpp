#include "ProcessingWidget.h"
#include <QMouseEvent>
#include <QPainter>
#include <QDebug>

void ProcessingWidget::mouseMoveEvent(QMouseEvent *event)
{
    try {
        if (!m_currentImage.isNull() && imageLabel) {
            // 获取QLabel中的图像显示区域
            QRect imageRect = imageLabel->contentsRect();
            QSize imageSize = m_currentImage.size();

            // 计算相对于QLabel的鼠标位置
            QPoint labelPos = imageLabel->mapFrom(this, event->pos());

            // 确保鼠标在图像区域内
            if (labelPos.x() >= 0 && labelPos.x() < imageRect.width() &&
                labelPos.y() >= 0 && labelPos.y() < imageRect.height()) {
                
                // 计算缩放比例
                double scaleX = static_cast<double>(imageSize.width()) / imageRect.width();
                double scaleY = static_cast<double>(imageSize.height()) / imageRect.height();
                
                // 计算实际图像坐标
                int imageX = static_cast<int>(labelPos.x() * scaleX);
                int imageY = static_cast<int>(labelPos.y() * scaleY);
                
                // 确保坐标在图像范围内
                imageX = qBound(0, imageX, imageSize.width() - 1);
                imageY = qBound(0, imageY, imageSize.height() - 1);
                
                QColor pixelColor = m_currentImage.pixelColor(imageX, imageY);
                int grayValue = qGray(pixelColor.rgb());
                
                emit mouseMoved(QPoint(imageX, imageY), grayValue);
            }
        }
        QWidget::mouseMoveEvent(event);
    } catch (const std::exception& e) {
        qDebug() << "Error in mouseMoveEvent:" << e.what();
        QWidget::mouseMoveEvent(event);
    }
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
    try {
        if (m_currentImage.isNull()) {
            emit imageStatsUpdated(0.0);
            return;
        }

        double sum = 0.0;
        int count = 0;
        
        // 遍历图像所有像素
        for (int y = 0; y < m_currentImage.height(); ++y) {
            for (int x = 0; x < m_currentImage.width(); ++x) {
                QColor pixelColor = m_currentImage.pixelColor(x, y);
                int grayValue = qGray(pixelColor.rgb());
                sum += grayValue;
                ++count;
            }
        }
        
        if (count > 0) {
            double meanValue = sum / count;
            qDebug() << "Image stats calculated: mean =" << meanValue << "count =" << count;
            emit imageStatsUpdated(meanValue);
        } else {
            emit imageStatsUpdated(0.0);
        }
    } catch (const std::exception& e) {
        qDebug() << "Error in updateImageStats:" << e.what();
        emit imageStatsUpdated(0.0);
    }
}

void ProcessingWidget::displayImage(const QImage &image)
{
    try {
        if (image.isNull()) {
            qDebug() << "Warning: Attempted to display null image";
            return;
        }

        if (!imageLabel) {
            qDebug() << "Error: imageLabel is null";
            return;
        }

        m_currentImage = image;
        
        QPixmap pixmap = QPixmap::fromImage(image);
        if (pixmap.isNull()) {
            qDebug() << "Error: Failed to create pixmap from image";
            return;
        }

        qDebug() << "Scaling pixmap to label size:" << imageLabel->size();
        pixmap = pixmap.scaled(imageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        
        if (pixmap.isNull()) {
            qDebug() << "Error: Failed to scale pixmap";
            return;
        }

        imageLabel->setPixmap(pixmap);
        qDebug() << "Image display updated successfully";
        
        // 更新图像统计信息
        updateImageStats();
    } catch (const std::exception& e) {
        qDebug() << "Error displaying image:" << e.what();
    }
} 