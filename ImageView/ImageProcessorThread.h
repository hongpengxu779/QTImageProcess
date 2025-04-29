#ifndef IMAGEPROCESSORTHREAD_H
#define IMAGEPROCESSORTHREAD_H

#include <QThread>
#include <QImage>
#include <QMutex>
#include <QWaitCondition>

class ImageProcessorThread : public QThread
{
    Q_OBJECT

public:
    explicit ImageProcessorThread(QObject *parent = nullptr);
    ~ImageProcessorThread();

    void setImage(const QImage &image);
    void stop();

signals:
    void imageProcessed(const QImage &processedImage);
    void imageStatsUpdated(double meanValue);

protected:
    void run() override;

private:
    QImage m_currentImage;
    QMutex m_mutex;
    QWaitCondition m_condition;
    bool m_stop;
    bool m_imageUpdated;
    QString m_lastSaveFolder;

    void processImage();
    void calculateImageStats();
};

#endif // IMAGEPROCESSORTHREAD_H 