#include "ImageProcessorThread.h"
#include <QDebug>

ImageProcessorThread::ImageProcessorThread(QObject *parent)
    : QThread(parent)
    , m_stop(false)
    , m_imageUpdated(false)
{
}

ImageProcessorThread::~ImageProcessorThread()
{
    stop();
    wait();
}

void ImageProcessorThread::setImage(const QImage &image)
{
    if (image.isNull()) {
        qDebug() << "Warning: Attempted to set null image";
        return;
    }

    QMutexLocker locker(&m_mutex);
    m_currentImage = image;
    m_imageUpdated = true;
    m_condition.wakeOne();
}

void ImageProcessorThread::stop()
{
    QMutexLocker locker(&m_mutex);
    m_stop = true;
    m_condition.wakeOne();
}

void ImageProcessorThread::run()
{
    while (!m_stop) {
        QMutexLocker locker(&m_mutex);
        if (!m_imageUpdated && !m_stop) {
            m_condition.wait(&m_mutex);
        }
        if (m_stop) break;

        QImage image = m_currentImage;
        m_imageUpdated = false;
        locker.unlock();

        if (!image.isNull()) {
            try {
                processImage();
                calculateImageStats();
            } catch (const std::exception& e) {
                qDebug() << "Error processing image:" << e.what();
            }
        }
    }
}

void ImageProcessorThread::processImage()
{
    if (m_currentImage.isNull()) {
        qDebug() << "Error: Cannot process null image";
        return;
    }

    // 确保在主线程中发送信号
    QMetaObject::invokeMethod(this, [this]() {
        emit imageProcessed(m_currentImage);
    }, Qt::QueuedConnection);
}

void ImageProcessorThread::calculateImageStats()
{
    if (m_currentImage.isNull()) {
        qDebug() << "Error: Cannot calculate stats for null image";
        return;
    }

    try {
        double sum = 0;
        int count = 0;
        
        const uchar *bits = m_currentImage.bits();
        if (!bits) {
            qDebug() << "Error: Invalid image bits";
            return;
        }

        int bytesPerLine = m_currentImage.bytesPerLine();
        int width = m_currentImage.width();
        int height = m_currentImage.height();
        
        if (width <= 0 || height <= 0) {
            qDebug() << "Error: Invalid image dimensions";
            return;
        }

        for (int y = 0; y < height; ++y) {
            const uchar *line = bits + y * bytesPerLine;
            for (int x = 0; x < width; ++x) {
                sum += line[x];
                ++count;
            }
        }
        
        if (count > 0) {
            double meanValue = sum / count;
            // 确保在主线程中发送信号
            QMetaObject::invokeMethod(this, [this, meanValue]() {
                emit imageStatsUpdated(meanValue);
            }, Qt::QueuedConnection);
        }
    } catch (const std::exception& e) {
        qDebug() << "Error calculating image stats:" << e.what();
    }
} 