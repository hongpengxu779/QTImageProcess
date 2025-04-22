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
        if (!m_imageUpdated) {
            m_condition.wait(&m_mutex);
        }
        if (m_stop) break;

        QImage image = m_currentImage;
        m_imageUpdated = false;
        locker.unlock();

        if (!image.isNull()) {
            processImage();
            calculateImageStats();
        }
    }
}

void ImageProcessorThread::processImage()
{
    // 在这里进行图像处理
    // 目前只是简单地将处理后的图像发送出去
    emit imageProcessed(m_currentImage);
}

void ImageProcessorThread::calculateImageStats()
{
    if (m_currentImage.isNull()) {
        return;
    }

    double sum = 0;
    int count = 0;
    
    const uchar *bits = m_currentImage.bits();
    int bytesPerLine = m_currentImage.bytesPerLine();
    int width = m_currentImage.width();
    int height = m_currentImage.height();
    
    for (int y = 0; y < height; ++y) {
        const uchar *line = bits + y * bytesPerLine;
        for (int x = 0; x < width; ++x) {
            sum += line[x];
            ++count;
        }
    }
    
    if (count > 0) {
        double meanValue = sum / count;
        emit imageStatsUpdated(meanValue);
    }
} 