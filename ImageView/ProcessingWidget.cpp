#include "ProcessingWidget.h"
#include <QPushButton>
#include <QSlider>
#include <QTabWidget>
#include <QGraphicsView>
#include <QLabel>
#include <QCheckBox>
#include <QGroupBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFrame>
#include <QMouseEvent>
#include <QPainter>
#include <QDebug>

ProcessingWidget::ProcessingWidget(QWidget *parent)
    : QWidget(parent)
    , btnSelect(nullptr)
    , btnSelectFolder(nullptr)
    , btnSave(nullptr)
    , btnShowOriginal(nullptr)
    , btnFlipH(nullptr)
    , btnFlipV(nullptr)
    , btnMeanFilter(nullptr)
    , btnGaussianFilter(nullptr)
    , btnMedianFilter(nullptr)
    , tabWidget(nullptr)
    , graphicsView(nullptr)
    , imageLabel(nullptr)
    , gbBasic(nullptr)
    , sliderBrightness(nullptr)
    , sliderThreshold(nullptr)
    , sliderContrast(nullptr)
    , sliderSaturation(nullptr)
    , gbColor(nullptr)
    , sliderR(nullptr)
    , sliderG(nullptr)
    , sliderB(nullptr)
    , m_processorThread(new ImageProcessorThread(this))
    , m_zoomFactor(1.0)
    , ZOOM_FACTOR_STEP(0.1)
    , MAX_ZOOM(2.0)
    , MIN_ZOOM(0.5)
{
    setMouseTracking(true);  // 启用鼠标跟踪
    setupUi();

    // 连接图像处理线程的信号
    connect(m_processorThread, &ImageProcessorThread::imageProcessed,
            this, &ProcessingWidget::onImageProcessed);
    connect(m_processorThread, &ImageProcessorThread::imageStatsUpdated,
            this, &ProcessingWidget::onImageStatsUpdated);

    // 启动处理线程
    m_processorThread->start();
}

ProcessingWidget::~ProcessingWidget()
{
    m_processorThread->stop();
    m_processorThread->wait();
}

void ProcessingWidget::setupUi()
{
    auto *hMain = new QHBoxLayout(this);
    hMain->setContentsMargins(10,10,10,10);
    hMain->setSpacing(15);

    // 左侧布局
    QWidget *leftW = new QWidget;
    auto *vLeft = new QVBoxLayout(leftW);
    vLeft->setSpacing(12);
    leftW->setMinimumWidth(200);

    // 创建三个分组框
    QGroupBox *gbFile = new QGroupBox(tr("文件操作"));
    QGroupBox *gbFlip = new QGroupBox(tr("图像翻转"));
    QGroupBox *gbFilter = new QGroupBox(tr("图像滤波"));

    // 文件操作组
    auto *vFile = new QVBoxLayout(gbFile);
    btnSelect = new QPushButton(tr("选择图像"));
    btnSelectFolder = new QPushButton(tr("选择文件夹"));
    btnSave = new QPushButton(tr("保存图片"));
    btnShowOriginal = new QPushButton(tr("显示原图"));

    QSize buttonSize(120, 35);
    btnSelect->setFixedSize(buttonSize);
    btnSelectFolder->setFixedSize(buttonSize);
    btnSave->setFixedSize(buttonSize);
    btnShowOriginal->setFixedSize(buttonSize);

    vFile->addWidget(btnSelect);
    vFile->addWidget(btnSelectFolder);
    vFile->addWidget(btnSave);
    vFile->addWidget(btnShowOriginal);
    vFile->addStretch();

    // 图像翻转组
    auto *vFlip = new QVBoxLayout(gbFlip);
    btnFlipH = new QPushButton(tr("水平翻转"));
    btnFlipV = new QPushButton(tr("垂直翻转"));

    btnFlipH->setFixedSize(buttonSize);
    btnFlipV->setFixedSize(buttonSize);

    vFlip->addWidget(btnFlipH);
    vFlip->addWidget(btnFlipV);
    vFlip->addStretch();

    // 图像滤波组
    auto *vFilter = new QVBoxLayout(gbFilter);
    btnMeanFilter = new QPushButton(tr("均值滤波"));
    btnGaussianFilter = new QPushButton(tr("高斯滤波"));
    btnMedianFilter = new QPushButton(tr("中值滤波"));

    btnMeanFilter->setFixedSize(buttonSize);
    btnGaussianFilter->setFixedSize(buttonSize);
    btnMedianFilter->setFixedSize(buttonSize);

    vFilter->addWidget(btnMeanFilter);
    vFilter->addWidget(btnGaussianFilter);
    vFilter->addWidget(btnMedianFilter);
    vFilter->addStretch();

    // 将分组框添加到左侧布局
    vLeft->addWidget(gbFile);
    vLeft->addWidget(gbFlip);
    vLeft->addWidget(gbFilter);
    vLeft->addStretch();

    // 中间布局
    QWidget *centerW = new QWidget;
    auto *vCenter = new QVBoxLayout(centerW);
    imageLabel = new QLabel;
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setMinimumSize(600, 500);
    imageLabel->setStyleSheet("QLabel { background-color : white; border: 1px solid gray; }");
    imageLabel->setMouseTracking(true);  // 启用鼠标跟踪
    vCenter->addWidget(imageLabel);

    // 右侧布局
    QWidget *rightW = new QWidget;
    auto *vRight = new QVBoxLayout(rightW);
    vRight->setSpacing(20);
    rightW->setMinimumWidth(250);

    gbBasic = new QGroupBox(tr("基础调整"));
    gbBasic->setMinimumHeight(300);
    auto *fBasic = new QFormLayout(gbBasic);
    fBasic->setSpacing(15);

    sliderBrightness = new QSlider(Qt::Horizontal);
    sliderBrightness->setRange(-100,100);
    sliderBrightness->setMinimumHeight(30);

    sliderThreshold  = new QSlider(Qt::Horizontal);
    sliderThreshold->setRange(0,255);
    sliderThreshold->setValue(128);
    sliderThreshold->setMinimumHeight(30);

    sliderContrast   = new QSlider(Qt::Horizontal);
    sliderContrast->setMinimumHeight(30);

    sliderSaturation = new QSlider(Qt::Horizontal);
    sliderSaturation->setMinimumHeight(30);

    fBasic->addRow(tr("调节亮度值:"), sliderBrightness);
    fBasic->addRow(tr("二值化:"),     sliderThreshold);
    fBasic->addRow(tr("对比度:"),     sliderContrast);
    fBasic->addRow(tr("饱和度:"),     sliderSaturation);
    vRight->addWidget(gbBasic);

    gbColor = new QGroupBox(tr("色彩调整"));
    gbColor->setMinimumHeight(200);
    auto *fColor = new QFormLayout(gbColor);
    fColor->setSpacing(15);

    sliderR = new QSlider(Qt::Horizontal);
    sliderG = new QSlider(Qt::Horizontal);
    sliderB = new QSlider(Qt::Horizontal);

    sliderR->setMinimumHeight(30);
    sliderG->setMinimumHeight(30);
    sliderB->setMinimumHeight(30);

    fColor->addRow(tr("R值:"), sliderR);
    fColor->addRow(tr("G值:"), sliderG);
    fColor->addRow(tr("B值:"), sliderB);
    vRight->addWidget(gbColor);
    vRight->addStretch(1);

    hMain->addWidget(leftW);
    hMain->addWidget(centerW, 1);
    hMain->addWidget(rightW);
}

void ProcessingWidget::displayImage(const QImage &image)
{
    m_currentImage = image;
    m_processorThread->setImage(image);
}

void ProcessingWidget::onImageProcessed(const QImage &processedImage)
{
    QPixmap pixmap = QPixmap::fromImage(processedImage);
    pixmap = pixmap.scaled(imageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    imageLabel->setPixmap(pixmap);
}

void ProcessingWidget::onImageStatsUpdated(double meanValue)
{
    emit imageStatsUpdated(meanValue);
}

void ProcessingWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && !m_currentImage.isNull()) {
        // 获取QLabel中的图像显示区域
        QRect imageRect = imageLabel->contentsRect();
        QSize imageSize = m_currentImage.size();

        // 计算缩放比例
        double scaleX = static_cast<double>(imageSize.width()) / imageRect.width();
        double scaleY = static_cast<double>(imageSize.height()) / imageRect.height();

        // 计算相对于QLabel的点击位置
        QPoint labelPos = imageLabel->mapFrom(this, event->pos());

        // 确保点击在图像区域内
        if (labelPos.x() >= 0 && labelPos.x() < imageRect.width() &&
            labelPos.y() >= 0 && labelPos.y() < imageRect.height()) {

            // 计算实际图像坐标
            int imageX = static_cast<int>(labelPos.x() * scaleX);
            int imageY = static_cast<int>(labelPos.y() * scaleY);

            // 确保坐标在图像范围内
            imageX = qBound(0, imageX, imageSize.width() - 1);
            imageY = qBound(0, imageY, imageSize.height() - 1);

            QColor pixelColor = m_currentImage.pixelColor(imageX, imageY);
            int grayValue = qGray(pixelColor.rgb());

            emit mouseClicked(QPoint(imageX, imageY), grayValue,
                            pixelColor.red(), pixelColor.green(), pixelColor.blue());
        }
    }
    QWidget::mousePressEvent(event);
}

void ProcessingWidget::wheelEvent(QWheelEvent *event)
{
    if (event->angleDelta().y() > 0) {
        // Zoom in
        m_zoomFactor = qMin(m_zoomFactor + ZOOM_FACTOR_STEP, MAX_ZOOM);
    } else {
        // Zoom out
        m_zoomFactor = qMax(m_zoomFactor - ZOOM_FACTOR_STEP, MIN_ZOOM);
    }

    // Update the displayed image with new zoom factor
    if (!m_currentImage.isNull()) {
        QImage scaledImage = m_currentImage.scaled(
            m_currentImage.width() * m_zoomFactor,
            m_currentImage.height() * m_zoomFactor,
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
        );
        imageLabel->setPixmap(QPixmap::fromImage(scaledImage));
    }
}


