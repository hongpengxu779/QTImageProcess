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
    , btnHistEqual(nullptr)
    , tabWidget(nullptr)
    , graphicsView(nullptr)
    , imageLabel(nullptr)
    , gbBasic(nullptr)
    , sliderBrightness(nullptr)
    , gbColor(nullptr)
    , m_subtractFiltered(nullptr)
    , m_rgbToGray(nullptr)
    , m_showHistogram(nullptr)
    , sliderGamma(nullptr)
    , sliderOffset(nullptr)
    , lblKValue(nullptr)
    , lblBValue(nullptr)
    , lblGammaValue(nullptr)
    , m_zoomFactor(1.0)
    , ZOOM_FACTOR_STEP(0.1)
    , MAX_ZOOM(2.0)
    , MIN_ZOOM(0.5)
{
    try {
        qDebug() << "Initializing ProcessingWidget...";

        setMouseTracking(true);

        // 设置UI
        setupUi();
        if (!imageLabel) {
            throw std::runtime_error("Failed to create imageLabel");
        }
        qDebug() << "UI setup completed";

        qDebug() << "ProcessingWidget initialization completed successfully";
    } catch (const std::exception& e) {
        qCritical() << "Error in ProcessingWidget constructor:" << e.what();
        throw; // 重新抛出异常以便在MainWindow中捕获
    }
}

ProcessingWidget::~ProcessingWidget()
{
    try {
        // 清理资源
    } catch (const std::exception& e) {
        qDebug() << "Error in ProcessingWidget destructor:" << e.what();
    }
}

void ProcessingWidget::setupUi()
{
    try {
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

        // 添加复选框
        m_subtractFiltered = new QCheckBox(tr("从原图中减去"));
        m_subtractFiltered->setChecked(false);
        vFilter->addWidget(m_subtractFiltered);

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
        imageLabel->setMouseTracking(true);
        vCenter->addWidget(imageLabel);

        // 右侧布局
        QWidget *rightW = new QWidget;
        auto *vRight = new QVBoxLayout(rightW);
        vRight->setSpacing(20);
        rightW->setMinimumWidth(250);

        // 基础调整组 - 包含线性变换、Gamma校正和RGB转灰度
        gbBasic = new QGroupBox(tr("基础调整"));
        gbBasic->setMinimumHeight(400);
        auto *vBasic = new QVBoxLayout(gbBasic);
        vBasic->setSpacing(15);
        vBasic->setContentsMargins(10, 20, 10, 10);

        // 添加RGB转灰度复选框
        m_rgbToGray = new QCheckBox(tr("RGB转灰度图"));
        m_rgbToGray->setChecked(false);
        m_rgbToGray->setStyleSheet("QCheckBox { font-weight: bold; }");
        vBasic->addWidget(m_rgbToGray);

        // 添加间距
        vBasic->addSpacing(10);

        // 添加线性变换标签
        QLabel *lblLinearTransform = new QLabel(tr("<b>线性变换 (y = kx + b)</b>"));
        vBasic->addWidget(lblLinearTransform);

        // 创建系数k的滑动条和标签
        QWidget *kWidget = new QWidget;
        auto *kLayout = new QHBoxLayout(kWidget);
        kLayout->setContentsMargins(0, 0, 0, 0);
        kLayout->setSpacing(0);

        QLabel *lblK = new QLabel(tr("系数 k:"));
        lblK->setMinimumWidth(50);

        sliderBrightness = new QSlider(Qt::Horizontal);
        sliderBrightness->setRange(-100, 100);
        sliderBrightness->setValue(0);
        sliderBrightness->setMinimumHeight(30);
        sliderBrightness->setTickPosition(QSlider::TicksBelow);
        sliderBrightness->setTickInterval(20);

        lblKValue = new QLabel("k = 1.00");
        lblKValue->setMinimumWidth(70);

        kLayout->addWidget(lblK);
        kLayout->addWidget(sliderBrightness);
        kLayout->addWidget(lblKValue);

        vBasic->addWidget(kWidget);

        // 创建偏移量b的滑动条和标签
        QWidget *bWidget = new QWidget;
        auto *bLayout = new QHBoxLayout(bWidget);
        bLayout->setContentsMargins(0, 0, 0, 0);
        bLayout->setSpacing(0);

        QLabel *lblB = new QLabel(tr("偏移量 b:"));
        lblB->setMinimumWidth(50);

        sliderOffset = new QSlider(Qt::Horizontal);
        sliderOffset->setRange(-100, 100);
        sliderOffset->setValue(0);
        sliderOffset->setMinimumHeight(30);
        sliderOffset->setTickPosition(QSlider::TicksBelow);
        sliderOffset->setTickInterval(20);

        lblBValue = new QLabel("b = 0");
        lblBValue->setMinimumWidth(70);

        bLayout->addWidget(lblB);
        bLayout->addWidget(sliderOffset);
        bLayout->addWidget(lblBValue);

        vBasic->addWidget(bWidget);

        // 添加间距
        vBasic->addSpacing(10);

        // 添加Gamma校正标签
        QLabel *lblGammaCorrection = new QLabel(tr("<b>Gamma校正</b>"));
        vBasic->addWidget(lblGammaCorrection);

        // 创建Gamma的滑动条和标签
        QWidget *gammaWidget = new QWidget;
        auto *gammaLayout = new QHBoxLayout(gammaWidget);
        gammaLayout->setContentsMargins(0, 0, 0, 0);
        gammaLayout->setSpacing(0);

        QLabel *lblGamma = new QLabel(tr("Gamma值:"));
        lblGamma->setMinimumWidth(70);

        sliderGamma = new QSlider(Qt::Horizontal);
        sliderGamma->setRange(1, 100);
        sliderGamma->setValue(10);
        sliderGamma->setMinimumHeight(30);
        sliderGamma->setTickPosition(QSlider::TicksBelow);
        sliderGamma->setTickInterval(10);

        lblGammaValue = new QLabel("γ = 1.00");
        lblGammaValue->setMinimumWidth(80);
        lblGammaValue->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

        gammaLayout->addWidget(lblGamma);
        gammaLayout->addSpacing(5);
        gammaLayout->addWidget(sliderGamma);
        gammaLayout->addWidget(lblGammaValue);

        vBasic->addWidget(gammaWidget);

        // 添加弹性空间
        vBasic->addStretch();

        vRight->addWidget(gbBasic);

        // 色彩调整组
        gbColor = new QGroupBox(tr("灰度直方图调整"));
        gbColor->setMinimumHeight(150);
        auto *vColor = new QVBoxLayout(gbColor);
        vColor->setSpacing(15);

        // Add histogram display checkbox
        m_showHistogram = new QCheckBox(tr("显示灰度直方图"));
        m_showHistogram->setChecked(false);
        m_showHistogram->setStyleSheet("QCheckBox { font-weight: bold; }");
        vColor->addWidget(m_showHistogram);

        // Connect the checkbox state change signal
        connect(m_showHistogram, &QCheckBox::stateChanged, [this](int state) {
            emit showHistogramRequested(state == Qt::Checked);
        });

        // 添加一些说明文字
        QLabel *histogramInfoLabel = new QLabel(tr("注意：显示直方图将会打开一个新窗口，显示当前图像的灰度分布情况。"));
        histogramInfoLabel->setWordWrap(true);
        histogramInfoLabel->setStyleSheet("QLabel { color: #666; }");
        vColor->addWidget(histogramInfoLabel);

        // 添加弹性空间
        vColor->addStretch();

        // 将组添加到右侧布局
        vRight->addWidget(gbColor);
        vRight->addStretch(1);

        // 连接滑块值改变信号到更新标签函数
        connect(sliderBrightness, &QSlider::valueChanged, this, &ProcessingWidget::updateKValueLabel);
        connect(sliderOffset, &QSlider::valueChanged, this, &ProcessingWidget::updateBValueLabel);
        connect(sliderGamma, &QSlider::valueChanged, this, &ProcessingWidget::updateGammaValueLabel);

        // 初始化标签显示
        updateKValueLabel(sliderBrightness->value());
        updateBValueLabel(sliderOffset->value());
        updateGammaValueLabel(sliderGamma->value());

        hMain->addWidget(leftW);
        hMain->addWidget(centerW, 1);
        hMain->addWidget(rightW);
    } catch (const std::exception& e) {
        qDebug() << "Error in setupUi:" << e.what();
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

        // 触发图像统计信息更新
        emit imageStatsUpdated(calculateMeanValue(image));
    } catch (const std::exception& e) {
        qDebug() << "Error displaying image:" << e.what();
    }
}

// 添加一个辅助函数来计算图像的均值
double ProcessingWidget::calculateMeanValue(const QImage &image)
{
    try {
        if (image.isNull()) {
            return 0.0;
        }

        double sum = 0.0;
        int count = 0;

        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                QColor pixel = image.pixelColor(x, y);
                int gray = qGray(pixel.rgb());
                sum += gray;
                count++;
            }
        }

        return count > 0 ? sum / count : 0.0;
    } catch (const std::exception& e) {
        qDebug() << "Error calculating mean value:" << e.what();
        return 0.0;
    }
}

void ProcessingWidget::onImageProcessed(const QImage &processedImage)
{
    try {
        if (processedImage.isNull()) {
            qDebug() << "Warning: Received null processed image";
            return;
        }

        if (!imageLabel) {
            qDebug() << "Error: imageLabel is null";
            return;
        }

        qDebug() << "Processing image size:" << processedImage.width() << "x" << processedImage.height();
        m_currentImage = processedImage;  // 保存当前图像

        QPixmap pixmap = QPixmap::fromImage(processedImage);
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
    } catch (const std::exception& e) {
        qDebug() << "Error in onImageProcessed:" << e.what();
    }
}

void ProcessingWidget::onImageStatsUpdated(double meanValue)
{
    try {
        emit imageStatsUpdated(meanValue);
    } catch (const std::exception& e) {
        qDebug() << "Error in onImageStatsUpdated:" << e.what();
    }
}

void ProcessingWidget::mousePressEvent(QMouseEvent *event)
{
    try {
        if (event->button() == Qt::LeftButton && !m_currentImage.isNull() && imageLabel) {
            // 获取当前鼠标位置相对于imageLabel的坐标
            QPoint mousePos = imageLabel->mapFrom(this, event->pos());

            // 获取图像标签的尺寸
            QSize labelSize = imageLabel->size();

            // 计算图像缩放尺寸（保持纵横比）
            QSize scaledImageSize;
            double aspectRatio = static_cast<double>(m_currentImage.width()) / m_currentImage.height();

            if (labelSize.width() / aspectRatio <= labelSize.height()) {
                // 宽度限制
                scaledImageSize.setWidth(labelSize.width());
                scaledImageSize.setHeight(static_cast<int>(labelSize.width() / aspectRatio));
            } else {
                // 高度限制
                scaledImageSize.setHeight(labelSize.height());
                scaledImageSize.setWidth(static_cast<int>(labelSize.height() * aspectRatio));
            }

            // 计算图像在标签中的位置（居中显示）
            int offsetX = (labelSize.width() - scaledImageSize.width()) / 2;
            int offsetY = (labelSize.height() - scaledImageSize.height()) / 2;

            // 计算鼠标位置相对于图像左上角的偏移
            int relX = mousePos.x() - offsetX;
            int relY = mousePos.y() - offsetY;

            // 检查鼠标是否在图像区域内
            if (relX >= 0 && relX < scaledImageSize.width() &&
                relY >= 0 && relY < scaledImageSize.height()) {

                // 计算在原始图像中的坐标
                int imageX = static_cast<int>(relX * m_currentImage.width() / scaledImageSize.width());
                int imageY = static_cast<int>(relY * m_currentImage.height() / scaledImageSize.height());

                // 确保坐标在图像范围内
                imageX = qMin(imageX, m_currentImage.width() - 1);
                imageY = qMin(imageY, m_currentImage.height() - 1);

                // 获取像素颜色
                QColor pixelColor = m_currentImage.pixelColor(imageX, imageY);
                int grayValue = qGray(pixelColor.rgb());

                // 发送信号 - 这里发送的是原始图像中的坐标
                emit mouseClicked(QPoint(imageX, imageY), grayValue,
                                 pixelColor.red(), pixelColor.green(), pixelColor.blue());

                // 输出调试信息帮助诊断
                qDebug() << "Click in image at:" << QPoint(imageX, imageY)
                         << "RGB:" << pixelColor.red() << pixelColor.green() << pixelColor.blue();
            }
        }
    } catch (const std::exception& e) {
        qDebug() << "Error in mousePressEvent:" << e.what();
    }
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

// 更新k值标签
void ProcessingWidget::updateKValueLabel(int value)
{
    double k = 1.0;
    if (value >= 0) {
        k = 1.0 + value / 100.0;  // k范围是1.0到2.0
    } else {
        k = 1.0 + value / 100.0;  // k范围是0.0到1.0
    }
    lblKValue->setText(QString("k = %1").arg(k, 0, 'f', 2));
}

// 更新b值标签
void ProcessingWidget::updateBValueLabel(int value)
{
    lblBValue->setText(QString("b = %1").arg(value));
}

// 更新Gamma值标签
void ProcessingWidget::updateGammaValueLabel(int value)
{
    double gamma = value / 10.0;
    lblGammaValue->setText(QString("γ = %1").arg(gamma, 0, 'f', 2));
}

// 重置标签显示
void ProcessingWidget::resetValueLabels()
{
    // 重置k值标签为默认值
    lblKValue->setText("k = 1.00");

    // 重置b值标签为默认值
    lblBValue->setText("b = 0");

    // 重置Gamma值标签为默认值
    lblGammaValue->setText("γ = 1.00");
}

void ProcessingWidget::mouseMoveEvent(QMouseEvent *event)
{
    try {
        if (!m_currentImage.isNull() && imageLabel) {
            // 获取当前鼠标位置相对于imageLabel的坐标
            QPoint mousePos = imageLabel->mapFrom(this, event->pos());

            // 获取图像标签的尺寸
            QSize labelSize = imageLabel->size();

            // 计算图像缩放尺寸（保持纵横比）
            QSize scaledImageSize;
            double aspectRatio = static_cast<double>(m_currentImage.width()) / m_currentImage.height();

            if (labelSize.width() / aspectRatio <= labelSize.height()) {
                // 宽度限制
                scaledImageSize.setWidth(labelSize.width());
                scaledImageSize.setHeight(static_cast<int>(labelSize.width() / aspectRatio));
            } else {
                // 高度限制
                scaledImageSize.setHeight(labelSize.height());
                scaledImageSize.setWidth(static_cast<int>(labelSize.height() * aspectRatio));
            }

            // 计算图像在标签中的位置（居中显示）
            int offsetX = (labelSize.width() - scaledImageSize.width()) / 2;
            int offsetY = (labelSize.height() - scaledImageSize.height()) / 2;

            // 计算鼠标位置相对于图像左上角的偏移
            int relX = mousePos.x() - offsetX;
            int relY = mousePos.y() - offsetY;

            // 检查鼠标是否在图像区域内
            if (relX >= 0 && relX < scaledImageSize.width() &&
                relY >= 0 && relY < scaledImageSize.height()) {

                // 计算在原始图像中的坐标
                int imageX = static_cast<int>(relX * m_currentImage.width() / scaledImageSize.width());
                int imageY = static_cast<int>(relY * m_currentImage.height() / scaledImageSize.height());

                // 确保坐标在图像范围内
                imageX = qMin(imageX, m_currentImage.width() - 1);
                imageY = qMin(imageY, m_currentImage.height() - 1);

                // 获取像素颜色
                QColor pixelColor = m_currentImage.pixelColor(imageX, imageY);
                int grayValue = qGray(pixelColor.rgb());

                // 发送信号 - 这里发送的是原始图像中的坐标
                emit mouseMoved(QPoint(imageX, imageY), grayValue);

                // 输出调试信息帮助诊断
                qDebug() << "Mouse in image at:" << QPoint(imageX, imageY)
                         << "RGB:" << pixelColor.red() << pixelColor.green() << pixelColor.blue();
            }
        }
        QWidget::mouseMoveEvent(event);
    } catch (const std::exception& e) {
        qDebug() << "Error in mouseMoveEvent:" << e.what();
        QWidget::mouseMoveEvent(event);
    }
}


